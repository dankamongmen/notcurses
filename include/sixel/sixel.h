#ifndef NOTCURSES_SIXEL_SIXEL
#define NOTCURSES_SIXEL_SIXEL

// a library built atop Notcurses suitable for working with sixels outside
// the Notcurses framework--a replacement for libsixel.

#ifdef __cplusplus
extern "C" {
#define RESTRICT
#else
#define RESTRICT restrict
#endif

#ifndef __MINGW64__
#define API __attribute__((visibility("default")))
#else
#define API __declspec(dllexport)
#endif
#define ALLOC __attribute__((malloc)) __attribute__((warn_unused_result))

struct sixel;
struct sixelctx;

API ALLOC struct sixelctx* libncsixel_init(void);

// load the file and encode the first frame as a sixel. if |colorregs| is 0,
// use the number supported by the terminal, or 256 if the terminal does not
// support sixel.
API ALLOC __attribute__ ((nonnull (1, 2)))
struct sixel* libncsixel_encode(struct sixelctx* sctx, const char* file,
                                unsigned colorregs);

API void libncsixel_stop(struct sixelctx* sctx);

#undef API
#undef ALLOC

#ifdef __cplusplus
} // extern "C"
#endif

#endif
