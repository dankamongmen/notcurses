#ifndef MEDIA_OIIO
#define MEDIA_OIIO

#ifdef __cplusplus
extern "C" {
#endif

#include "lib/internal.h"

int oiio_decode(ncvisual* nc);
struct ncvisual_details* oiio_details_init(void);
void oiio_printbanner(fbuf* f);
void oiio_details_seed(struct ncvisual* ncv);
int oiio_blit(const ncvisual* ncv, unsigned rows, unsigned cols,
              struct ncplane* n, const struct blitset* bset,
              const blitterargs* bargs);
ncvisual* oiio_from_file(const char* filename);
int oiio_decode_loop(ncvisual* ncv);
int oiio_resize(ncvisual* nc, unsigned rows, unsigned cols);
ncvisual* oiio_create(void);
void oiio_destroy(ncvisual* ncv);
int oiio_blit_dispatch(struct ncplane* nc, const struct blitset* bset,
                       int linesize, const void* data,
                       int leny, int lenx, const blitterargs* bargs);
int oiio_init(int logl);

extern ncvisual_implementation local_visual_implementation;

#ifdef __cplusplus
}
#endif

#endif
