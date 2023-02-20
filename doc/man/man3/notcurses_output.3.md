% notcurses_output(3)
% nick black <nickblack@linux.com>
% v3.0.9

# NAME

notcurses_output - output to ncplanes

# SYNOPSIS

**#include <notcurses/notcurses.h>**

**static inline int ncplane_putc(struct ncplane* ***n***, const nccell* ***c***);**

**int ncplane_putc_yx(struct ncplane* ***n***, int ***y***, int ***x***, const nccell* ***c***);**

**static inline int ncplane_putchar(struct ncplane* ***n***, char ***c***);**

**static inline int ncplane_putchar_yx(struct ncplane* ***n***, int ***y***, int ***x***, char ***c***);**

**int ncplane_putchar_stained(struct ncplane* ***n***, char ***c***);**

**static inline int ncplane_putwc(struct ncplane* ***n***, wchar_t ***w***);**

**int ncplane_putwc_yx(struct ncplane* ***n***, int ***y***, int ***x***, wchar_t ***w***);**

**static inline int ncplane_putwc_stained(struct ncplane* ***n***, wchar_t ***w***);**

**static inline int ncplane_putegc(struct ncplane* ***n***, const char* ***gclust***, int* ***sbytes***);**

**int ncplane_putegc_yx(struct ncplane* ***n***, int ***y***, int ***x***, const char* ***gclust***, int* ***sbytes***);**

**int ncplane_putegc_stained(struct ncplane* ***n***, const char* ***gclust***, int* ***sbytes***);**

**static inline int ncplane_putwegc(struct ncplane* ***n***, const wchar_t* ***gclust***, int* ***sbytes***);**

**static inline int ncplane_putwegc_yx(struct ncplane* ***n***, int ***y***, int ***x***, const wchar_t* ***gclust***, int* ***sbytes***);**

**int ncplane_putwegc_stained(struct ncplane* ***n***, const wchar_t* ***gclust***, int* ***sbytes***);**

**int ncplane_putstr_yx(struct ncplane* ***n***, int ***y***, int ***x***, const char* ***gclustarr***);**

**static inline int ncplane_putstr(struct ncplane* ***n***, const char* ***gclustarr***);**

**int ncplane_putstr_aligned(struct ncplane* ***n***, int ***y***, ncalign_e ***align***, const char* ***s***);**

**int ncplane_putstr_stained(struct ncplane* ***n***, const char* ***s***);**

**static inline int ncplane_putwstr(struct ncplane* ***n***, const wchar_t* ***gclustarr***);**

**int ncplane_putwstr_yx(struct ncplane* ***n***, int ***y***, int ***x***, const wchar_t* ***gclustarr***);**

**static inline int ncplane_putwstr_aligned(struct ncplane* ***n***, int ***y***, ncalign_e ***align***, const wchar_t* ***gclustarr***);**

**int ncplane_putwstr_stained(struct ncplane* ***n***, const wchar_t* ***gclustarr***);**

**static inline int ncplane_putnstr(struct ncplane* ***n***, size_t ***s***, const char* ***gclustarr***);**

**int ncplane_putnstr_yx(struct ncplane* ***n***, int ***y***, int ***x***, size_t ***s***, const char* ***gclusters***);**

**int ncplane_putnstr_aligned(struct ncplane* ***n***, int ***y***, ncalign_e ***align***, size_t ***s***, const char* ***s***);**

**int ncplane_vprintf_aligned(struct ncplane* ***n***, int ***y***, ncalign_e ***align***, const char* ***format***, va_list ***ap***);**

**int ncplane_vprintf_yx(struct ncplane* ***n***, int ***y***, int ***x***, const char* ***format***, va_list ***ap***);**

**static inline int ncplane_vprintf(struct ncplane* ***n***, const char* ***format***, va_list ***ap***);**

**static inline int ncplane_printf(struct ncplane* ***n***, const char* ***format***, ***...***);**

**static inline int ncplane_printf_yx(struct ncplane* ***n***, int ***y***, int ***x***, const char* ***format***, ***...***);**

**static inline int ncplane_printf_aligned(struct ncplane* ***n***, int ***y***, ncalign_e ***align***, const char* ***format***, ***...***);**

**int ncplane_cursor_move_yx(struct ncplane* ***n***, int ***y***, int ***x***);**

**int ncplane_puttext(struct ncplane* ***n***, int ***y***, ncalign_e ***align***, const char* ***text***, size_t* ***bytes***);**

# DESCRIPTION

These functions write EGCs (Extended Grapheme Clusters) to the specified
**struct ncplane**s. The following inputs are supported:

* **ncplane_putc()**: writes a single **nccell** (see **notcurses_cell(3)**)
* **ncplane_putchar()**: writes a single 7-bit ASCII character
* **ncplane_putwc()**: writes a single **wchar_t** (following UTF-8 conversion)
* **ncplane_putwegc()**: writes a single EGC from an array of **wchar_t**
* **ncplane_putegc()**: writes a single EGC from an array of UTF-8
* **ncplane_putstr()**: writes a set of EGCs from an array of UTF-8
* **ncplane_putwstr()**: writes a set of EGCs from an array of **wchar_t**
* **ncplane_vprintf()**: formatted output using **va_list**
* **ncplane_printf()**: formatted output using variadic arguments
* **ncplane_puttext()**: multi-line, line-broken, aligned text

All of these use the **ncplane**'s active styling, save **notcurses_putc()**,
which uses the **nccell**'s styling. Functions accepting a single EGC expect a series
of **wchar_t** terminated by **L'\0'** or a series of UTF-8 **char** terminated
by **'\0'**. The EGC must be well-formed, and must not contain any cluster
breaks. For more information, consult [Unicode® Standard Annex #29](https://unicode.org/reports/tr29/).
Functions accepting a set of EGCs must consist of a series of well-formed EGCs,
broken by cluster breaks, terminated by the appropriate NUL terminator.

Control characters are rejected, aside from two exceptions: a horizontal tab
(**'\\t'**, 0x09), and a newline (**'\\n'**, 0x0a) *when the output plane
is in scrolling mode*. A newline outside of scrolling mode will be rejected.
A tab will advance to the next tab stop, filling the space between with spaces.
Tab stops are separated by eight columns, and the first character of each line
is a tab stop.

These functions output to the **ncplane**'s current cursor location. Aside from
**ncplane_puttext()**, they *do not* move to the next line upon reaching the
right extreme of the containing plane. If the entirety of the content cannot be
output, they will output as much as possible.

Each of these base functions has two additional forms:

* **ncplane_putc_aligned()**, etc.: accepts a row and an alignment type
* **ncplane_putc_yx()**, etc.: accepts a row and column

If a positional parameter is -1, no movement will be made along that axis.
Passing **-1, -1** to e.g. **ncplane_putc_yx()** is equivalent to calling the
base form. These functions are implemented by moving the cursor, and then
performing the output. The two steps are *atomic* on success (it is not possible
for another caller to move the cursor between when it is positioned, and when
output begins), and thus these functions ought generally be preferred to an
explicit **ncplane_cursor_move_yx()**.

Upon successful return, the cursor will follow the last cell output.

# RETURN VALUES

**ncplane_cursor_move_yx()** returns -1 on error (invalid coordinate), or 0
on success.

For output functions, a negative return indicates an error with the inputs.
Otherwise, the number of *screen columns* output is returned. It is entirely
possible to get a short return, if there was insufficient room to output all
EGCs.

# SEE ALSO

**fprintf(3)**
**notcurses(3)**,
**notcurses_cell(3)**,
**notcurses_plane(3)**,
**stdarg(3)**,
**ascii(7)**,
**unicode(7)**,
**utf-8(7)**
