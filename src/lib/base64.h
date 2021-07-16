#ifndef NOTCURSES_PNG
#define NOTCURSES_PNG

#ifdef __cplusplus
extern "C" {
#endif

// lookup table for base64
static unsigned const char b64subs[] =
 "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// every 3 RGBA pixels (96 bits) become 16 base64-encoded bytes (128 bits). if
// there are only 2 pixels available, those 64 bits become 12 bytes. if there
// is only 1 pixel available, those 32 bits become 8 bytes. (pcount + 1) * 4
// bytes are used, plus a null terminator. we thus must receive 17.
static inline void
base64_rgba3(const uint32_t* pixels, size_t pcount, char* b64, bool wipe[static 3],
             uint32_t transcolor){
  uint32_t pixel = *pixels++;
  unsigned r = ncpixel_r(pixel);
  unsigned g = ncpixel_g(pixel);
  unsigned b = ncpixel_b(pixel);
  // we go ahead and take advantage of kitty's ability to reproduce 8-bit
  // alphas by copying it in directly, rather than mapping to {0, 255}.
  unsigned a = ncpixel_a(pixel);
  if(wipe[0] || rgba_trans_p(pixel, transcolor)){
    a = 0;
  }
  b64[0] = b64subs[(r & 0xfc) >> 2];
  b64[1] = b64subs[(r & 0x3 << 4) | ((g & 0xf0) >> 4)];
  b64[2] = b64subs[((g & 0xf) << 2) | ((b & 0xc0) >> 6)];
  b64[3] = b64subs[b & 0x3f];
  b64[4] = b64subs[(a & 0xfc) >> 2];
  if(pcount == 1){
    b64[5] = b64subs[(a & 0x3) << 4];
    b64[6] = '=';
    b64[7] = '=';
    b64[8] = '\0';
    return;
  }
  b64[5] = (a & 0x3) << 4;
  pixel = *pixels++;
  r = ncpixel_r(pixel);
  g = ncpixel_g(pixel);
  b = ncpixel_b(pixel);
  a = wipe[1] ? 0 : rgba_trans_p(pixel, transcolor) ? 0 : 255;
  b64[5] = b64subs[b64[5] | ((r & 0xf0) >> 4)];
  b64[6] = b64subs[((r & 0xf) << 2) | ((g & 0xc0) >> 6u)];
  b64[7] = b64subs[g & 0x3f];
  b64[8] = b64subs[(b & 0xfc) >> 2];
  b64[9] = b64subs[((b & 0x3) << 4) | ((a & 0xf0) >> 4)];
  if(pcount == 2){
    b64[10] = b64subs[(a & 0xf) << 2];
    b64[11] = '=';
    b64[12] = '\0';
    return;
  }
  b64[10] = (a & 0xf) << 2;
  pixel = *pixels;
  r = ncpixel_r(pixel);
  g = ncpixel_g(pixel);
  b = ncpixel_b(pixel);
  a = wipe[2] ? 0 : rgba_trans_p(pixel, transcolor) ? 0 : 255;
  b64[10] = b64subs[b64[10] | ((r & 0xc0) >> 6)];
  b64[11] = b64subs[r & 0x3f];
  b64[12] = b64subs[(g & 0xfc) >> 2];
  b64[13] = b64subs[((g & 0x3) << 4) | ((b & 0xf0) >> 4)];
  b64[14] = b64subs[((b & 0xf) << 2) | ((a & 0xc0) >> 6)];
  b64[15] = b64subs[a & 0x3f];
  b64[16] = '\0';
}

// convert 3 8-bit bytes into 4 base64-encoded characters
static inline void
base64x3(const unsigned char* src, char* b64){
  uint8_t a = src[0] >> 2u;
  uint8_t b = ((src[0] & 0x3u) << 4u) + ((src[1] & 0xf0u) >> 4u);
  uint8_t c = ((src[1] & 0x0fu) << 2u) + ((src[2] & 0xb0u) >> 6u);
  uint8_t d = src[2] & 0x3f;
  b64[0] = b64subs[a];
  b64[1] = b64subs[b];
  b64[2] = b64subs[c];
  b64[3] = b64subs[d];
}

// finalize a base64 stream with 3 or fewer 8-bit bytes
static inline void
base64final(const unsigned char* src, char* b64, size_t b){
  if(b == 3){
    base64x3(src, b64);
  }else if(b == 2){
    uint8_t s0 = src[0] >> 2u;
    uint8_t s1 = ((src[0] & 0x3u) << 4u) + ((src[1] & 0xf0u) >> 4u);
    uint8_t s2 = ((src[1] & 0x0fu) << 2u);
    b64[0] = b64subs[s0];
    b64[1] = b64subs[s1];
    b64[2] = b64subs[s2];
    b64[3] = '=';
  }else{ // b == 1
    uint8_t s0 = src[0] >> 2u;
    uint8_t s1 = (src[0] & 0x3u) << 4u;
    b64[0] = b64subs[s0];
    b64[1] = b64subs[s1];
    b64[2] = '=';
    b64[3] = '=';
  }
}

#ifdef __cplusplus
}
#endif

#endif
