% ncdirect_init(3)
% nick black <nickblack@linux.com>
% v1.6.10

# NAME

ncdirect_init - minimal notcurses instances for styling text

# SYNOPSIS

**#include <notcurses/direct.h>**

**struct ncdirect* ncdirect_init(const char* termtype, FILE* fp);**

**int ncdirect_palette_size(const struct ncdirect* nc);**

**int ncdirect_bg_rgb8(struct ncdirect* nc, unsigned r, unsigned g, unsigned b);**

**int ncdirect_fg_rgb8(struct ncdirect* nc, unsigned r, unsigned g, unsigned b);**

**int ncdirect_fg(struct ncdirect* nc, unsigned rgb);**

**int ncdirect_bg(struct ncdirect* nc, unsigned rgb);**

**int ncdirect_fg_default(struct ncdirect* nc);**

**int ncdirect_bg_default(struct ncdirect* nc);**

**int ncdirect_dim_x(const struct ncdirect* nc);**

**int ncdirect_dim_y(const struct ncdirect* nc);**

**int ncdirect_styles_set(struct ncdirect* n, unsigned stylebits);**

**int ncdirect_styles_on(struct ncdirect* n, unsigned stylebits);**

**int ncdirect_styles_off(struct ncdirect* n, unsigned stylebits);**

**int ncdirect_clear(struct ncdirect* nc)**

**int ncdirect_stop(struct ncdirect* nc);**

**int ncdirect_cursor_move_yx(struct ncdirect* n, int y, int x);**

**int ncdirect_cursor_enable(struct ncdirect* nc);**

**int ncdirect_cursor_disable(struct ncdirect* nc);**

**int ncdirect_cursor_up(struct ncdirect* nc, int num);**

**int ncdirect_cursor_left(struct ncdirect* nc, int num);**

**int ncdirect_cursor_right(struct ncdirect* nc, int num);**

**int ncdirect_cursor_down(struct ncdirect* nc, int num);**

**int ncdirect_putstr(struct ncdirect* nc, uint64_t channels, const char* utf8);**

**bool ncdirect_canopen_images(const struct ncdirect* n);**

**bool ncdirect_canutf8(const struct ncdirect* n);**

**int ncdirect_hline_interp(struct ncdirect* n, const char* egc, int len, uint64_t h1, uint64_t h2);**

**int ncdirect_vline_interp(struct ncdirect* n, const char* egc, int len, uint64_t h1, uint64_t h2);**

**int ncdirect_box(struct ncdirect* n, uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr, const wchar_t* wchars, int ylen, int xlen, unsigned ctlword);**

**int ncdirect_rounded_box(struct ncdirect* n, uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr, int ylen, int xlen, unsigned ctlword);**

**int ncdirect_double_box(struct ncdirect* n, uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr, int ylen, int xlen, unsigned ctlword);**

**nc_err_e ncdirect_render_image(struct ncdirect* n, const char* filename, ncblitter_e blitter, ncscale_e scale);**

# DESCRIPTION

**ncdirect_init** prepares the **FILE** provided as **fp** (which must
be attached to a terminal) for colorizing and styling. On success, a pointer to
a valid **struct ncdirect** is returned. **NULL** is returned on failure.
Before the process exits, **ncdirect_stop(3)** should be called to reset the
terminal and free up resources.

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

# RETURN VALUES

**ncdirect_init** returns **NULL** on failure. Otherwise, the return value
points to a valid **struct ncdirect**, which can be used until it is provided
to **ncdirect_stop**.

All other functions return 0 on success, and non-zero on error.

# SEE ALSO

**getenv(3)**,
**termios(3)**,
**notcurses(3)**,
**notcurses_plane(3)**,
**terminfo(5)**
