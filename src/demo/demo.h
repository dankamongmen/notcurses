#ifndef NOTCURSES_DEMO
#define NOTCURSES_DEMO

#include <time.h>
#include <notcurses.h>

#ifdef __cplusplus
extern "C" {
#endif

// configured via command line option -- the base number of ns between demos
extern struct timespec demodelay;

#define FADE_MILLISECONDS 500

int unicodeblocks_demo(struct notcurses* nc);
int widecolor_demo(struct notcurses* nc);
int box_demo(struct notcurses* nc);
int maxcolor_demo(struct notcurses* nc);
int grid_demo(struct notcurses* nc);
int sliding_puzzle_demo(struct notcurses* nc);

#ifdef __cplusplus
}
#endif

#endif
