#include <zlib.h>
#include <inttypes.h>
#include <stdatomic.h>
#include <arpa/inet.h>
#include "visual-details.h"
#include "internal.h"
#include "png.h"

// http://www.libpng.org/pub/png/spec/1.2/PNG-Contents.html

// 4-byte length, 4-byte type, 4-byte CRC, plus data
#define CHUNK_DESC_BYTES 12
#define IHDR_DATA_BYTES 13

// PNG generally allows unsigned 32-bit values to only reach 2**31 "to assist
// [loser] languages which have difficulty dealing with unsigned values."
#define CHUNK_MAX_DATA 0x80000000llu
static const unsigned char PNGHEADER[] = "\x89PNG\x0d\x0a\x1a\x0a";

// FIXME replace with PCLMULQDQ method (and ARM CRC32 instruction)
// this is taken from the PNG reference
static unsigned long crc_table[256];

static atomic_bool crc_table_computed;

static void
make_crc_table(void){
  for(size_t n = 0 ; n < sizeof(crc_table) / sizeof(*crc_table) ; n++){
    unsigned long c = n;
    for(int k = 0 ; k < 8 ; k++){
      if(c & 1){
        c = 0xedb88320L ^ (c >> 1);
      }else{
        c = c >> 1;
      }
    }
    crc_table[n] = c;
  }
  crc_table_computed = true;
}

static inline unsigned long
update_crc(unsigned long crc, const unsigned char *buf, int len){
  unsigned long c = crc;
  int n;

  if(!crc_table_computed){
    make_crc_table();
  }
  for(n = 0 ; n < len ; n++){
    c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
  }
  return c;
}

static inline unsigned long
crc(const unsigned char *buf, int len){
  return update_crc(0xffffffffL, buf, len) ^ 0xffffffffL;
}

// compress the ncvisual data suitably for PNG. this requires adding a byte
// of filter type (currently always 0) before each scanline =[.
static void*
compress_image(const ncvisual* ncv, size_t* dlen){
  z_stream zctx;
  int zret;
  if((zret = deflateInit(&zctx, Z_DEFAULT_COMPRESSION)) != Z_OK){
    logerror("Couldn't get a deflate context (%d)\n", zret);
    return NULL;
  }
  // one byte per scanline for adaptive filtering type (always 0 for now)
  uint64_t databytes = ncv->pixx * ncv->pixy * 4 + ncv->pixy;
  unsigned long bound = deflateBound(&zctx, databytes);
  unsigned char* buf = malloc(bound);
  if(buf == NULL){
    logerror("Couldn't allocate %zuB\n", bound);
    deflateEnd(&zctx);
    return NULL;
  }
  zctx.next_out = buf;
  zctx.avail_out = bound;
  // enough space for a single scanline + filter byte
  unsigned char* sbuf = malloc(1 + ncv->pixx * 4);
  if(sbuf == NULL){
    logerror("Couldn't allocate %zuB\n", 1 + ncv->pixx * 4);
    deflateEnd(&zctx);
    free(buf);
    return NULL;
  }
  for(int i = 0 ; i < ncv->pixy ; ++i){
    if(zctx.avail_out == 0){
      free(buf);
      free(sbuf);
      return NULL;
    }
    zctx.avail_in = ncv->pixx * 4 + 1;
    sbuf[0] = 0;
    memcpy(sbuf + 1, ncv->data + ncv->rowstride * i, ncv->pixx * 4);
    zctx.next_in = sbuf;
    if((zret = deflate(&zctx, Z_NO_FLUSH)) != Z_OK){
      logerror("Error deflating %dB to %dB (%d)\n", zctx.avail_in, zctx.avail_out, zret);
      free(buf);
      free(sbuf);
      return NULL;
    }
  }
  if((zret = deflate(&zctx, Z_FINISH)) != Z_STREAM_END){
    logerror("Error finalizing %dB to %dB (%d)\n", zctx.avail_in, zctx.avail_out, zret);
    free(buf);
    free(sbuf);
    return NULL;
  }
  free(sbuf);
  *dlen = zctx.total_out;
  if((zret = deflateEnd(&zctx)) != Z_OK){
    logerror("Couldn't finalize the deflate context (%d)\n", zret);
    free(buf);
    return NULL;
  }
  loginfo("Deflated %"PRIu64" to %zu\n", databytes, *dlen);
  return buf;
}

// number of bytes necessary to encode (uncompressed) the visual specified by
// |ncv|. on error, *|deflated| will be NULL.
size_t compute_png_size(const ncvisual* ncv, void** deflated, size_t* dlen){
  if((*deflated = compress_image(ncv, dlen)) == NULL){
    return 0;
  }
//fprintf(stderr, "ACTUAL: %zu (0x%02x) (0x%02x)\n", *dlen, (*(char **)deflated)[*dlen - 1], (*(char**)deflated)[20]);
  uint64_t fullchunks = *dlen / CHUNK_MAX_DATA; // full 2GB IDATs
  return (sizeof(PNGHEADER) - 1) +  // PNG header
         CHUNK_DESC_BYTES + IHDR_DATA_BYTES + // mandatory IHDR chunk
         (CHUNK_DESC_BYTES + CHUNK_MAX_DATA) * fullchunks +
         (CHUNK_DESC_BYTES + *dlen % CHUNK_MAX_DATA) +
         CHUNK_DESC_BYTES; // mandatory IEND chunk
}

// calculate the PNG CRC of the chunk starting at |buf|. all chunks start with
// a 4-byte length parameter in NBO, which we use to determine the area over
// which the crc must be calculated (this length only covers the data, not the
// chunk header, so we add 8 bytes). returns 0 on error, but that's also a
// valid value, so whacha gonna do?
static uint32_t
chunk_crc(const unsigned char* buf){
  uint32_t length;
  memcpy(&length, buf, 4);
  length = ntohl(length);
  if(length > CHUNK_MAX_DATA){
    logerror("Chunk length too large (%lu)\n", length);
    return 0;
  }
  length += 4; // don't use length or crc fields
  uint32_t crc32 = htonl(crc(buf + 4, length));
  return crc32;
}

// write the ihdr at |buf|, which is guaranteed to be large enough (25B).
static size_t
write_ihdr(const ncvisual* ncv, unsigned char* buf){
  uint32_t length = htonl(IHDR_DATA_BYTES);
  memcpy(buf, &length, 4);
  static const char ctype[] = "IHDR";
  memcpy(buf + 4, ctype, 4);
  uint32_t width = htonl(ncv->pixx);
  memcpy(buf + 8, &width, 4);
  uint32_t height = htonl(ncv->pixy);
  memcpy(buf + 12, &height, 4);
  uint8_t depth = 8;                  // 8 bits per channel
  memcpy(buf + 16, &depth, 1);
  uint8_t color = 6;                  // RGBA
  memcpy(buf + 17, &color, 1);
  uint8_t compression = 0;            // deflate, max window 32768
  memcpy(buf + 18, &compression, 1);
  uint8_t filter = 0;                 // adaptive filtering
  memcpy(buf + 19, &filter, 1);
  uint8_t interlace = 0;              // no interlace
  memcpy(buf + 20, &interlace, 1);
  uint32_t crc = chunk_crc(buf);
  memcpy(buf + 21, &crc, 4);
  return CHUNK_DESC_BYTES + IHDR_DATA_BYTES; // 25
}

// write 1+ IDAT chunks at |buf| from the deflated |dlen| bytes at |data|.
static size_t
write_idats(unsigned char* buf, const unsigned char* data, size_t dlen){
  static const char ctype[] = "IDAT";
  uint32_t written = 0;
  uint32_t dwritten = 0;
  while(dlen){
    uint32_t thischunk = dlen;
    if(thischunk > CHUNK_MAX_DATA){
      thischunk = CHUNK_MAX_DATA;
    }
    uint32_t nclen = htonl(thischunk);
    memcpy(buf + written, &nclen, 4);
    memcpy(buf + written + 4, ctype, 4);
    memcpy(buf + written + 8, data + dwritten, thischunk);
    uint32_t crc = chunk_crc(buf + written);
    memcpy(buf + written + 8 + thischunk, &crc, 4);
    dlen -= thischunk;
    dwritten += thischunk;
    written += CHUNK_DESC_BYTES + thischunk;
  }
  return written;
}

// write the constant 12B IEND chunk at |buf|. it contains no data.
static size_t
write_iend(unsigned char* buf){
  static const char iend[] = "\x00\x00\x00\x00IEND\xae\x42\x60\x82";
  memcpy(buf, iend, CHUNK_DESC_BYTES);
  return CHUNK_DESC_BYTES;
}

// write a PNG at the provided buffer |buf| using the ncvisual ncv, the
// deflated data |deflated| of |dlen| bytes. |buf| must be large enough to
// write all necessary data; it ought have been sized with compute_png_size().
static size_t
create_png(const ncvisual* ncv, void* buf, const unsigned char* deflated,
           size_t dlen){
  size_t written = sizeof(PNGHEADER) - 1;
  memcpy(buf, PNGHEADER, written);
  size_t r = write_ihdr(ncv, (unsigned char*)buf + written);
  written += r;
  r = write_idats((unsigned char*)buf + written, deflated, dlen);
  written += r;
  r = write_iend((unsigned char*)buf + written);
  written += r;
  return written;
}

static inline size_t
mmap_round_size(size_t s){
  const size_t pgsize = 4096; // FIXME get page size, round up s
  return (s + pgsize - 1) / pgsize * pgsize;
}

// write a PNG, creating the buffer ourselves. it must be munmapped. the
// resulting length is written to *bsize on success (the file/map might be
// larger than this, but the end is immaterial padding). returns MMAP_FAILED
// on a failure. if |fd| is negative, an anonymous map will be made.
void* create_png_mmap(const ncvisual* ncv, size_t* bsize, int fd){
  void* deflated;
  size_t dlen;
  size_t mlen;
  *bsize = compute_png_size(ncv, &deflated, &dlen);
  if(deflated == NULL){
    logerror("Couldn't compress to %d\n", fd);
    return MAP_FAILED;
  }
  mlen = mmap_round_size(*bsize);
  if(mlen == 0){
    return MAP_FAILED;
  }
  if(fd >= 0){
    if(ftruncate(fd, mlen) < 0){
      logerror("Couldn't set size of %d to %zuB (%s)\n", fd, mlen, strerror(errno));
      free(deflated);
      return MAP_FAILED;
    }
    loginfo("Set size of %d to %zuB\n", fd, mlen);
  }
  // FIXME hugetlb?
  void* map = mmap(NULL, mlen, PROT_WRITE | PROT_READ,
#ifdef MAP_SHARED_VALIDATE
                   MAP_SHARED_VALIDATE |
#else
                   MAP_SHARED |
#endif
                   (fd >= 0 ? 0 : MAP_ANONYMOUS), fd, 0);
  if(map == MAP_FAILED){
    logerror("Couldn't get %zuB map for %d\n", mlen, fd);
    free(deflated);
    return MAP_FAILED;
  }
  size_t w = create_png(ncv, map, deflated, dlen);
  free(deflated);
  loginfo("Wrote %zuB PNG to %d\n", w, fd);
  if(fd >= 0){
    if(ftruncate(fd, w) < 0){
      logerror("Couldn't set size of %d to %zuB (%s)\n", fd, w, strerror(errno));
      munmap(map, mlen);
      return MAP_FAILED;
    }
  }
  return map;
}
