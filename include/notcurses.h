#ifndef NOTCURSES_NOTCURSES
#define NOTCURSES_NOTCURSES

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

const char* notcurses_version(void);

struct ncplane;   // a drawable notcurses surface
struct notcurses; // notcurses state for a given terminal

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
  // use of the "alternate screen". This flag inhibits use of smcup/rmcup.
  bool inhibit_alternate_screen;
} notcurses_options;

// Initialize a notcurses context, corresponding to a connected terminal.
// Returns NULL on error, including any failure to initialize terminfo.
struct notcurses* notcurses_init(const notcurses_options* opts);

// Destroy a notcurses context.
int notcurses_stop(struct notcurses* nc);

// Get a reference to the standard plane (one matching our current idea of the
// terminal size) for this terminal. Invalidated following a call to
// notcurses_resize().
struct ncplane* notcurses_stdplane(struct notcurses* nc);
const struct ncplane* notcurses_stdplane_const(const struct notcurses* nc);

// Make the physical screen match the virtual screen. Changes made to the
// virtual screen (i.e. most other calls) will not be visible until after a
// successful call to notcurses_render().
int notcurses_render(struct notcurses* nc);

// Returns the dimensions of this ncplane.
void ncplane_dimyx(const struct ncplane* n, int* rows, int* cols);

// Return our current idea of the terminal dimensions in rows and cols.
static inline void
notcurses_term_dimyx(const struct notcurses* n, int* rows, int* cols){
  ncplane_dimyx(notcurses_stdplane_const(n), rows, cols);
}

// Refresh our idea of the terminal's dimensions, reshaping the standard plane
// if necessary. Without a call to this function following a terminal resize
// (as signaled via SIGWINCH), notcurses_render() might not function properly.
// Following a call to notcurses_resize(), any references to the standard plane
// ought be considered invalidated.
int notcurses_resize(struct notcurses* n);

// Move the cursor to the specified position (the cursor needn't be visible).
// Returns -1 on error, including negative parameters, or ones exceeding the
// plane's dimensions.
int ncplane_movyx(struct ncplane* n, int y, int x);

// Get the current position of the cursor within n. y and/or x may be NULL.
void ncplane_posyx(const struct ncplane* n, int* y, int* x);

// Set the current cell in the specified plane to the provided wchar_t array.
// The array must not be more than one column worth of wchar_t's, among other
// restrictions. Advances the cursor by one cell.
int ncplane_putwc(struct ncplane* n, const wchar_t* wcs);

// Set the current fore/background color using RGB specifications. If the
// terminal does not support directly-specified 3x8b cells (24-bit "Direct
// Color", indicated by the "RGB" terminfo capability), the provided values
// will be interpreted in some lossy fashion. None of r, g, or b may exceed 255.
// "HP-like" terminals require setting foreground and background at the same
// time using "color pairs"; notcurses will manage color pairs transparently.
int ncplane_fg_rgb8(struct ncplane* n, int r, int g, int b);
int ncplane_bg_rgb8(struct ncplane* n, int r, int g, int b);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
