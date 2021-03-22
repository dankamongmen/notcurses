#ifndef MEDIA_OIIO
#define MEDIA_OIIO

#ifdef __cplusplus
extern "C" {
#endif

#include "internal.h"

int oiio_decode(ncvisual* nc);
struct ncvisual_details* oiio_details_init(void);
void oiio_printbanner(const struct notcurses* nc);
int oiio_blit(ncvisual* ncv, int rows, int cols,
              struct ncplane* n, const struct blitset* bset,
              int leny, int lenx, const blitterargs* bargs);
ncvisual* oiio_from_file(const char* filename);
int oiio_decode_loop(ncvisual* ncv);
int oiio_resize(ncvisual* nc, int rows, int cols);
ncvisual* oiio_create(void);
void oiio_destroy(ncvisual* ncv);
int oiio_blit_dispatch(struct ncplane* nc, const struct blitset* bset,
                       int linesize, const void* data,
                       int leny, int lenx, const blitterargs* bargs);

#ifdef __cplusplus
}
#endif

#endif
