#ifndef NOTCURSES_BANNER
#define NOTCURSES_BANNER

#ifdef __cplusplus
extern "C" {
#endif

struct fbuf;
struct notcurses;

int init_banner(const struct notcurses* nc, struct fbuf* f);

#ifdef __cplusplus
}
#endif

#endif
