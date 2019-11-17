#ifndef NOTCURSES_NOTCURSES
#define NOTCURSES_NOTCURSES

#ifdef __cplusplus
extern "C" {
#endif

const char* notcurses_version(void);

int notcurses_init(void);
int notcurses_stop(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
