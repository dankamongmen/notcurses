#include <zlib.h>
#include <inttypes.h>
#include <stdatomic.h>
#include "visual-details.h"
#include "internal.h"
#include "base64.h"
#include "fbuf.h"
#include "png.h"

// http://www.libpng.org/pub/png/spec/1.2/PNG-Contents.html

// 4-byte length, 4-byte type, 4-byte CRC, plus data
#define CHUNK_DESC_BYTES 12
#define IHDR_DATA_BYTES 13

// PNG generally allows unsigned 32-bit values to only reach 2**31 "to assist
// [loser] languages which have difficulty dealing with unsigned values."
#define CHUNK_MAX_DATA 0x80000000llu
static const unsigned char PNGHEADER[] = "\x89PNG\x0d\x0a\x1a\x0a";
static const unsigned char IEND[] = "\x00\x00\x00\x00IEND\xae\x42\x60\x82";

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
compress_image(const void* data, int rows, int rowstride, int cols, size_t* dlen){
  z_stream zctx = { };
  int zret;
  if((zret = deflateInit(&zctx, Z_DEFAULT_COMPRESSION)) != Z_OK){
    logerror("Couldn't get a deflate context (%d)\n", zret);
    return NULL;
  }
  // one byte per scanline for adaptive filtering type (always 0 for now)
  uint64_t databytes = cols * rows * 4 + rows;
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
  unsigned char* sbuf = malloc(1 + cols * 4);
  if(sbuf == NULL){
    logerror("Couldn't allocate %zuB\n", 1 + cols * 4);
    free(buf);
    deflateEnd(&zctx);
    return NULL;
  }
  for(int i = 0 ; i < rows ; ++i){
    if(zctx.avail_out == 0){
      free(buf);
      free(sbuf);
      deflateEnd(&zctx);
      return NULL;
    }
    zctx.avail_in = cols * 4 + 1;
    // FIXME eliminate sbuf via 2x deflate(Z_NO_FLUSH)
    sbuf[0] = 0;
    memcpy(sbuf + 1, data + rowstride * i, cols * 4);
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
size_t compute_png_size(const void* data, int rows, int rowstride, int cols,
                        void** deflated, size_t* dlen){
  if((*deflated = compress_image(data, rows, rowstride, cols, dlen)) == NULL){
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
  length += 4; // don't use length or crc fields (but type *is* covered)
  uint32_t crc32 = htonl(crc(buf + 4, length));
  return crc32;
}

// write the ihdr at |buf|, which is guaranteed to be large enough (25B).
static size_t
write_ihdr(int rows, int cols, unsigned char buf[static 25]){
  uint32_t length = htonl(IHDR_DATA_BYTES);
  memcpy(buf, &length, 4);
  static const char ctype[] = "IHDR";
  memcpy(buf + 4, ctype, 4);
  uint32_t width = htonl(cols);
  memcpy(buf + 8, &width, 4);
  uint32_t height = htonl(rows);
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

struct b64ctx {
  unsigned char src[3]; // try to convert three at a time
  size_t srcidx;        // how many src bytes we have
};

static int
fbuf_putn64(fbuf* f, const void* src, size_t osize, struct b64ctx* bctx){
  size_t w = 0;
  char b64[4];
  if(bctx->srcidx){
    size_t copy = sizeof(bctx->src) - bctx->srcidx;
    // the unlikely event that we don't fill the bctx with our entire chunk...
    if(copy > osize){
      memcpy(bctx->src + bctx->srcidx, src, osize);
      bctx->srcidx += osize;
      return 0;
    }
    memcpy(bctx->src + bctx->srcidx, src, copy);
    base64x3(bctx->src, b64);
    bctx->srcidx = 0;
    if(fbuf_putn(f, b64, 4) != 4){
      return -1;
    }
    w = copy;
  }
  // the bctx is now guaranteed to be empty
  while(w + 3 <= osize){
    base64x3((const unsigned char*)src + w, b64);
    if(fbuf_putn(f, b64, 4) != 4){
      return -1;
    }
    w += 3;
  }
  // less than 3 remain; copy them into the bctx for further use
  if(w < osize){
    bctx->srcidx = osize - w;
    memcpy(bctx->src, src + w, bctx->srcidx);
  }
  return 1;
}

static size_t
fwrite_idats(fbuf* f, const unsigned char* data, size_t dlen,
             struct b64ctx* bctx){
  static const char ctype[] = "IDAT";
  uint32_t written = 0;
  uint32_t dwritten = 0;
  while(dlen){
    uint32_t thischunk = dlen;
    if(thischunk > CHUNK_MAX_DATA){
      thischunk = CHUNK_MAX_DATA;
    }
    uint32_t nclen = htonl(thischunk);
    if(fbuf_putn64(f, &nclen, 4, bctx) != 1 ||
       fbuf_putn64(f, ctype, 4, bctx) != 1 ||
       fbuf_putn64(f, data + dwritten, thischunk, bctx) != 1){
      return 0;
    }
// FIXME horrible; PoC; do not retain!
unsigned char* crcbuf = malloc(thischunk + 8);
memcpy(crcbuf, &nclen, 4);
memcpy(crcbuf + 4, ctype, 4);
memcpy(crcbuf + 8, data + dwritten, thischunk);
// END horribleness
    uint32_t crc = chunk_crc(crcbuf);
free(crcbuf); // FIXME well a bit more
    if(fbuf_putn64(f, &crc, 4, bctx) != 1){
      return 0;
    }
    dlen -= thischunk;
    dwritten += thischunk;
    written += CHUNK_DESC_BYTES + thischunk;
  }
  return written;
}

int write_png_b64(const void* data, int rows, int rowstride, int cols, fbuf* f){
  void* deflated;
  size_t dlen;
  compute_png_size(data, rows, rowstride, cols, &deflated, &dlen);
  if(deflated == NULL){
    return -1;
  }
  struct b64ctx bctx = { };
  if(fbuf_putn64(f, PNGHEADER, sizeof(PNGHEADER) - 1, &bctx) < 0){
    free(deflated);
    return -1;
  }
  unsigned char ihdr[25];
  write_ihdr(rows, cols, ihdr);
  if(fbuf_putn64(f, ihdr, sizeof(ihdr), &bctx) != 1){
    free(deflated);
    return -1;
  }
  if(fwrite_idats(f, deflated, dlen, &bctx) == 0){
    free(deflated);
    return -1;
  }
  free(deflated);
  if(fbuf_putn64(f, IEND, sizeof(IEND) - 1, &bctx) != 1){
    return -1;
  }
  if(bctx.srcidx){
    char b64[4];
    base64final(bctx.src, b64, bctx.srcidx);
    if(fbuf_putn(f, b64, 4) < 0){
      return -1;
    }
  }
  return 0;
}
