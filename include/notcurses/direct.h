#ifndef NOTCURSES_DIRECT
#define NOTCURSES_DIRECT

#include <notcurses/notcurses.h>

#ifdef __cplusplus
extern "C" {
#endif

#define API __attribute__((visibility("default")))
#define ALLOC __attribute__((malloc)) __attribute__((warn_unused_result))

// ncdirect_init() will call setlocale() to inspect the current locale. If
// that locale is "C" or "POSIX", it will call setlocale(LC_ALL, "") to set
// the locale according to the LANG environment variable. Ideally, this will
// result in UTF8 being enabled, even if the client app didn't call
// setlocale() itself. Unless you're certain that you're invoking setlocale()
// prior to notcurses_init(), you should not set this bit. Even if you are
// invoking setlocale(), this behavior shouldn't be an issue unless you're
// doing something weird (setting a locale not based on LANG).
#define NCDIRECT_OPTION_INHIBIT_SETLOCALE   0x0001ull

// *Don't* place the terminal into cbreak mode (see tcgetattr(3)). By default,
// echo and input's line buffering are turned off.
#define NCDIRECT_OPTION_INHIBIT_CBREAK      0x0002ull

// We typically install a signal handler for SIG{INT, SEGV, ABRT, QUIT} that
// restores the screen, and then calls the old signal handler. Set to inhibit
// registration of these signal handlers. Chosen to match fullscreen mode.
#define NCDIRECT_OPTION_NO_QUIT_SIGHANDLERS 0x0008ull

// Initialize a direct-mode Notcurses context on the connected terminal at 'fp'.
// 'fp' must be a tty. You'll usually want stdout. Direct mode supports a
// limited subset of Notcurses routines which directly affect 'fp', and neither
// supports nor requires notcurses_render(). This can be used to add color and
// styling to text in the standard output paradigm. 'flags' is a bitmask over
// NCDIRECT_OPTION_*.
// Returns NULL on error, including any failure initializing terminfo.
API ALLOC struct ncdirect* ncdirect_init(const char* termtype, FILE* fp, uint64_t flags);

// The same as ncdirect_init(), but without any multimedia functionality,
// allowing for a svelter binary. Link with notcurses-core if this is used.
API ALLOC struct ncdirect* ncdirect_core_init(const char* termtype, FILE* fp, uint64_t flags);

// Read a (heap-allocated) line of text using the Readline library Initializes
// Readline the first time it's called. For input to be echoed to the terminal,
// it is necessary that NCDIRECT_OPTION_INHIBIT_CBREAK be provided to
// ncdirect_init(). Returns NULL on error.
__attribute__ ((nonnull (1)))
API ALLOC char* ncdirect_readline(struct ncdirect* nc, const char* prompt);

// Direct mode. This API can be used to colorize and stylize output generated
// outside of notcurses, without ever calling notcurses_render(). These should
// not be intermixed with standard Notcurses rendering.
API int ncdirect_set_fg_rgb(struct ncdirect* nc, unsigned rgb)
  __attribute__ ((nonnull (1)));
API int ncdirect_set_bg_rgb(struct ncdirect* nc, unsigned rgb)
  __attribute__ ((nonnull (1)));

static inline int ncdirect_fg_rgb(struct ncdirect* nc, unsigned rgb)
  __attribute__ ((deprecated));

static inline int
ncdirect_fg_rgb(struct ncdirect* nc, unsigned rgb){
  return ncdirect_set_fg_rgb(nc, rgb);
}

static inline int ncdirect_bg_rgb(struct ncdirect* nc, unsigned rgb)
  __attribute__ ((deprecated));

static inline int
ncdirect_bg_rgb(struct ncdirect* nc, unsigned rgb){
  return ncdirect_set_bg_rgb(nc, rgb);
}

API int ncdirect_set_fg_palindex(struct ncdirect* nc, int pidx)
  __attribute__ ((nonnull (1)));
API int ncdirect_set_bg_palindex(struct ncdirect* nc, int pidx)
  __attribute__ ((nonnull (1)));

static inline int ncdirect_fg_palindex(struct ncdirect* nc, int pidx)
  __attribute__ ((deprecated));

static inline int
ncdirect_fg_palindex(struct ncdirect* nc, int pidx){
  return ncdirect_set_fg_palindex(nc, pidx);
}

static inline int ncdirect_bg_palindex(struct ncdirect* nc, int pidx)
  __attribute__ ((deprecated));

static inline int
ncdirect_bg_palindex(struct ncdirect* nc, int pidx){
  return ncdirect_set_bg_palindex(nc, pidx);
}

// Returns the number of simultaneous colors claimed to be supported, or 1 if
// there is no color support. Note that several terminal emulators advertise
// more colors than they actually support, downsampling internally.
API unsigned ncdirect_palette_size(const struct ncdirect* nc)
  __attribute__ ((nonnull (1)));

// Output the string |utf8| according to the channels |channels|. Note that
// ncdirect_putstr() does not explicitly flush output buffers, so it will not
// necessarily be immediately visible.
API int ncdirect_putstr(struct ncdirect* nc, uint64_t channels, const char* utf8)
  __attribute__ ((nonnull (1, 3)));

// Formatted printing (plus alignment relative to the terminal). Returns the
// number of columns printed on success.
API int ncdirect_printf_aligned(struct ncdirect* n, int y, ncalign_e align,
                                const char* fmt, ...)
  __attribute__ ((nonnull (1, 4))) __attribute__ ((format (printf, 4, 5)));

// Force a flush. Returns 0 on success, -1 on failure.
API int ncdirect_flush(const struct ncdirect* nc)
  __attribute__ ((nonnull (1)));

static inline int
ncdirect_set_bg_rgb8(struct ncdirect* nc, unsigned r, unsigned g, unsigned b){
  if(r > 255 || g > 255 || b > 255){
    return -1;
  }
  return ncdirect_set_bg_rgb(nc, (r << 16u) + (g << 8u) + b);
}

static inline int
ncdirect_set_fg_rgb8(struct ncdirect* nc, unsigned r, unsigned g, unsigned b){
  if(r > 255 || g > 255 || b > 255){
    return -1;
  }
  return ncdirect_set_fg_rgb(nc, (r << 16u) + (g << 8u) + b);
}

static inline int
ncdirect_fg_rgb8(struct ncdirect* nc, unsigned r, unsigned g, unsigned b)
  __attribute__ ((deprecated));

static inline int
ncdirect_fg_rgb8(struct ncdirect* nc, unsigned r, unsigned g, unsigned b){
  return ncdirect_set_fg_rgb8(nc, r, g, b);
}

static inline int
ncdirect_bg_rgb8(struct ncdirect* nc, unsigned r, unsigned g, unsigned b)
  __attribute__ ((deprecated));

static inline int
ncdirect_bg_rgb8(struct ncdirect* nc, unsigned r, unsigned g, unsigned b){
  return ncdirect_set_bg_rgb8(nc, r, g, b);
}

API int ncdirect_set_fg_default(struct ncdirect* nc)
  __attribute__ ((nonnull (1)));
API int ncdirect_set_bg_default(struct ncdirect* nc)
  __attribute__ ((nonnull (1)));

static inline int ncdirect_fg_default(struct ncdirect* nc)
  __attribute__ ((deprecated));

static inline int
ncdirect_fg_default(struct ncdirect* nc){
  return ncdirect_set_fg_default(nc);
}

static inline int ncdirect_bg_default(struct ncdirect* nc)
  __attribute__ ((deprecated));

static inline int
ncdirect_bg_default(struct ncdirect* nc){
  return ncdirect_set_bg_default(nc);
}

// Get the current number of columns/rows.
API int ncdirect_dim_x(const struct ncdirect* nc)
  __attribute__ ((nonnull (1)));
API int ncdirect_dim_y(const struct ncdirect* nc)
  __attribute__ ((nonnull (1)));

// ncplane_styles_*() analogues
API int ncdirect_set_styles(struct ncdirect* n, unsigned stylebits)
  __attribute__ ((nonnull (1)));
API int ncdirect_on_styles(struct ncdirect* n, unsigned stylebits)
  __attribute__ ((nonnull (1)));
API int ncdirect_off_styles(struct ncdirect* n, unsigned stylebits)
  __attribute__ ((nonnull (1)));

// Deprecated forms of above.
API int ncdirect_styles_set(struct ncdirect* n, unsigned stylebits)
  __attribute__ ((deprecated));
API int ncdirect_styles_on(struct ncdirect* n, unsigned stylebits)
  __attribute__ ((deprecated));
API int ncdirect_styles_off(struct ncdirect* n, unsigned stylebits)
  __attribute__ ((deprecated));

// Move the cursor in direct mode. -1 to retain current location on that axis.
API int ncdirect_cursor_move_yx(struct ncdirect* n, int y, int x)
  __attribute__ ((nonnull (1)));
API int ncdirect_cursor_enable(struct ncdirect* nc)
  __attribute__ ((nonnull (1)));
API int ncdirect_cursor_disable(struct ncdirect* nc)
  __attribute__ ((nonnull (1)));
API int ncdirect_cursor_up(struct ncdirect* nc, int num)
  __attribute__ ((nonnull (1)));
API int ncdirect_cursor_left(struct ncdirect* nc, int num)
  __attribute__ ((nonnull (1)));
API int ncdirect_cursor_right(struct ncdirect* nc, int num)
  __attribute__ ((nonnull (1)));
API int ncdirect_cursor_down(struct ncdirect* nc, int num)
  __attribute__ ((nonnull (1)));

// Get the cursor position, when supported. This requires writing to the
// terminal, and then reading from it. If the terminal doesn't reply, or
// doesn't reply in a way we understand, the results might be deleterious.
API int ncdirect_cursor_yx(struct ncdirect* n, int* y, int* x)
  __attribute__ ((nonnull (1)));

// Push or pop the cursor location to the terminal's stack. The depth of this
// stack, and indeed its existence, is terminal-dependent.
API int ncdirect_cursor_push(struct ncdirect* n)
  __attribute__ ((nonnull (1)));

API int ncdirect_cursor_pop(struct ncdirect* n)
  __attribute__ ((nonnull (1)));

// Clear the screen.
API int ncdirect_clear(struct ncdirect* nc)
  __attribute__ ((nonnull (1)));

// Can we load images? This requires being built against FFmpeg/OIIO.
API bool ncdirect_canopen_images(const struct ncdirect* n)
  __attribute__ ((nonnull (1)));

// Is our encoding UTF-8? Requires LANG being set to a UTF8 locale.
API bool ncdirect_canutf8(const struct ncdirect* n)
  __attribute__ ((nonnull (1)));

// This function must successfully return before NCBLIT_PIXEL is available.
// Returns -1 on error, 0 for no support, or 1 if pixel output is supported.
// Must not be called concurrently with either input or rasterization.
API int ncdirect_check_pixel_support(struct ncdirect* n)
  __attribute__ ((nonnull (1)));

// Draw horizontal/vertical lines using the specified channels, interpolating
// between them as we go. The EGC may not use more than one column. For a
// horizontal line, |len| cannot exceed the screen width minus the cursor's
// offset. For a vertical line, it may be as long as you'd like; the screen
// will scroll as necessary. All lines start at the current cursor position.
API int ncdirect_hline_interp(struct ncdirect* n, const char* egc, int len,
                              uint64_t h1, uint64_t h2)
  __attribute__ ((nonnull (1)));

API int ncdirect_vline_interp(struct ncdirect* n, const char* egc, int len,
                              uint64_t h1, uint64_t h2)
  __attribute__ ((nonnull (1)));

// Draw a box with its upper-left corner at the current cursor position, having
// dimensions |ylen|x|xlen|. See ncplane_box() for more information. The
// minimum box size is 2x2, and it cannot be drawn off-screen. |wchars| is an
// array of 6 wide characters: UL, UR, LL, LR, HL, VL.
API int ncdirect_box(struct ncdirect* n, uint64_t ul, uint64_t ur,
                     uint64_t ll, uint64_t lr, const wchar_t* wchars,
                     int ylen, int xlen, unsigned ctlword)
  __attribute__ ((nonnull (1)));

// ncdirect_box() with the rounded box-drawing characters
API int ncdirect_rounded_box(struct ncdirect* n, uint64_t ul, uint64_t ur,
                             uint64_t ll, uint64_t lr,
                             int ylen, int xlen, unsigned ctlword)
  __attribute__ ((nonnull (1)));

// ncdirect_box() with the double box-drawing characters
API int ncdirect_double_box(struct ncdirect* n, uint64_t ul, uint64_t ur,
                            uint64_t ll, uint64_t lr,
                            int ylen, int xlen, unsigned ctlword)
  __attribute__ ((nonnull (1)));

// See ppoll(2) for more detail. Provide a NULL 'ts' to block at length, a 'ts'
// of 0 for non-blocking operation, and otherwise a timespec to bound blocking.
// Signals in sigmask (less several we handle internally) will be atomically
// masked and unmasked per ppoll(2). '*sigmask' should generally contain all
// signals. Returns a single Unicode code point, or (char32_t)-1 on error.
// 'sigmask' may be NULL. Returns 0 on a timeout. If an event is processed, the
// return value is the 'id' field from that event. 'ni' may be NULL.
API char32_t ncdirect_getc(struct ncdirect* n, const struct timespec* ts,
                           sigset_t* sigmask, ncinput* ni)
  __attribute__ ((nonnull (1)));

// Get a file descriptor suitable for input event poll()ing. When this
// descriptor becomes available, you can call ncdirect_getc_nblock(),
// and input ought be ready. This file descriptor is *not* necessarily
// the file descriptor associated with stdin (but it might be!).
API int ncdirect_inputready_fd(struct ncdirect* n)
  __attribute__ ((nonnull (1)));

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

typedef struct ncplane ncdirectv;
typedef struct ncvisual ncdirectf;

// FIXME this ought be used in the rendered mode API as well; it's currently
// only used by direct mode. describes all geometries of an ncvisual--both those
// which are inherent, and those in a given rendering regime. pixy and pixx are
// the true internal pixel geometry, taken directly from the load (and updated
// by ncvisual_resize()). cdimy/cdimx are the cell pixel geometry *at the time
// of this call* (it can change with a font change, in which case all values
// other than pixy/pixx are invalidated). rpixy/rpixx are the pixel geometry as
// handed to the blitter, following any scaling. scaley/scalex are the number
// of input pixels drawn to full cell; when using NCBLIT_PIXEL, they are
// equivalent to cdimy/cdimx. rcelly/rcellx are the cell geometry as written by
// the blitter, following any padding (there is padding whenever rpix{y, x} is
// not evenly divided by scale{y, x}, and also sometimes for Sixel).
// maxpixely/maxpixelx are defined only when NCBLIT_PIXEL is used, and specify
// the largest bitmap that the terminal is willing to accept.
typedef struct ncvgeom {
  int pixy, pixx;     // true pixel geometry of ncvisual data
  int cdimy, cdimx;   // terminal cell geometry when this was calculated
  int rpixy, rpixx;   // rendered pixel geometry
  int rcelly, rcellx; // rendered cell geometry
  int scaley, scalex; // pixels per filled cell
  // only defined for NCBLIT_PIXEL
  int maxpixely, maxpixelx;
} ncvgeom;

// Display an image using the specified blitter and scaling. The image may
// be arbitrarily many rows -- the output will scroll -- but will only occupy
// the column of the cursor, and those to the right. The render/raster process
// can be split by using ncdirect_render_frame() and ncdirect_raster_frame().
API int ncdirect_render_image(struct ncdirect* n, const char* filename,
                              ncalign_e align, ncblitter_e blitter,
                              ncscale_e scale)
  __attribute__ ((nonnull (1, 2)));

// Render an image using the specified blitter and scaling, but do not write
// the result. The image may be arbitrarily many rows -- the output will scroll
// -- but will only occupy the column of the cursor, and those to the right.
// To actually write (and free) this, invoke ncdirect_raster_frame(). 'maxx'
// and 'maxy', if greater than 0, are used for scaling; the terminal's geometry
// is otherwise used.
API ALLOC ncdirectv* ncdirect_render_frame(struct ncdirect* n, const char* filename,
                                           ncblitter_e blitter, ncscale_e scale,
                                           int maxy, int maxx)
  __attribute__ ((nonnull (1, 2)));

// Takes the result of ncdirect_render_frame() and writes it to the output,
// freeing it on all paths.
API int ncdirect_raster_frame(struct ncdirect* n, ncdirectv* ncdv, ncalign_e align)
  __attribute__ ((nonnull (1, 2)));

// Load media from disk, but do not yet render it (presumably because you want
// to get its geometry via ncdirect_geom_frame(), or to use the same file with
// ncdirect_render_loaded_frame() multiple times). You must destroy the result
// with ncdirectf_free();
API ALLOC ncdirectf* ncdirectf_from_file(struct ncdirect* n, const char* filename)
  __attribute__ ((nonnull (1, 2)));

// Free a ncdirectf returned from ncdirectf_from_file().
API void ncdirectf_free(ncdirectf* frame);

// Same as ncdirect_render_frame(), except 'frame' must already have been
// loaded. A loaded frame may be rendered in different ways before it is
// destroyed.
API ALLOC ncdirectv* ncdirectf_render(struct ncdirect* n, ncdirectf* frame,
                                      ncblitter_e blitter, ncscale_e scale,
                                      int maxy, int maxx)
  __attribute__ ((nonnull (1, 2)));

// Having loaded the frame 'frame', get the geometry of a potential render.
API int ncdirectf_geom(struct ncdirect* n, ncdirectf* frame,
                       ncblitter_e* blitter, ncscale_e scale,
                       int maxy, int maxx, ncvgeom* geom)
  __attribute__ ((nonnull (1, 2)));

// Load successive frames from a file, invoking 'streamer' on each.
API int ncdirect_stream(struct ncdirect* n, const char* filename, ncstreamcb streamer,
                        struct ncvisual_options* vopts, void* curry)
  __attribute__ ((nonnull (1, 2)));

#undef ALLOC
#undef API

#ifdef __cplusplus
}
#endif

#endif
