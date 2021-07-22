#ifndef NOTCURSES_PNG
#define NOTCURSES_PNG

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

struct ncvisual;

// create the PNG, encode it using base64, and write it to |fp|
int write_png_b64(const void* data, int rows, int rowstride, int cols,
                  FILE* fp);

#ifdef __cplusplus
}
#endif

#endif
