#ifndef NOTCURSES_DIRECT
#define NOTCURSES_DIRECT

#include <notcurses/notcurses.h>

#ifdef __cplusplus
extern "C" {
#endif

#define API __attribute__((visibility("default")))

// Initialize a direct-mode notcurses context on the connected terminal at 'fp'.
// 'fp' must be a tty. You'll usually want stdout. Direct mode supportes a
// limited subset of notcurses routines which directly affect 'fp', and neither
// supports nor requires notcurses_render(). This can be used to add color and
// styling to text in the standard output paradigm. Returns NULL on error,
// including any failure initializing terminfo.
API struct ncdirect* ncdirect_init(const char* termtype, FILE* fp);

// Direct mode. This API can be used to colorize and stylize output generated
// outside of notcurses, without ever calling notcurses_render(). These should
// not be intermixed with standard notcurses rendering.
API int ncdirect_fg(struct ncdirect* nc, unsigned rgb);
API int ncdirect_bg(struct ncdirect* nc, unsigned rgb);

API int ncdirect_fg_palindex(struct ncdirect* nc, int pidx);
API int ncdirect_bg_palindex(struct ncdirect* nc, int pidx);

// Returns the number of simultaneous colors claimed to be supported, or 1 if
// there is no color support. Note that several terminal emulators advertise
// more colors than they actually support, downsampling internally.
API int ncdirect_palette_size(const struct ncdirect* nc);

// Output the EGC |egc| according to the channels |channels|.
API int ncdirect_putc(struct ncdirect* nc, uint64_t channels, const char* egc);

static inline int
ncdirect_bg_rgb(struct ncdirect* nc, unsigned r, unsigned g, unsigned b){
  if(r > 255 || g > 255 || b > 255){
    return -1;
  }
  return ncdirect_bg(nc, (r << 16u) + (g << 8u) + b);
}

static inline int
ncdirect_fg_rgb(struct ncdirect* nc, unsigned r, unsigned g, unsigned b){
  if(r > 255 || g > 255 || b > 255){
    return -1;
  }
  return ncdirect_fg(nc, (r << 16u) + (g << 8u) + b);
}

API int ncdirect_fg_default(struct ncdirect* nc);
API int ncdirect_bg_default(struct ncdirect* nc);

// Get the current number of columns/rows.
API int ncdirect_dim_x(const struct ncdirect* nc);
API int ncdirect_dim_y(const struct ncdirect* nc);

// ncplane_styles_*() analogues
API int ncdirect_styles_set(struct ncdirect* n, unsigned stylebits);
API int ncdirect_styles_on(struct ncdirect* n, unsigned stylebits);
API int ncdirect_styles_off(struct ncdirect* n, unsigned stylebits);

// Move the cursor in direct mode. -1 to retain current location on that axis.
API int ncdirect_cursor_move_yx(struct ncdirect* n, int y, int x);
API int ncdirect_cursor_enable(struct ncdirect* nc);
API int ncdirect_cursor_disable(struct ncdirect* nc);
API int ncdirect_cursor_up(struct ncdirect* nc, int num);
API int ncdirect_cursor_left(struct ncdirect* nc, int num);
API int ncdirect_cursor_right(struct ncdirect* nc, int num);
API int ncdirect_cursor_down(struct ncdirect* nc, int num);

// Get the cursor position, when supported. This requires writing to the
// terminal, and then reading from it. If the terminal doesn't reply, or
// doesn't reply in a way we understand, the results might be deleterious.
API int ncdirect_cursor_yx(struct ncdirect* n, int* y, int* x);

// Push or pop the cursor location to the terminal's stack. The depth of this
// stack, and indeed its existence, is terminal-dependent.
API int ncdirect_cursor_push(struct ncdirect* n);
API int ncdirect_cursor_pop(struct ncdirect* n);

// Formatted printing (plus alignment relative to the terminal).
API int ncdirect_printf_aligned(struct ncdirect* n, int y, ncalign_e align,
                                const char* fmt, ...)
  __attribute__ ((format (printf, 4, 5)));

// Display an image using the specified blitter and scaling. The image may
// // be arbitrarily many rows -- the output will scroll -- but will only occupy
// // the column of the cursor, and those to the right.
API nc_err_e ncdirect_render_image(struct ncdirect* n, const char* filename,
                                   ncalign_e align, ncblitter_e blitter,
                                   ncscale_e scale);

// Clear the screen.
API int ncdirect_clear(struct ncdirect* nc);

// Release 'nc' and any associated resources. 0 on success, non-0 on failure.
API int ncdirect_stop(struct ncdirect* nc);

#undef API

#ifdef __cplusplus
}
#endif

#endif
