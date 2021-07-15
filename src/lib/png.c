#include <zlib.h>
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

// number of bytes necessary to encode (uncompressed) the visual specified by
// |ncv|. on error, *|deflated| will be NULL.
size_t compute_png_size(const ncvisual* ncv, void** deflated, size_t* dlen){
  uint64_t databytes = ncv->pixx * ncv->pixy * 4;
  unsigned long bound = compressBound(databytes);
  *deflated = malloc(bound);
  if(*deflated == NULL){
    return 0;
  }
  int cret = compress(*deflated, &bound, (const unsigned char*)ncv->data, databytes);
  if(cret != Z_OK){
    free(*deflated);
    logerror("Error compressing %zuB (%d)\n", databytes, cret);
    return 0;
  }
  *dlen = bound;
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
chunk_crc(const char* buf){
  uint32_t length;
  memcpy(&length, buf, 4);
  length = htonl(length);
  if(length > CHUNK_MAX_DATA){
    logerror("Chunk length too large (%lu)\n", length);
    return 0;
  }
  length += 8;
  uint32_t crc = 0;
  // FIXME evaluate crc32 over |length| bytes
  return crc;
}

// write the ihdr at |buf|, which is guaranteed to be large enough (25B).
static size_t
write_ihdr(const ncvisual* ncv, char* buf){
  uint32_t length = htonl(IHDR_DATA_BYTES);
  memcpy(buf, &length, 4);
  static const char ctype[] = "IHDR";
  memcpy(buf + 4, &ctype, 4);
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
write_idats(char* buf, const char* data, size_t dlen){
  uint32_t written = 0;
  while(dlen){
    size_t thischunk = dlen;
    if(thischunk > CHUNK_MAX_DATA){
      thischunk = CHUNK_MAX_DATA;
    }
    memcpy(buf + written, data + written, thischunk);
    dlen -= thischunk;
    written += CHUNK_DESC_BYTES + thischunk;
  }
  return written;
}

// write the constant 12B IEND chunk at |buf|. it contains no data.
static size_t
write_iend(char* buf){
  static const char iend[] = "\x00\x00\x00\x00IEND\xae\x42\x60\x82";
  memcpy(buf, iend, CHUNK_DESC_BYTES);
  return CHUNK_DESC_BYTES;
}

// write a PNG at the provided buffer |buf| using the ncvisual ncv, the
// deflated data |deflated| of |dlen| bytes. |buf| must be large enough to
// write all necessary data; it ought have been sized with compute_png_size().
static size_t
create_png(const ncvisual* ncv, void* buf, const char* deflated, size_t dlen){
  size_t written = sizeof(PNGHEADER) - 1;
  memcpy(buf, PNGHEADER, written);
  size_t r = write_ihdr(ncv, (char*)buf + written);
  written += r;
  r = write_idats((char*)buf + written, deflated, dlen);
  written += r;
  r = write_iend((char*)buf + written);
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
      logerror("Couldn't set size of %d to %zuB (%s)\n",
               fd, mlen, strerror(errno));
      return MAP_FAILED;
    }
    loginfo("Set size of %d to %zuB\n", fd, mlen);
  }
  // FIXME hugetlb?
  void* map = mmap(NULL, mlen, PROT_WRITE | PROT_READ,
                   MAP_SHARED_VALIDATE |
                   (fd >= 0 ? 0 : MAP_ANONYMOUS), fd, 0);
  if(map == MAP_FAILED){
    logerror("Couldn't get %zuB map for %d\n", mlen, fd);
    return MAP_FAILED;
  }
  size_t w = create_png(ncv, map, deflated, dlen);
  loginfo("Wrote %zuB PNG to %d\n", w, fd);
  return map;
}
