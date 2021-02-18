% notcurses_direct(3)
% nick black <nickblack@linux.com>
% v2.2.2

# NAME

notcurses_direct - minimal notcurses instances for styling text

# SYNOPSIS

```c
#include <notcurses/direct.h>

#define NCDIRECT_OPTION_INHIBIT_SETLOCALE   0x0001ull
#define NCDIRECT_OPTION_INHIBIT_CBREAK      0x0002ull
#define NCDIRECT_OPTION_NO_QUIT_SIGHANDLERS 0x0008ull
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

**int ncdirect_dim_x(const struct ncdirect* ***nc***);**

**int ncdirect_dim_y(const struct ncdirect* ***nc***);**

**int ncdirect_styles_set(struct ncdirect* ***n***, unsigned ***stylebits***);**

**int ncdirect_styles_on(struct ncdirect* ***n***, unsigned ***stylebits***);**

**int ncdirect_styles_off(struct ncdirect* ***n***, unsigned ***stylebits***);**

**int ncdirect_clear(struct ncdirect* ***nc***)**

**int ncdirect_stop(struct ncdirect* ***nc***);**

**int ncdirect_cursor_move_yx(struct ncdirect* ***n***, int ***y***, int ***x***);**

**int ncdirect_cursor_enable(struct ncdirect* ***nc***);**

**int ncdirect_cursor_disable(struct ncdirect* ***nc***);**

**int ncdirect_cursor_up(struct ncdirect* ***nc***, int ***num***);**

**int ncdirect_cursor_left(struct ncdirect* ***nc***, int ***num***);**

**int ncdirect_cursor_right(struct ncdirect* ***nc***, int ***num***);**

**int ncdirect_cursor_down(struct ncdirect* ***nc***, int ***num***);**

**int ncdirect_putstr(struct ncdirect* ***nc***, uint64_t ***channels***, const char* ***utf8***);**

**int ncdirect_printf_aligned(struct ncdirect* ***n***, int ***y***, ncalign_e ***align***, const char* ***fmt***, ***...***);**

**bool ncdirect_canopen_images(const struct ncdirect* ***n***);**

**bool ncdirect_canutf8(const struct ncdirect* ***n***);**

**int ncdirect_hline_interp(struct ncdirect* ***n***, const char* ***egc***, int ***len***, uint64_t ***h1***, uint64_t ***h2***);**

**int ncdirect_vline_interp(struct ncdirect* ***n***, const char* ***egc***, int ***len***, uint64_t ***h1***, uint64_t ***h2***);**

**int ncdirect_box(struct ncdirect* ***n***, uint64_t ***ul***, uint64_t ***ur***, uint64_t ***ll***, uint64_t ***lr***, const wchar_t* ***wchars***, int ***ylen***, int ***xlen***, unsigned ***ctlword***);**

**int ncdirect_rounded_box(struct ncdirect* ***n***, uint64_t ***ul***, uint64_t ***ur***, uint64_t ***ll***, uint64_t ***lr***, int ***ylen***, int ***xlen***, unsigned ***ctlword***);**

**int ncdirect_double_box(struct ncdirect* ***n***, uint64_t ***ul***, uint64_t ***ur***, uint64_t ***ll***, uint64_t ***lr***, int ***ylen***, int ***xlen***, unsigned ***ctlword***);**

**int ncdirect_render_image(struct ncdirect* ***n***, const char* ***filename***, ncblitter_e ***blitter***, ncscale_e ***scale***);**

**char* ncdirect_readline(struct ncdirect* ***n***, const char* ***prompt***);**

# DESCRIPTION

**ncdirect_init** prepares the **FILE** provided as **fp** for colorizing and
styling. On success, a pointer to a valid **struct ncdirect** is returned.
**NULL** is returned on failure. Before the process exits, **ncdirect_stop**
should be called to reset the terminal and free up resources. **ncdirect_init**
places the terminal into "cbreak" (also known as "rare") mode, disabling
line-buffering and echo of input. **ncdirect_stop** restores the terminal state
as it was when the corresponding **ncdirect_init** call was made.

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

* **NCDIRECT_OPTION_NO_QUIT_SIGHANDLERS**: A signal handler will usually be
    installed for **SIGINT**, **SIGQUIT**, **SIGSEGV**, **SIGTERM**, and
    **SIGABRT**, cleaning up the terminal on such exceptions. With this flag,
    the handler will not be installed.

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

**ncdirect_readline** uses the Readline library to read a (heap-allocated)
line of arbitrary length, supporting line-editing controls. For more
information, consult **readline(3)**. If you want input echoed to the
terminal while using **ncdirect_readline**, **NCDIRECT_OPTION_INHIBIT_CBREAK**
must be supplied to **ncdirect_init**.

# RETURN VALUES

**ncdirect_init** returns **NULL** on failure. Otherwise, the return value
points to a valid **struct ncdirect**, which can be used until it is provided
to **ncdirect_stop**.

**ncdirect_putstr** and **ncdirect_printf_aligned** return the number of bytes
written on success. On failure, they return some negative number.

All other functions return 0 on success, and non-zero on error.

# SEE ALSO

**getenv(3)**,
**readline(3)**
**notcurses(3)**,
**notcurses_plane(3)**,
**terminfo(5)**,
**termios(3)**
