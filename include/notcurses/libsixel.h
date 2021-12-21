#ifndef NOTCURSES_LIBSIXEL
#define NOTCURSES_LIBSIXEL

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

struct sixel;
struct sixelctx;

struct sixelctx* libncsixel_init(void);

struct sixel* libncsixel_encode(struct sixelctx* sctx, const char* file, unsigned colorregs);

uint32_t* libncsixel_explode(const struct sixel* s);

void libncsixel_stop(struct sixelctx* sctx);

#ifdef __cplusplus
}
#endif

#endif
