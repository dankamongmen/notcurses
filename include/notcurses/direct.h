#ifndef NOTCURSES_DIRECT
#define NOTCURSES_DIRECT

#include <notcurses/notcurses.h>

#ifdef __cplusplus
extern "C" {
#endif

#define API __attribute__((visibility("default")))

// ncdirect_init() will call setlocale() to inspect the current locale. If
// that locale is "C" or "POSIX", it will call setlocale(LC_ALL, "") to set
// the locale according to the LANG environment variable. Ideally, this will
// result in UTF8 being enabled, even if the client app didn't call
// setlocale() itself. Unless you're certain that you're invoking setlocale()
// prior to notcurses_init(), you should not set this bit. Even if you are
// invoking setlocale(), this behavior shouldn't be an issue unless you're
// doing something weird (setting a locale not based on LANG).
#define NCDIRECT_OPTION_INHIBIT_SETLOCALE 0x0001ull

// *Don't* place the terminal into cbreak mode (see tcgetattr(3)). By default,
// echo and input's line buffering are turned off.
#define NCDIRECT_OPTION_INHIBIT_CBREAK    0x0002ull

// Initialize a direct-mode Notcurses context on the connected terminal at 'fp'.
// 'fp' must be a tty. You'll usually want stdout. Direct mode supports a
// limited subset of Notcurses routines which directly affect 'fp', and neither
// supports nor requires notcurses_render(). This can be used to add color and
// styling to text in the standard output paradigm. 'flags' is a bitmask over
// NCDIRECT_OPTION_*.
// Returns NULL on error, including any failure initializing terminfo.
API struct ncdirect* ncdirect_init(const char* termtype, FILE* fp, uint64_t flags);

// Direct mode. This API can be used to colorize and stylize output generated
// outside of notcurses, without ever calling notcurses_render(). These should
// not be intermixed with standard Notcurses rendering.
API int ncdirect_fg_rgb(struct ncdirect* nc, unsigned rgb);
API int ncdirect_bg_rgb(struct ncdirect* nc, unsigned rgb);

API int ncdirect_fg_palindex(struct ncdirect* nc, int pidx);
API int ncdirect_bg_palindex(struct ncdirect* nc, int pidx);

// Returns the number of simultaneous colors claimed to be supported, or 1 if
// there is no color support. Note that several terminal emulators advertise
// more colors than they actually support, downsampling internally.
API unsigned ncdirect_palette_size(const struct ncdirect* nc);

// Output the string |utf8| according to the channels |channels|. Note that
// ncdirect_putstr() does not explicitly flush output buffers, so it will not
// necessarily be immediately visible.
API int ncdirect_putstr(struct ncdirect* nc, uint64_t channels, const char* utf8);

// Formatted printing (plus alignment relative to the terminal). Returns the
// number of columns printed on success.
API int ncdirect_printf_aligned(struct ncdirect* n, int y, ncalign_e align,
                                const char* fmt, ...)
  __attribute__ ((format (printf, 4, 5)));

// Force a flush. Returns 0 on success, -1 on failure.
API int ncdirect_flush(const struct ncdirect* nc);

static inline int
ncdirect_bg_rgb8(struct ncdirect* nc, unsigned r, unsigned g, unsigned b){
  if(r > 255 || g > 255 || b > 255){
    return -1;
  }
  return ncdirect_bg_rgb(nc, (r << 16u) + (g << 8u) + b);
}

static inline int
ncdirect_fg_rgb8(struct ncdirect* nc, unsigned r, unsigned g, unsigned b){
  if(r > 255 || g > 255 || b > 255){
    return -1;
  }
  return ncdirect_fg_rgb(nc, (r << 16u) + (g << 8u) + b);
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

// Display an image using the specified blitter and scaling. The image may
// be arbitrarily many rows -- the output will scroll -- but will only occupy
// the column of the cursor, and those to the right.
API int ncdirect_render_image(struct ncdirect* n, const char* filename,
                              ncalign_e align, ncblitter_e blitter,
                              ncscale_e scale);

// Display an image using the specified blitter and scaling. The image may
// be arbitrarily many rows -- the output will scroll -- but will only occupy
// the column of the cursor, and those to the right.
API struct ncplane* ncdirect_render_frame(struct ncdirect* n, const char* filename,
                                          ncblitter_e blitter, ncscale_e scale);

API int ncdirect_raster_frame(struct ncdirect* n, struct ncplane* faken,
                              ncalign_e align, ncblitter_e blitter,
                              ncscale_e scale);

// Clear the screen.
API int ncdirect_clear(struct ncdirect* nc);

// Can we load images? This requires being built against FFmpeg/OIIO.
API bool ncdirect_canopen_images(const struct ncdirect* n);

// Is our encoding UTF-8? Requires LANG being set to a UTF8 locale.
API bool ncdirect_canutf8(const struct ncdirect* n);

// Draw horizontal/vertical lines using the specified channels, interpolating
// between them as we go. The EGC may not use more than one column. For a
// horizontal line, |len| cannot exceed the screen width minus the cursor's
// offset. For a vertical line, it may be as long as you'd like; the screen
// will scroll as necessary. All lines start at the current cursor position.
API int ncdirect_hline_interp(struct ncdirect* n, const char* egc, int len,
                              uint64_t h1, uint64_t h2);
API int ncdirect_vline_interp(struct ncdirect* n, const char* egc, int len,
                              uint64_t h1, uint64_t h2);

// Draw a box with its upper-left corner at the current cursor position, having
// dimensions |ylen|x|xlen|. See ncplane_box() for more information. The
// minimum box size is 2x2, and it cannot be drawn off-screen. |wchars| is an
// array of 6 wide characters: UL, UR, LL, LR, HL, VL.
API int ncdirect_box(struct ncdirect* n, uint64_t ul, uint64_t ur,
                     uint64_t ll, uint64_t lr, const wchar_t* wchars,
                     int ylen, int xlen, unsigned ctlword);

// ncdirect_box() with the rounded box-drawing characters
API int ncdirect_rounded_box(struct ncdirect* n, uint64_t ul, uint64_t ur,
                             uint64_t ll, uint64_t lr,
                             int ylen, int xlen, unsigned ctlword);

// ncdirect_box() with the double box-drawing characters
API int ncdirect_double_box(struct ncdirect* n, uint64_t ul, uint64_t ur,
                            uint64_t ll, uint64_t lr,
                            int ylen, int xlen, unsigned ctlword);

// See ppoll(2) for more detail. Provide a NULL 'ts' to block at length, a 'ts'
// of 0 for non-blocking operation, and otherwise a timespec to bound blocking.
// Signals in sigmask (less several we handle internally) will be atomically
// masked and unmasked per ppoll(2). '*sigmask' should generally contain all
// signals. Returns a single Unicode code point, or (char32_t)-1 on error.
// 'sigmask' may be NULL. Returns 0 on a timeout. If an event is processed, the
// return value is the 'id' field from that event. 'ni' may be NULL.
API char32_t ncdirect_getc(struct ncdirect* n, const struct timespec* ts,
                           sigset_t* sigmask, ncinput* ni);

// Get a file descriptor suitable for input event poll()ing. When this
// descriptor becomes available, you can call ncdirect_getc_nblock(),
// and input ought be ready. This file descriptor is *not* necessarily
// the file descriptor associated with stdin (but it might be!).
API int ncdirect_inputready_fd(struct ncdirect* n);

// 'ni' may be NULL if the caller is uninterested in event details. If no event
// is ready, returns 0.
static inline char32_t
ncdirect_getc_nblock(struct ncdirect* n, ncinput* ni){
  sigset_t sigmask;
  sigfillset(&sigmask);
  struct timespec ts = { .tv_sec = 0, .tv_nsec = 0 };
  return ncdirect_getc(n, &ts, &sigmask, ni);
}

// 'ni' may be NULL if the caller is uninterested in event details. Blocks
// until an event is processed or a signal is received.
static inline char32_t
ncdirect_getc_blocking(struct ncdirect* n, ncinput* ni){
  sigset_t sigmask;
  sigemptyset(&sigmask);
  return ncdirect_getc(n, NULL, &sigmask, ni);
}

// Release 'nc' and any associated resources. 0 on success, non-0 on failure.
API int ncdirect_stop(struct ncdirect* nc);

#undef API

#ifdef __cplusplus
}
#endif

#endif
