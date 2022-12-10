% notcurses_direct(3)
% nick black <nickblack@linux.com>
% v3.0.9

# NAME

notcurses_direct - the Direct Mode API

# SYNOPSIS

```c
#include <notcurses/direct.h>

#define NCDIRECT_OPTION_INHIBIT_SETLOCALE   0x0001ull
#define NCDIRECT_OPTION_INHIBIT_CBREAK      0x0002ull
#define NCDIRECT_OPTION_NO_QUIT_SIGHANDLERS 0x0008ull
#define NCDIRECT_OPTION_VERBOSE             0x0010ull
#define NCDIRECT_OPTION_VERY_VERBOSE        0x0020ull
```

**struct ncdirect* ncdirect_init(const char* ***termtype***, FILE* ***fp***, uint64_t ***flags***);**

**unsigned ncdirect_palette_size(const struct ncdirect* ***nc***);**

**int ncdirect_set_bg_rgb8(struct ncdirect* ***nc***, unsigned ***r***, unsigned ***g***, unsigned ***b***);**

**int ncdirect_set_fg_rgb8(struct ncdirect* ***nc***, unsigned ***r***, unsigned ***g***, unsigned ***b***);**

**int ncdirect_set_fg_rgb(struct ncdirect* ***nc***, unsigned ***rgb***);**

**int ncdirect_set_bg_rgb(struct ncdirect* ***nc***, unsigned ***rgb***);**

**int ncdirect_set_fg_default(struct ncdirect* ***nc***);**

**int ncdirect_set_bg_default(struct ncdirect* ***nc***);**

**int ncdirect_set_fg_palindex(struct ncdirect* ***nc***, int ***pidx***);**

**int ncdirect_set_bg_palindex(struct ncdirect* ***nc***, int ***pidx***);**

**unsigned ncdirect_dim_x(const struct ncdirect* ***nc***);**

**unsigned ncdirect_dim_y(const struct ncdirect* ***nc***);**

**unsigned ncdirect_supported_styles(const struct ncdirect* ***nc***);**

**int ncdirect_styles_set(struct ncdirect* ***n***, unsigned ***stylebits***);**

**int ncdirect_styles_on(struct ncdirect* ***n***, unsigned ***stylebits***);**

**int ncdirect_styles_off(struct ncdirect* ***n***, unsigned ***stylebits***);**

**unsigned ncdirect_styles(struct ncdirect* ***n***);**

**int ncdirect_clear(struct ncdirect* ***nc***)**

**int ncdirect_stop(struct ncdirect* ***nc***);**

**int ncdirect_cursor_move_yx(struct ncdirect* ***n***, int ***y***, int ***x***);**

**int ncdirect_cursor_yx(struct ncdirect* ***n***, unsigned* ***y***, unsigned* ***x***);**

**int ncdirect_cursor_enable(struct ncdirect* ***nc***);**

**int ncdirect_cursor_disable(struct ncdirect* ***nc***);**

**int ncdirect_cursor_up(struct ncdirect* ***nc***, int ***num***);**

**int ncdirect_cursor_left(struct ncdirect* ***nc***, int ***num***);**

**int ncdirect_cursor_right(struct ncdirect* ***nc***, int ***num***);**

**int ncdirect_cursor_down(struct ncdirect* ***nc***, int ***num***);**

**int ncdirect_putstr(struct ncdirect* ***nc***, uint64_t ***channels***, const char* ***utf8***);**

**int ncdirect_putegc(struct ncdirect* ***nc***, uint64_t ***channels***, const char* ***utf8***, int* ***sbytes***);**

**int ncdirect_printf_aligned(struct ncdirect* ***n***, int ***y***, ncalign_e ***align***, const char* ***fmt***, ***...***);**

**const char* ncdirect_detected_terminal(const struct ncdirect* ***n***);**

**int ncdirect_hline_interp(struct ncdirect* ***n***, const char* ***egc***, unsigned ***len***, uint64_t ***h1***, uint64_t ***h2***);**

**int ncdirect_vline_interp(struct ncdirect* ***n***, const char* ***egc***, unsigned ***len***, uint64_t ***h1***, uint64_t ***h2***);**

**int ncdirect_box(struct ncdirect* ***n***, uint64_t ***ul***, uint64_t ***ur***, uint64_t ***ll***, uint64_t ***lr***, const wchar_t* ***wchars***, int ***ylen***, int ***xlen***, unsigned ***ctlword***);**

**int ncdirect_rounded_box(struct ncdirect* ***n***, uint64_t ***ul***, uint64_t ***ur***, uint64_t ***ll***, uint64_t ***lr***, int ***ylen***, int ***xlen***, unsigned ***ctlword***);**

**int ncdirect_double_box(struct ncdirect* ***n***, uint64_t ***ul***, uint64_t ***ur***, uint64_t ***ll***, uint64_t ***lr***, int ***ylen***, int ***xlen***, unsigned ***ctlword***);**

**ncdirectv* ncdirect_render_frame(struct ncdirect* ***n***, const char* ***filename***, ncblitter_e ***blitter***, ncscale_e ***scale***, int ***maxy***, int ***maxx***);**

**char* ncdirect_readline(struct ncdirect* ***n***, const char* ***prompt***);**

**bool ncdirect_cantruecolor(const struct ncdirect* ***nc***);**

**bool ncdirect_canchangecolor(const struct ncdirect* ***nc***);**

**bool ncdirect_canfade(const struct ncdirect* ***nc***);**

**bool ncdirect_canopen_images(const struct ncdirect* ***nc***);**

**bool ncdirect_canopen_videos(const struct ncdirect* ***nc***);**

**bool ncdirect_canutf8(const struct ncdirect* ***nc***);**

**int ncdirect_check_pixel_support(const struct ncdirect* ***nc***);**

**bool ncdirect_canhalfblock(const struct ncdirect* ***nc***);**

**bool ncdirect_canquadrant(const struct ncdirect* ***nc***);**

**bool ncdirect_cansextant(const struct ncdirect* ***nc***);**

**bool ncdirect_canbraille(const struct ncdirect* ***nc***);**

**bool ncdirect_canget_cursor(const struct ncdirect* ***nc***);**

**uint32_t ncdirect_get(struct ncdirect* ***n***, const struct timespec* ***absdl***, ncinput* ***ni***);**

```c
typedef struct ncvgeom {
  int pixy, pixx;     // true pixel geometry of ncvisual data
  int cdimy, cdimx;   // terminal cell geometry when this was calculated
  int rpixy, rpixx;   // rendered pixel geometry
  int rcelly, rcellx; // rendered cell geometry
  int scaley, scalex; // pixels per filled cell
  // only defined for NCBLIT_PIXEL
  int maxpixely, maxpixelx;
} ncvgeom;
```

**int ncdirect_render_image(struct ncdirect* ***n***, const char* ***filename***, ncblitter_e ***blitter***, ncscale_e ***scale***);**

**int ncdirect_raster_frame(struct ncdirect* ***n***, ncdirectv* ***ncdv***, ncalign_e ***align***);**

**int ncdirect_stream(struct ncdirect* ***n***, const char* ***filename***, ncstreamcb ***streamer***, struct ncvisual_options* ***vopts***, void* ***curry***);**

**int ncdirect_raster_frame(struct ncdirect* ***n***, ncdirectv* ***ncdv***, ncalign_e ***align***);**

**struct ncdirectf* ncdirectf_from_file(struct ncdirect* ***n***, const char* ***filename***);***

**void ncdirectf_free(struct ncdirectf* ***frame***);**

**ncdirectv* ncdirectf_render(struct ncdirect* ***n***, struct ncdirectf* ***frame***, const struct ncvisual_options ***vopts***);**

**int ncdirectf_geom(struct ncdirect* ***n***, struct ncdirectf* ***frame***, const struct ncvisual_options ***vopts***, ncvgeom* ***geom***);**


# DESCRIPTION

**ncdirect_init** prepares the **FILE** provided as **fp** for colorizing and
styling. On success, a pointer to a valid **struct ncdirect** is returned.
**NULL** is returned on failure. Before the process exits, **ncdirect_stop**
should be called to reset the terminal and free up resources. **ncdirect_init**
places the terminal into "cbreak" (also known as "rare") mode, disabling
line-buffering and echo of input. **ncdirect_stop** restores the terminal state
as it was when the corresponding **ncdirect_init** call was made. A process
can have only one context active at once.

The following flags are defined:

* **NCDIRECT_OPTION_INHIBIT_SETLOCALE**: Unless this flag is set,
    **ncdirect_init** will call **setlocale(LC_ALL, NULL)**. If the result is
    either "**C**" or "**POSIX**", it will print a diagnostic to **stderr**, and
    then call **setlocale(LC_ALL, "").** This will attempt to set the locale
    based off the **LANG** environment variable. Your program should call
    **setlocale(3)** itself, usually as one of the first lines.

* **NCDIRECT_OPTION_INHIBIT_CBREAK**: Unless this flag is set, **ncdirect_init**
    will place the terminal into cbreak mode (i.e. disabling echo and line
    buffering; see **tcgetattr(3)**).

* **NCDIRECT_OPTION_DRAIN_INPUT**: Standard input may be freely discarded. If
    you do not intend to process input, pass this flag. Otherwise, input can
    buffer up, eventually preventing Notcurses from processing terminal
    messages. It will furthermore avoid wasting time processing useless input.

* **NCDIRECT_OPTION_NO_QUIT_SIGHANDLERS**: A signal handler will usually be
    installed for **SIGABRT**, **SIGBUS**, **SIGFPE**, **SIGILL**, **SIGINT**,
    **SIGQUIT**, **SIGSEGV**, and **SIGTERM**, cleaning up the terminal on
    such exceptions. With this flag, the handler will not be installed.

* **NCDIRECT_OPTION_VERBOSE**: Enable diagnostics to **stderr** at the level of
    **NCLOGLEVEL_WARNING**.

* **NCDIRECT_OPTION_VERY_VERBOSE**: Enable all diagnostics (equivalent to
    **NCLOGLEVEL_TRACE**). Implies **NCDIRECT_OPTION_VERBOSE**.

The loglevel can also be set externally using the **NOTCURSES_LOGLEVEL**
environment variable. See **notcurses_init(3)** for more information.

An appropriate **terminfo(5)** entry must exist for the terminal. This entry is
usually selected using the value of the **TERM** environment variable (see
**getenv(3)**), but a non-**NULL** value for **termtype** will override this. An
invalid terminfo specification can lead to reduced performance, reduced
display capabilities, and/or display errors. notcurses natively targets
24bpp/8bpc RGB color, and it is thus desirable to use a terminal with the
**rgb** capability (e.g. xterm's **xterm-direct**).

**ncdirect_dim_x** returns the current number of columns, and **ncdirect_dim_y**
the current number of rows.

**ncdirect_clear** clears the screen using a control code if one exists in
terminfo. Otherwise, it prints successive newlines to scroll everything off.

**ncdirect_cursor_move_yx** moves the cursor to the specified coordinate. -1 can
be specified for either **y** or **x** to leave that axis unchanged.

**ncdirect_enable_cursor** and **ncdirect_disable_cursor** always flush the
output stream, taking effect immediately.

**ncdirect_cursor_up** and friends all move relative to the current position.
Attempting to e.g. move up while on the top row will return 0, but have no
effect.

**ncdirect_readline** reads a (heap-allocated) line of arbitrary length,
supporting some libreadline-style line-editing controls.
**NCDIRECT_OPTION_INHIBIT_CBREAK** should not be used if you intend to
use **ncdirect_readline**; if used, line-editing keybindings cannot be
implemented. Input will be echoed whether this option is used or not.

**ncdirect_check_pixel_support** must be called (and successfully return)
before **NCBLIT_PIXEL** can be used to render images; see
**notcurses_visual(3)** for more details.

When rendering an image, ***maxy*** and ***maxx*** specify a maximum number
of (cell) rows and columns to use, respectively. Passing 0 means "use as much
space as is necessary". It is an error to pass a negative number for either.

# RETURN VALUES

**ncdirect_init** returns **NULL** on failure. Otherwise, the return value
points to a valid **struct ncdirect**, which can be used until it is provided
to **ncdirect_stop**.

**ncdirect_printf_aligned** returns the number of bytes written on success. On
failure, it returns some negative number.

**ncdirect_putstr** returns a nonnegative number on success, and **EOF**
on failure.

**ncdirect_putegc** returns the number of columns consumed on success, or -1
on failure. If ***sbytes*** is not **NULL**, the number of bytes consumed
will be written to it.

**ncdirect_check_pixel_support** returns -1 on error, 0 if there is no pixel
support, and 1 if pixel support is successfully detected.

**ncdirect_styles** returns the current styling, a bitmask over the various
**NCSTYLE_** constants.

All other functions return 0 on success, and non-zero on error.

# NOTES

You are recommended to accept **-v** and **-vv** as command-line options,
mapping them to **NCDIRECT_OPTION_VERBOSE** and
**NCDIRECT_OPTION_VERY_VERBOSE** respectively.

# SEE ALSO

**getenv(3)**,
**notcurses(3)**,
**notcurses_init(3)**,
**notcurses_plane(3)**,
**notcurses_visual(3)**,
**terminfo(5)**,
**termios(3)**
