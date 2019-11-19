#ifndef NOTCURSES_NOTCURSES
#define NOTCURSES_NOTCURSES

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

const char* notcurses_version(void);

struct notcurses;

// Configuration for notcurses_init().
typedef struct notcurses_options {
  // The name of the terminfo database entry describing this terminal. If NULL,
  // the environment variable TERM is used. Failure to open the terminal
  // definition will result in failure to initialize notcurses.
  const char* termtype;
  // A file descriptor for this terminal on which we will generate output.
  // Must be a valid file descriptor attached to a terminal, or notcurses will
  // refuse to start. You'll usually want STDOUT_FILENO.
  int outfd;
  // If smcup/rmcup capabilities are indicated, notcurses defaults to making
  // use of the "alternate screen." This flag inhibits use of smcup/rmcup.
  bool inhibit_alternate_screen;
} notcurses_options;

// Initialize a notcurses context. While a program can have more than one
// context, it usually doesn't make much sense, as they're all dealing with
// the same screen(s). Returns NULL on error, including any failure to
// initialize the terminfo library. If termtype is NULL, the environment
// variable TERM is used.
struct notcurses* notcurses_init(const notcurses_options* opts);

// Destroy a notcurses context.
int notcurses_stop(struct notcurses* nc);

// Make the physical screen match the virtual screen. Changes made to the
// virtual screen (i.e. most other calls) will not be visible until after a
// successful call to notcurses_render().
int notcurses_render(struct notcurses* nc);

// Refresh the object's concept of the terminal size, and return it via *rows
// and *cols. On error, -1 is returned, and the params are untouched.
int notcurses_term_dimensions(struct notcurses* n, int* rows, int* cols);

// Move the cursor to the specified position (the cursor needn't be visible).
// Returns -1 on error, including negative parameters, or ones exceeding the
// screen dimentsion.
int notcurses_move(struct notcurses* n, int x, int y);

// Set the current fore/background color using RGB specifications. If the
// terminal does not support directly-specified 3x8b cells (24-bit "Direct
// Color", indicated by the "RGB" terminfo capability), the provided values
// will be interpreted in some lossy fashion. None of r, g, or b may exceed 255.
// "HP-like" terminals require setting foreground and background at the same
// time using "color pairs"; notcurses will manage color pairs transparently.
int notcurses_fg_rgb8(struct notcurses* nc, unsigned r, unsigned g, unsigned b);
int notcurses_bg_rgb8(struct notcurses* nc, unsigned r, unsigned g, unsigned b);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
