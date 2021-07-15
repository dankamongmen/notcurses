#include <sys/mman.h>
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
// |ncv|. if alphap is non-zero, an alpha channel will be used, increasing the
// size of the data by 1/3.
size_t compute_png_size(const ncvisual* ncv, unsigned alphap){
  uint64_t databytes = ncv->pixx * ncv->pixy * (3 + !!alphap);
  uint64_t fullchunks = databytes / CHUNK_MAX_DATA; // full 2GB IDATs
  return (sizeof(PNGHEADER) - 1) +  // PNG header
         CHUNK_DESC_BYTES + IHDR_DATA_BYTES + // mandatory IHDR chunk
         (CHUNK_DESC_BYTES + CHUNK_MAX_DATA) * fullchunks +
         (CHUNK_DESC_BYTES + databytes % CHUNK_MAX_DATA) +
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
write_ihdr(const ncvisual* ncv, char* buf, unsigned alphap){
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
  uint8_t color = 2 + alphap ? 4 : 0; // RGB with possible alpha
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

// write 1+ IDAT chunks at |buf|.
static size_t
write_idats(const ncvisual* ncv, char* buf, unsigned alphap){
  uint32_t written = 0;
  // FIXME
  return written;
}

// write the constant 12B IEND chunk at |buf|. it contains no data.
static size_t
write_iend(char* buf){
  static const char iend[] = "\x00\x00\x00\x00IEND\xae\x42\x60\x82";
  memcpy(buf, iend, CHUNK_DESC_BYTES);
  return CHUNK_DESC_BYTES;
}

// write a PNG at the provided buffer using the ncvisual
int create_png(const ncvisual* ncv, void* buf, size_t* bsize, unsigned alphap){
  size_t totalsize = compute_png_size(ncv, alphap);
  if(*bsize < totalsize){
    logerror("%zuB buffer too small for %zuB PNG\n", *bsize, totalsize);
    return -1;
  }
  *bsize = totalsize;
  size_t written = sizeof(PNGHEADER) - 1;
  memcpy(buf, PNGHEADER, written);
  size_t r = write_ihdr(ncv, (char*)buf + written, alphap);
  written += r;
  r = write_idats(ncv, (char*)buf + written, alphap);
  written += r;
  r = write_iend((char*)buf + written);
  written += r;
  if(written != *bsize){
    logwarn("PNG was %zuB, not %zuB\n", written, *bsize);
  }
  return 0;
}

static inline size_t
mmap_round_size(size_t s){
  const size_t pgsize = 4096; // FIXME get page size, round up s
  return (s + pgsize - 1) / pgsize * pgsize;
}

// write a PNG, creating the buffer ourselves. it must be munmapped. the
// resulting length is written to *bsize on success. returns MMAP_FAILED
// on a failure. if |fname| is NULL, an anonymous map will be made.
void* create_png_mmap(const ncvisual* ncv, size_t* bsize, const char* fname){
  const unsigned alphap = 1; // FIXME 0 if no alpha used, for smaller output
  *bsize = compute_png_size(ncv, alphap);
  *bsize = mmap_round_size(*bsize);
  if(*bsize == 0){
    return MAP_FAILED;
  }
  // FIXME open and ftruncate file if fname is set
  // FIXME hugetlb?
  void* map = mmap(NULL, *bsize, PROT_WRITE | PROT_READ,
                   MAP_SHARED_VALIDATE |
                   (fname ? 0 : MAP_ANONYMOUS), -1, 0);
  if(map == MAP_FAILED){
    logerror("Couldn't get %zuB map\n", *bsize);
    return MAP_FAILED;
  }
  if(create_png(ncv, map, bsize, alphap)){
    munmap(map, *bsize);
    return MAP_FAILED;
  }
  return map;
}
