#ifndef NOTCURSES_NOTCURSES
#define NOTCURSES_NOTCURSES

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

const char* notcurses_version(void);

struct notcurses;

// Initialize a notcurses context. While a program can have more than one
// context, it usually doesn't make much sense, as they're all dealing with
// the same screen(s). Returns NULL on error, including any failure to
// initialize the terminfo library. If termtype is NULL, the environment
// variable TERM is used.
struct notcurses* notcurses_init(const char* termtype);

// Destroy a notcurses context.
int notcurses_stop(struct notcurses* nc);

// Make the physical screen match the virtual screen. Changes made to the
// virtual screen (i.e. most other calls) will not be visible until after a
// successful call to notcurses_render().
int notcurses_render(struct notcurses* nc);

// Refresh the object's concept of the terminal size, and return it via *rows
// and *cols. On error, -1 is returned, and the params are untouched.
int notcurses_term_dimensions(struct notcurses* n, int* rows, int* cols);

// Set the current color using RGB specifications. If the terminal does not
// support 16M colors (indicated by the "RGB" terminfo capability), the
// provided values will be interpreted in some fashion.
int notcurses_setrgb(struct notcurses* nc, uint32_t r, uint32_t g, uint32_t b);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
