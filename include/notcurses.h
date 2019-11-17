#ifndef NOTCURSES_NOTCURSES
#define NOTCURSES_NOTCURSES

#ifdef __cplusplus
extern "C" {
#endif

const char* notcurses_version(void);

struct notcurses;

struct notcurses* notcurses_init(void);
int notcurses_stop(struct notcurses* nc);

int notcurses_term_dimensions(struct notcurses* n, int* rows, int* cols);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
