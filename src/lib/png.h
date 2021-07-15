#ifndef NOTCURSES_PNG
#define NOTCURSES_PNG

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/mman.h>

struct ncvisual;

void* create_png_mmap(const struct ncvisual* ncv, size_t* bsize, int fd);

#ifdef __cplusplus
}
#endif

#endif
