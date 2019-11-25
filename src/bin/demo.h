#ifndef NOTCURSES_DEMO
#define NOTCURSES_DEMO

#include <notcurses.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FADE_MILLISECONDS 500

int widecolor_demo(struct notcurses* nc, struct ncplane* n);
int box_demo(struct notcurses* nc);
int grid_demo(struct notcurses* nc);

#ifdef __cplusplus
}
#endif

#endif
