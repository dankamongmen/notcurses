#ifndef NOTCURSES_SIXEL_SIXEL
#define NOTCURSES_SIXEL_SIXEL

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

// FIXME libsixel-like API

#undef API
#undef ALLOC

#ifdef __cplusplus
} // extern "C"
#endif

#endif
