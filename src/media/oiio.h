#ifndef MEDIA_OIIO
#define MEDIA_OIIO

#ifdef __cplusplus
extern "C" {
#endif

int oiio_decode(struct ncvisual* nc);
struct ncvisual_details* oiio_details_init(void);
void oiio_printbanner(const struct notcurses* nc);
int oiio_blit(struct ncvisual* ncv, int rows, int cols,
              struct ncplane* n, const struct blitset* bset,
              int placey, int placex, int begy, int begx,
              int leny, int lenx, unsigned blendcolors);
ncvisual* oiio_from_file(const char* filename);
void oiio_details_destroy(struct ncvisual_details* deets);
int oiio_decode_loop(ncvisual* ncv);
int oiio_resize(ncvisual* nc, int rows, int cols);
struct ncvisual* oiio_create(void);
int oiio_blit_dispatch(struct ncplane* nc, const struct blitset* bset,
                       int placey, int placex, int linesize,
                       const void* data, int begy, int begx,
                       int leny, int lenx, unsigned blendcolors);

#ifdef __cplusplus
}
#endif

#endif
