#ifndef NOTCURSES_NOTCURSES
#define NOTCURSES_NOTCURSES

#include <time.h>
#include <uchar.h>
#include <ctype.h>
#include <wchar.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <limits.h>
#include <stdbool.h>
#include <notcurses/nckeys.h>
#include <notcurses/ncerrs.h>

#ifdef __cplusplus
extern "C" {
#define RESTRICT
#else
#define RESTRICT restrict
#endif

#define API __attribute__((visibility("default")))

// Get a human-readable string describing the running notcurses version.
API const char* notcurses_version(void);

struct notcurses; // notcurses state for a given terminal, composed of ncplanes
struct ncplane;   // a drawable notcurses surface, composed of cells
struct cell;      // a coordinate on an ncplane: an EGC plus styling
struct ncvisual;  // a visual bit of multimedia opened with LibAV|OIIO
struct ncuplot;   // a histogram, bound to a plane (uint64_ts)
struct ncdplot;   // a histogram, bound to a plane (non-negative doubles)
struct ncfdplane; // i/o wrapper to dump file descriptor to plane
struct ncsubproc; // ncfdplane wrapper with subprocess management
struct ncselector;// widget supporting selecting 1 from a list of options
struct ncmultiselector; // widget supporting selecting 0..n from n options
struct ncreader;  // widget supporting free string input ala readline
struct ncfadectx; // context for a palette fade operation

// each has the empty cell in addition to the product of its dimensions. i.e.
// NCBLIT_1x1 has two states: empty and full block. NCBLIT_1x1x4 has five
// states: empty, the three shaded blocks, and the full block.
typedef enum {
  NCBLIT_DEFAULT, // let the ncvisual pick
  NCBLIT_1x1,     // full block                █
  NCBLIT_2x1,     // full/(upper|left) blocks  ▄█
  NCBLIT_1x1x4,   // shaded full blocks        ▓▒░█
  NCBLIT_2x2,     // quadrants                 ▗▐ ▖▄▟▌▙█
  NCBLIT_4x1,     // four vert/horz levels     █▆▄▂ / ▎▌▊█
  NCBLIT_BRAILLE, // 4 rows, 2 cols (braille)  ⡀⡄⡆⡇⢀⣀⣄⣆⣇⢠⣠⣤⣦⣧⢰⣰⣴⣶⣷⢸⣸⣼⣾⣿
  NCBLIT_8x1,     // eight vert/horz levels    █▇▆▅▄▃▂▁ / ▏▎▍▌▋▊▉█
  NCBLIT_SIXEL,   // 6 rows, 1 col (RGB), spotty support among terminals
} ncblitter_e;

// Alignment within a plane or terminal. Left/right-justified, or centered.
typedef enum {
  NCALIGN_LEFT,
  NCALIGN_CENTER,
  NCALIGN_RIGHT,
} ncalign_e;

// How to scale an ncvisual during rendering. NCSCALE_NONE will apply no
// scaling. NCSCALE_SCALE scales a visual to the plane's size, maintaining
// aspect ratio. NCSCALE_STRETCH stretches and scales the image in an
// attempt to fill the entirety of the plane.
typedef enum {
  NCSCALE_NONE,
  NCSCALE_SCALE,
  NCSCALE_STRETCH,
} ncscale_e;

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

// Returns the number of columns occupied by a multibyte (UTF-8) string, or
// -1 if a non-printable/illegal character is encountered.
static inline int
mbswidth(const char* mbs){
  int offset = 0; // offset into mbs
  int cols = 0;   // number of columns consumed thus far
  mbstate_t ps;
  memset(&ps, 0, sizeof(ps));
  do{
    wchar_t wcs;
    size_t r = mbrtowc(&wcs, mbs + offset, SIZE_MAX, &ps);
    if(r == (size_t)-1 || r == (size_t)-2){
      return -1;
    }
    int w = wcwidth(wcs);
    if(w < 0){
      return -1;
    }
    cols += w;
    offset += r;
  }while(mbs[offset]);
  return cols;
}

// extract these bits to get a channel's alpha value
#define NCCHANNEL_ALPHA_MASK    0x30000000ull
// background cannot be highcontrast, only foreground
#define CELL_ALPHA_HIGHCONTRAST 0x30000000ull
#define CELL_ALPHA_TRANSPARENT  0x20000000ull
#define CELL_ALPHA_BLEND        0x10000000ull
#define CELL_ALPHA_OPAQUE       0x00000000ull

// if this bit is set, the cell is part of a multicolumn glyph. whether a
// cell is the left or right side of the glyph can be determined by checking
// whether ->gcluster is zero.
#define CELL_WIDEASIAN_MASK     0x8000000000000000ull
// if this bit is set, we are *not* using the default background color
#define CELL_BGDEFAULT_MASK     0x0000000040000000ull
// if this bit is set, we are *not* using the default foreground color
#define CELL_FGDEFAULT_MASK     (CELL_BGDEFAULT_MASK << 32u)
// extract these bits to get the background RGB value
#define CELL_BG_RGB_MASK        0x0000000000ffffffull
// extract these bits to get the foreground RGB value
#define CELL_FG_RGB_MASK        (CELL_BG_RGB_MASK << 32u)
// if this bit *and* CELL_BGDEFAULT_MASK are set, we're using a
// palette-indexed background color
#define CELL_BG_PALETTE         0x0000000008000000ull
// if this bit *and* CELL_FGDEFAULT_MASK are set, we're using a
// palette-indexed foreground color
#define CELL_FG_PALETTE         (CELL_BG_PALETTE << 32u)
// extract these bits to get the background alpha mask
#define CELL_BG_ALPHA_MASK      NCCHANNEL_ALPHA_MASK
// extract these bits to get the foreground alpha mask
#define CELL_FG_ALPHA_MASK      (CELL_BG_ALPHA_MASK << 32u)

// initialize a 64-bit channel pair with specified RGB fg/bg
#define CHANNELS_RGB_INITIALIZER(fr, fg, fb, br, bg, bb) \
  (((((uint64_t)(fr) << 16u) + ((uint64_t)(fg) << 8u) + (uint64_t)(fb)) << 32ull) + \
   (((br) << 16u) + ((bg) << 8u) + (bb)) + CELL_BGDEFAULT_MASK + CELL_FGDEFAULT_MASK)

// These lowest-level functions manipulate a 64-bit channel encoding directly.
// Users will typically manipulate ncplane and cell channels through those APIs,
// rather than calling these directly.

// Extract the 8-bit red component from a 32-bit channel.
static inline unsigned
channel_r(unsigned channel){
  return (channel & 0xff0000u) >> 16u;
}

// Extract the 8-bit green component from a 32-bit channel.
static inline unsigned
channel_g(unsigned channel){
  return (channel & 0x00ff00u) >> 8u;
}

// Extract the 8-bit blue component from a 32-bit channel.
static inline unsigned
channel_b(unsigned channel){
  return (channel & 0x0000ffu);
}

// Extract the three 8-bit R/G/B components from a 32-bit channel.
static inline unsigned
channel_rgb(unsigned channel, unsigned* RESTRICT r, unsigned* RESTRICT g,
            unsigned* RESTRICT b){
  *r = channel_r(channel);
  *g = channel_g(channel);
  *b = channel_b(channel);
  return channel;
}

// Set the three 8-bit components of a 32-bit channel, and mark it as not using
// the default color. Retain the other bits unchanged.
static inline int
channel_set_rgb(unsigned* channel, int r, int g, int b){
  if(r >= 256 || g >= 256 || b >= 256){
    return -1;
  }
  if(r < 0 || g < 0 || b < 0){
    return -1;
  }
  unsigned c = (r << 16u) | (g << 8u) | b;
  *channel = (*channel & ~CELL_BG_RGB_MASK) | CELL_BGDEFAULT_MASK | c;
  return 0;
}

static inline uint32_t
ncpixel(int r, int g, int b){
  if(r < 0) r = 0;
  if(r > 255) r = 255;
  if(g < 0) g = 0;
  if(g > 255) g = 255;
  if(b < 0) b = 0;
  if(b > 255) b = 255;
  return 0xff000000ul | r | (b << 8u) | (g << 16u);
}

static inline unsigned
ncpixel_a(uint32_t pixel){
  return (pixel & 0xff0000fful) >> 24u;
}

static inline unsigned
ncpixel_r(uint32_t pixel){
  return (pixel & 0x000000fful);
}

static inline int
ncpixel_g(uint32_t pixel){
  return (pixel & 0x00ff0000ul) >> 16u;
}

static inline int
ncpixel_b(uint32_t pixel){
  return (pixel & 0x0000ff00ul) >> 8u;
}

static inline int
ncpixel_set_a(uint32_t* pixel, int a){
  if(a > 255 || a < 0){
    return -1;
  }
  *pixel = (*pixel & 0x00fffffful) | (a << 24u);
  return 0;
}

static inline int
ncpixel_set_r(uint32_t* pixel, int r){
  if(r > 255 || r < 0){
    return -1;
  }
  *pixel = (*pixel & 0xffffff00ul) | r;
  return 0;
}

static inline int
ncpixel_set_g(uint32_t* pixel, int g){
  if(g > 255 || g < 0){
    return -1;
  }
  *pixel = (*pixel & 0xff00fffful) | (g << 16u);
  return 0;
}

static inline int
ncpixel_set_b(uint32_t* pixel, int b){
  if(b > 255 || b < 0){
    return -1;
  }
  *pixel = (*pixel & 0xffff00fful) | (b << 8u);
  return 0;
}

// set the RGB values of an RGB pixel
static inline int
ncpixel_set_rgb(uint32_t* pixel, int r, int g, int b){
  if(ncpixel_set_r(pixel, r) || ncpixel_set_g(pixel, g) || ncpixel_set_b(pixel, b)){
    return -1;
  }
  return 0;
}

// Set the three 8-bit components of a 32-bit channel, and mark it as not using
// the default color. Retain the other bits unchanged. r, g, and b will be
// clipped to the range [0..255].
static inline void
channel_set_rgb_clipped(unsigned* channel, int r, int g, int b){
  if(r >= 256){
    r = 255;
  }
  if(g >= 256){
    g = 255;
  }
  if(b >= 256){
    b = 255;
  }
  if(r <= -1){
    r = 0;
  }
  if(g <= -1){
    g = 0;
  }
  if(b <= -1){
    b = 0;
  }
  unsigned c = (r << 16u) | (g << 8u) | b;
  *channel = (*channel & ~CELL_BG_RGB_MASK) | CELL_BGDEFAULT_MASK | c;
}

// Same, but provide an assembled, packed 24 bits of rgb.
static inline int
channel_set(unsigned* channel, unsigned rgb){
  if(rgb > 0xffffffu){
    return -1;
  }
  *channel = (*channel & ~CELL_BG_RGB_MASK) | CELL_BGDEFAULT_MASK | rgb;
  return 0;
}

// Extract the 2-bit alpha component from a 32-bit channel.
static inline unsigned
channel_alpha(unsigned channel){
  return channel & NCCHANNEL_ALPHA_MASK;
}

// Set the 2-bit alpha component of the 32-bit channel.
static inline int
channel_set_alpha(unsigned* channel, unsigned alpha){
  if(alpha & ~NCCHANNEL_ALPHA_MASK){
    return -1;
  }
  *channel = alpha | (*channel & ~NCCHANNEL_ALPHA_MASK);
  if(alpha != CELL_ALPHA_OPAQUE){
    *channel |= CELL_BGDEFAULT_MASK;
  }
  return 0;
}

// Is this channel using the "default color" rather than RGB/palette-indexed?
static inline bool
channel_default_p(unsigned channel){
  return !(channel & CELL_BGDEFAULT_MASK);
}

// Is this channel using palette-indexed color rather than RGB?
static inline bool
channel_palindex_p(unsigned channel){
  return !channel_default_p(channel) && (channel & CELL_BG_PALETTE);
}

// Mark the channel as using its default color, which also marks it opaque.
static inline unsigned
channel_set_default(unsigned* channel){
  return *channel &= ~(CELL_BGDEFAULT_MASK | CELL_ALPHA_HIGHCONTRAST);
}

// Extract the 32-bit background channel from a channel pair.
static inline uint32_t
channels_bchannel(uint64_t channels){
  return channels & 0xfffffffflu;
}

// Extract the 32-bit foreground channel from a channel pair.
static inline uint32_t
channels_fchannel(uint64_t channels){
  return channels_bchannel(channels >> 32u);
}

// Set the 32-bit background channel of a channel pair.
static inline uint64_t
channels_set_bchannel(uint64_t* channels, uint32_t channel){
  return *channels = (*channels & 0xffffffff00000000llu) | channel;
}

// Set the 32-bit foreground channel of a channel pair.
static inline uint64_t
channels_set_fchannel(uint64_t* channels, uint32_t channel){
  return *channels = (*channels & 0xfffffffflu) | ((uint64_t)channel << 32u);
}

static inline uint64_t
channels_combine(uint32_t fchan, uint32_t bchan){
  uint64_t channels = 0;
  channels_set_fchannel(&channels, fchan);
  channels_set_bchannel(&channels, bchan);
  return channels;
}

// Extract 24 bits of foreground RGB from 'channels', shifted to LSBs.
static inline unsigned
channels_fg(uint64_t channels){
  return channels_fchannel(channels) & CELL_BG_RGB_MASK;
}

// Extract 24 bits of background RGB from 'channels', shifted to LSBs.
static inline unsigned
channels_bg(uint64_t channels){
  return channels_bchannel(channels) & CELL_BG_RGB_MASK;
}

// Extract 2 bits of foreground alpha from 'channels', shifted to LSBs.
static inline unsigned
channels_fg_alpha(uint64_t channels){
  return channel_alpha(channels_fchannel(channels));
}

// Extract 2 bits of background alpha from 'channels', shifted to LSBs.
static inline unsigned
channels_bg_alpha(uint64_t channels){
  return channel_alpha(channels_bchannel(channels));
}

// Extract 24 bits of foreground RGB from 'channels', split into subchannels.
static inline unsigned
channels_fg_rgb(uint64_t channels, unsigned* r, unsigned* g, unsigned* b){
  return channel_rgb(channels_fchannel(channels), r, g, b);
}

// Extract 24 bits of background RGB from 'channels', split into subchannels.
static inline unsigned
channels_bg_rgb(uint64_t channels, unsigned* r, unsigned* g, unsigned* b){
  return channel_rgb(channels_bchannel(channels), r, g, b);
}

// Set the r, g, and b channels for the foreground component of this 64-bit
// 'channels' variable, and mark it as not using the default color.
static inline int
channels_set_fg_rgb(uint64_t* channels, int r, int g, int b){
  unsigned channel = channels_fchannel(*channels);
  if(channel_set_rgb(&channel, r, g, b) < 0){
    return -1;
  }
  *channels = ((uint64_t)channel << 32llu) | (*channels & 0xffffffffllu);
  return 0;
}

// Same, but clips to [0..255].
static inline void
channels_set_fg_rgb_clipped(uint64_t* channels, int r, int g, int b){
  unsigned channel = channels_fchannel(*channels);
  channel_set_rgb_clipped(&channel, r, g, b);
  *channels = ((uint64_t)channel << 32llu) | (*channels & 0xffffffffllu);
}

// Same, but set an assembled 24 bit channel at once.
static inline int
channels_set_fg(uint64_t* channels, unsigned rgb){
  unsigned channel = channels_fchannel(*channels);
  if(channel_set(&channel, rgb) < 0){
    return -1;
  }
  *channels = ((uint64_t)channel << 32llu) | (*channels & 0xffffffffllu);
  return 0;
}

// Set the r, g, and b channels for the background component of this 64-bit
// 'channels' variable, and mark it as not using the default color.
static inline int
channels_set_bg_rgb(uint64_t* channels, int r, int g, int b){
  unsigned channel = channels_bchannel(*channels);
  if(channel_set_rgb(&channel, r, g, b) < 0){
    return -1;
  }
  channels_set_bchannel(channels, channel);
  return 0;
}

// Same, but clips to [0..255].
static inline void
channels_set_bg_rgb_clipped(uint64_t* channels, int r, int g, int b){
  unsigned channel = channels_bchannel(*channels);
  channel_set_rgb_clipped(&channel, r, g, b);
  channels_set_bchannel(channels, channel);
}

// Same, but set an assembled 24 bit channel at once.
static inline int
channels_set_bg(uint64_t* channels, unsigned rgb){
  unsigned channel = channels_bchannel(*channels);
  if(channel_set(&channel, rgb) < 0){
    return -1;
  }
  channels_set_bchannel(channels, channel);
  return 0;
}

// Set the 2-bit alpha component of the foreground channel.
static inline int
channels_set_fg_alpha(uint64_t* channels, unsigned alpha){
  unsigned channel = channels_fchannel(*channels);
  if(channel_set_alpha(&channel, alpha) < 0){
    return -1;
  }
  *channels = ((uint64_t)channel << 32llu) | (*channels & 0xffffffffllu);
  return 0;
}

// Set the 2-bit alpha component of the background channel.
static inline int
channels_set_bg_alpha(uint64_t* channels, unsigned alpha){
  if(alpha == CELL_ALPHA_HIGHCONTRAST){ // forbidden for background alpha
    return -1;
  }
  unsigned channel = channels_bchannel(*channels);
  if(channel_set_alpha(&channel, alpha) < 0){
    return -1;
  }
  channels_set_bchannel(channels, channel);
  return 0;
}

// Is the foreground using the "default foreground color"?
static inline bool
channels_fg_default_p(uint64_t channels){
  return channel_default_p(channels_fchannel(channels));
}

// Is the foreground using indexed palette color?
static inline bool
channels_fg_palindex_p(uint64_t channels){
  return channel_palindex_p(channels_fchannel(channels));
}

// Is the background using the "default background color"? The "default
// background color" must generally be used to take advantage of
// terminal-effected transparency.
static inline bool
channels_bg_default_p(uint64_t channels){
  return channel_default_p(channels_bchannel(channels));
}

// Is the background using indexed palette color?
static inline bool
channels_bg_palindex_p(uint64_t channels){
  return channel_palindex_p(channels_bchannel(channels));
}

// Mark the foreground channel as using its default color.
static inline uint64_t
channels_set_fg_default(uint64_t* channels){
  unsigned channel = channels_fchannel(*channels);
  channel_set_default(&channel);
  *channels = ((uint64_t)channel << 32llu) | (*channels & 0xffffffffllu);
  return *channels;
}

// Mark the foreground channel as using its default color.
static inline uint64_t
channels_set_bg_default(uint64_t* channels){
  unsigned channel = channels_bchannel(*channels);
  channel_set_default(&channel);
  channels_set_bchannel(channels, channel);
  return *channels;
}

// A cell corresponds to a single character cell on some plane, which can be
// occupied by a single grapheme cluster (some root spacing glyph, along with
// possible combining characters, which might span multiple columns). At any
// cell, we can have a theoretically arbitrarily long UTF-8 string, a foreground
// color, a background color, and an attribute set. Valid grapheme cluster
// contents include:
//
//  * A NUL terminator,
//  * A single control character, followed by a NUL terminator,
//  * At most one spacing character, followed by zero or more nonspacing
//    characters, followed by a NUL terminator.
//
// Multi-column characters can only have a single style/color throughout.
// Existence is suffering, and thus wcwidth() is not reliable. It's just
// quoting whether or not the EGC contains a "Wide Asian" double-width
// character. This is set for some things, like most emoji, and not set for
// other things, like cuneiform. Fucccccck. True display width is a *property
// of the font*. Fuccccccccckkkkk. Among the longest Unicode codepoints is
//
//    U+FDFD ARABIC LIGATURE BISMILLAH AR-RAHMAN AR-RAHEEM ﷽
//
// wcwidth() rather optimistically claims this suicide bomber of a glyph to
// occupy a single column, right before it explodes in your diner. BiDi text
// is too complicated for me to even get into here. It sucks ass. Be assured
// there are no easy answers. Allah, the All-Powerful, has fucked us again!
//
// Each cell occupies 16 static bytes (128 bits). The surface is thus ~1.6MB
// for a (pretty large) 500x200 terminal. At 80x43, it's less than 64KB.
// Dynamic requirements (the egcpool) can add up to 32MB to an ncplane, but
// such large pools are unlikely in common use.
//
// We implement some small alpha compositing. Foreground and background both
// have two bits of inverted alpha. The actual grapheme written to a cell is
// the topmost non-zero grapheme. If its alpha is 00, its foreground color is
// used unchanged. If its alpha is 10, its foreground color is derived entirely
// from cells underneath it. Otherwise, the result will be a composite.
// Likewise for the background. If the bottom of a coordinate's zbuffer is
// reached with a cumulative alpha of zero, the default is used. In this way,
// a terminal configured with transparent background can be supported through
// multiple occluding ncplanes. A foreground alpha of 11 requests high-contrast
// text (relative to the computed background). A background alpha of 11 is
// currently forbidden.
//
// Default color takes precedence over palette or RGB, and cannot be used with
// transparency. Indexed palette takes precedence over RGB. It cannot
// meaningfully set transparency, but it can be mixed into a cascading color.
// RGB is used if neither default terminal colors nor palette indexing are in
// play, and fully supports all transparency options.
typedef struct cell {
  // These 32 bits are either a single-byte, single-character grapheme cluster
  // (values 0--0x7f), or an offset into a per-ncplane attached pool of
  // varying-length UTF-8 grapheme clusters. This pool may thus be up to 32MB.
  uint32_t gcluster;          // 4B -> 4B
  // NCSTYLE_* attributes (16 bits) + 8 foreground palette index bits + 8
  // background palette index bits. palette index bits are used only if the
  // corresponding default color bit *is not* set, and the corresponding
  // palette index bit *is* set.
  uint32_t attrword;          // + 4B -> 8B
  // (channels & 0x8000000000000000ull): part of a wide glyph
  // (channels & 0x4000000000000000ull): foreground is *not* "default color"
  // (channels & 0x3000000000000000ull): foreground alpha (2 bits)
  // (channels & 0x0800000000000000ull): foreground uses palette index
  // (channels & 0x0700000000000000ull): reserved, must be 0
  // (channels & 0x00ffffff00000000ull): foreground in 3x8 RGB (rrggbb)
  // (channels & 0x0000000080000000ull): reserved, must be 0
  // (channels & 0x0000000040000000ull): background is *not* "default color"
  // (channels & 0x0000000030000000ull): background alpha (2 bits)
  // (channels & 0x0000000008000000ull): background uses palette index
  // (channels & 0x0000000007000000ull): reserved, must be 0
  // (channels & 0x0000000000ffffffull): background in 3x8 RGB (rrggbb)
  // At render time, these 24-bit values are quantized down to terminal
  // capabilities, if necessary. There's a clear path to 10-bit support should
  // we one day need it, but keep things cagey for now. "default color" is
  // best explained by color(3NCURSES). ours is the same concept. until the
  // "not default color" bit is set, any color you load will be ignored.
  uint64_t channels;          // + 8B == 16B
} cell;

#define CELL_TRIVIAL_INITIALIZER { .gcluster = '\0', .attrword = 0, .channels = 0, }
#define CELL_SIMPLE_INITIALIZER(c) { .gcluster = (c), .attrword = 0, .channels = 0, }
#define CELL_INITIALIZER(c, a, chan) { .gcluster = (c), .attrword = (a), .channels = (chan), }

static inline void
cell_init(cell* c){
  memset(c, 0, sizeof(*c));
}

// Breaks the UTF-8 string in 'gcluster' down, setting up the cell 'c'. Returns
// the number of bytes copied out of 'gcluster', or -1 on failure. The styling
// of the cell is left untouched, but any resources are released.
__attribute__ ((nonnull (1, 2, 3)))
API int cell_load(struct ncplane* n, cell* c, const char* gcluster);

// cell_load(), plus blast the styling with 'attr' and 'channels'.
static inline int
cell_prime(struct ncplane* n, cell* c, const char* gcluster,
           uint32_t attr, uint64_t channels){
  c->attrword = attr;
  c->channels = channels;
  int ret = cell_load(n, c, gcluster);
  return ret;
}

// Duplicate 'c' into 'targ'; both must be/will be bound to 'n'.
API int cell_duplicate(struct ncplane* n, cell* targ, const cell* c);

// Release resources held by the cell 'c'.
API void cell_release(struct ncplane* n, cell* c);

#define NCSTYLE_MASK      0xffff0000ul
#define NCSTYLE_STANDOUT  0x00800000ul
#define NCSTYLE_UNDERLINE 0x00400000ul
#define NCSTYLE_REVERSE   0x00200000ul
#define NCSTYLE_BLINK     0x00100000ul
#define NCSTYLE_DIM       0x00080000ul
#define NCSTYLE_BOLD      0x00040000ul
#define NCSTYLE_INVIS     0x00020000ul
#define NCSTYLE_PROTECT   0x00010000ul
#define NCSTYLE_ITALIC    0x01000000ul
#define NCSTYLE_NONE      0

// Set the specified style bits for the cell 'c', whether they're actively
// supported or not.
static inline void
cell_styles_set(cell* c, unsigned stylebits){
  c->attrword = (c->attrword & ~NCSTYLE_MASK) | ((stylebits & NCSTYLE_MASK));
}

// Extract the style bits from the cell's attrword.
static inline unsigned
cell_styles(const cell* c){
  return c->attrword & NCSTYLE_MASK;
}

// Add the specified styles (in the LSBs) to the cell's existing spec, whether
// they're actively supported or not.
static inline void
cell_styles_on(cell* c, unsigned stylebits){
  c->attrword |= (stylebits & NCSTYLE_MASK);
}

// Remove the specified styles (in the LSBs) from the cell's existing spec.
static inline void
cell_styles_off(cell* c, unsigned stylebits){
  c->attrword &= ~(stylebits & NCSTYLE_MASK);
}

// Use the default color for the foreground.
static inline void
cell_set_fg_default(cell* c){
  channels_set_fg_default(&c->channels);
}

// Use the default color for the background.
static inline void
cell_set_bg_default(cell* c){
  channels_set_bg_default(&c->channels);
}

static inline int
cell_set_fg_alpha(cell* c, int alpha){
  return channels_set_fg_alpha(&c->channels, alpha);
}

static inline int
cell_set_bg_alpha(cell* c, int alpha){
  return channels_set_bg_alpha(&c->channels, alpha);
}

// Does the cell contain an East Asian Wide codepoint?
static inline bool
cell_double_wide_p(const cell* c){
  return (c->channels & CELL_WIDEASIAN_MASK);
}

// Is this the right half of a wide character?
static inline bool
cell_wide_right_p(const cell* c){
  return cell_double_wide_p(c) && c->gcluster == 0;
}

// Is this the left half of a wide character?
static inline bool
cell_wide_left_p(const cell* c){
  return cell_double_wide_p(c) && c->gcluster;
}

// Is the cell simple (a lone ASCII character, encoded as such)?
static inline bool
cell_simple_p(const cell* c){
  return c->gcluster < 0x80;
}

// return a pointer to the NUL-terminated EGC referenced by 'c'. this pointer
// is invalidated by any further operation on the plane 'n', so...watch out!
API const char* cell_extended_gcluster(const struct ncplane* n, const cell* c);

// Extract the EGC from 'c' as a nul-terminated string.
static inline char*
cell_strdup(const struct ncplane* n, const cell* c){
  char* ret;
  if(cell_simple_p(c)){
    if( (ret = (char*)malloc(2)) ){ // cast is here for C++ clients
      ret[0] = c->gcluster;
      ret[1] = '\0';
    }
  }else{
    ret = strdup(cell_extended_gcluster(n, c));
  }
  return ret;
}

// Extract the three elements of a cell.
static inline char*
cell_extract(const struct ncplane* n, const cell* c,
             uint32_t* attrword, uint64_t* channels){
  if(attrword){
    *attrword = c->attrword;
  }
  if(channels){
    *channels = c->channels;
  }
  return cell_strdup(n, c);
}

// Returns true if the two cells are distinct EGCs, attributes, or channels.
// The actual egcpool index needn't be the same--indeed, the planes needn't even
// be the same. Only the expanded EGC must be equal. The EGC must be bit-equal;
// it would probably be better to test whether they're Unicode-equal FIXME.
static inline bool
cellcmp(const struct ncplane* n1, const cell* RESTRICT c1,
        const struct ncplane* n2, const cell* RESTRICT c2){
  if(c1->attrword != c2->attrword){
    return true;
  }
  if(c1->channels != c2->channels){
    return true;
  }
  if(cell_simple_p(c1) && cell_simple_p(c2)){
    return c1->gcluster != c2->gcluster;
  }
  if(cell_simple_p(c1) || cell_simple_p(c2)){
    return true;
  }
  return strcmp(cell_extended_gcluster(n1, c1), cell_extended_gcluster(n2, c2));
}

// True if the cell does not generate foreground pixels (i.e., the cell is
// entirely whitespace or special characters).
// FIXME do this at cell prep time and set a bit in the channels
static inline bool
cell_noforeground_p(const cell* c){
  return cell_simple_p(c) && (c->gcluster == ' ' || !isprint(c->gcluster));
}

static inline int
cell_load_simple(struct ncplane* n, cell* c, char ch){
  cell_release(n, c);
  c->channels &= ~CELL_WIDEASIAN_MASK;
  c->gcluster = ch;
  if(cell_simple_p(c)){
    return 1;
  }
  return -1;
}

// get the offset into the egcpool for this cell's EGC. returns meaningless and
// unsafe results if called on a simple cell.
static inline uint32_t
cell_egc_idx(const cell* c){
  return c->gcluster - 0x80;
}

// These log levels consciously map cleanly to those of libav; notcurses itself
// does not use this full granularity. The log level does not affect the opening
// and closing banners, which can be disabled via the notcurses_option struct's
// 'suppress_banner'. Note that if stderr is connected to the same terminal on
// which we're rendering, any kind of logging will disrupt the output.
typedef enum {
  NCLOGLEVEL_SILENT,  // default. print nothing once fullscreen service begins
  NCLOGLEVEL_PANIC,   // print diagnostics immediately related to crashing
  NCLOGLEVEL_FATAL,   // we're hanging around, but we've had a horrible fault
  NCLOGLEVEL_ERROR,   // we can't keep doin' this, but we can do other things
  NCLOGLEVEL_WARNING, // you probably don't want what's happening to happen
  NCLOGLEVEL_INFO,    // "standard information"
  NCLOGLEVEL_VERBOSE, // "detailed information"
  NCLOGLEVEL_DEBUG,   // this is honestly a bit much
  NCLOGLEVEL_TRACE,   // there's probably a better way to do what you want
} ncloglevel_e;

// Bits for notcurses_options->flags.

// notcurses_init() will call setlocale() to inspect the current locale. If
// that locale is "C" or "POSIX", it will call setlocale(LC_ALL, "") to set
// the locale according to the LANG environment variable. Ideally, this will
// result in UTF8 being enabled, even if the client app didn't call
// setlocale() itself. Unless you're certain that you're invoking setlocale() 
// prior to notcurses_init(), you should not set this bit. Even if you are
// invoking setlocale(), this behavior shouldn't be an issue unless you're
// doing something weird (setting a locale not based on LANG).
#define NCOPTION_INHIBIT_SETLOCALE   0x0001ull

// Checking for Sixel support requires writing an escape, and then reading an
// inline reply from the terminal. Since this can interact poorly with actual
// user input, it's not done unless Sixel will actually be used. Set this flag
// to unconditionally test for Sixel support in notcurses_init().
#define NCOPTION_VERIFY_SIXEL        0x0002ull

// We typically install a signal handler for SIGWINCH that generates a resize
// event in the notcurses_getc() queue. Set to inhibit this handler.
#define NCOPTION_NO_WINCH_SIGHANDLER 0x0004ull

// We typically install a signal handler for SIG{INT, SEGV, ABRT, QUIT} that
// restores the screen, and then calls the old signal handler. Set to inhibit
// registration of these signal handlers.
#define NCOPTION_NO_QUIT_SIGHANDLERS 0x0008ull

// By default, we hide the cursor if possible. This flag inhibits use of
// the civis capability, retaining the cursor.
#define NCOPTION_RETAIN_CURSOR       0x0010ull

// Notcurses typically prints version info in notcurses_init() and performance
// info in notcurses_stop(). This inhibits that output.
#define NCOPTION_SUPPRESS_BANNERS    0x0020ull

// If smcup/rmcup capabilities are indicated, notcurses defaults to making use
// of the "alternate screen". This flag inhibits use of smcup/rmcup.
#define NCOPTION_NO_ALTERNATE_SCREEN 0x0040ull

// Configuration for notcurses_init().
typedef struct notcurses_options {
  // The name of the terminfo database entry describing this terminal. If NULL,
  // the environment variable TERM is used. Failure to open the terminal
  // definition will result in failure to initialize notcurses.
  const char* termtype;
  // If non-NULL, notcurses_render() will write each rendered frame to this
  // FILE* in addition to outfp. This is used primarily for debugging.
  FILE* renderfp;
  // Progressively higher log levels result in more logging to stderr. By
  // default, nothing is printed to stderr once fullscreen service begins.
  ncloglevel_e loglevel;
  // Desirable margins. If all are 0 (default), we will render to the entirety
  // of the screen. If the screen is too small, we do what we can--this is
  // strictly best-effort. Absolute coordinates are relative to the rendering
  // area ((0, 0) is always the origin of the rendering area).
  int margin_t, margin_r, margin_b, margin_l;
  // General flags; see NCOPTION_*. This is expressed as a bitfield so that
  // future options can be added without reshaping the struct. Undefined bits
  // must be set to 0.
  uint64_t flags;
} notcurses_options;

// Lex a margin argument according to the standard notcurses definition. There
// can be either a single number, which will define all margins equally, or
// there can be four numbers separated by commas.
API int notcurses_lex_margins(const char* op, notcurses_options* opts);

// Lex a visual scaling mode (one of "none", "stretch", or "scale").
API int notcurses_lex_scalemode(const char* op, ncscale_e* scalemode);

// Initialize a notcurses context on the connected terminal at 'fp'. 'fp' must
// be a tty. You'll usually want stdout. NULL can be supplied for 'fp', in
// which case /dev/tty will be opened. Returns NULL on error, including any
// failure initializing terminfo.
API struct notcurses* notcurses_init(const notcurses_options* opts, FILE* fp);

// Destroy a notcurses context.
API int notcurses_stop(struct notcurses* nc);

// Make the physical screen match the virtual screen. Changes made to the
// virtual screen (i.e. most other calls) will not be visible until after a
// successful call to notcurses_render().
API int notcurses_render(struct notcurses* nc);

// Write the last rendered frame, in its entirety, to 'fp'. This is not valid
// until notcurses_render() has been successfully called at least once.
API int notcurses_render_to_file(struct notcurses* nc, FILE* fp);

// Return the topmost ncplane, of which there is always at least one.
API struct ncplane* notcurses_top(struct notcurses* n);

// Destroy all ncplanes other than the stdplane.
API void notcurses_drop_planes(struct notcurses* nc);

// All input is currently taken from stdin, though this will likely change. We
// attempt to read a single UTF8-encoded Unicode codepoint, *not* an entire
// Extended Grapheme Cluster. It is also possible that we will read a special
// keypress, i.e. anything that doesn't correspond to a Unicode codepoint (e.g.
// arrow keys, function keys, screen resize events, etc.). These are mapped
// into Unicode's Supplementary Private Use Area-B, starting at U+100000.
//
// notcurses_getc() and notcurses_getc_nblock() are both nonblocking.
// notcurses_getc_blocking() blocks until a codepoint or special key is read,
// or until interrupted by a signal.
//
// In the case of a valid read, a 32-bit Unicode codepoint is returned. 0 is
// returned to indicate that no input was available, but only by
// notcurses_getc(). Otherwise (including on EOF) (char32_t)-1 is returned.

// Is this char32_t a Supplementary Private Use Area-B codepoint?
static inline bool
nckey_supppuab_p(char32_t w){
  return w >= 0x100000 && w <= 0x10fffd;
}

// Is the event a synthesized mouse event?
static inline bool
nckey_mouse_p(char32_t r){
  return r >= NCKEY_BUTTON1 && r <= NCKEY_RELEASE;
}

// An input event. Cell coordinates are currently defined only for mouse events.
typedef struct ncinput {
  char32_t id;     // identifier. Unicode codepoint or synthesized NCKEY event
  int y;           // y cell coordinate of event, -1 for undefined
  int x;           // x cell coordinate of event, -1 for undefined
  bool alt;        // was alt held?
  bool shift;      // was shift held?
  bool ctrl;       // was ctrl held?
  uint64_t seqnum; // input event number
} ncinput;

// See ppoll(2) for more detail. Provide a NULL 'ts' to block at length, a 'ts'
// of 0 for non-blocking operation, and otherwise a timespec to bound blocking.
// Signals in sigmask (less several we handle internally) will be atomically
// masked and unmasked per ppoll(2). It should generally contain all signals.
// Returns a single Unicode code point, or (char32_t)-1 on error. 'sigmask' may
// be NULL. Returns 0 on a timeout. If an event is processed, the return value
// is the 'id' field from that event. 'ni' may be NULL.
API char32_t notcurses_getc(struct notcurses* n, const struct timespec* ts,
                            sigset_t* sigmask, ncinput* ni);

// Get a file descriptor suitable for input event poll()ing. When this
// descriptor becomes available, you can call notcureses_getc_nblock(),
// and input ought be ready. This file descriptor is *not* necessarily
// the file descriptor associated with stdin (but it might be!).
API int notcurses_inputready_fd(struct notcurses* n);

// 'ni' may be NULL if the caller is uninterested in event details. If no event
// is ready, returns 0.
static inline char32_t
notcurses_getc_nblock(struct notcurses* n, ncinput* ni){
  sigset_t sigmask;
  sigfillset(&sigmask);
  struct timespec ts = { .tv_sec = 0, .tv_nsec = 0 };
  return notcurses_getc(n, &ts, &sigmask, ni);
}

// 'ni' may be NULL if the caller is uninterested in event details. Blocks
// until an event is processed or a signal is received.
static inline char32_t
notcurses_getc_blocking(struct notcurses* n, ncinput* ni){
  sigset_t sigmask;
  sigemptyset(&sigmask);
  return notcurses_getc(n, NULL, &sigmask, ni);
}

// Enable the mouse in "button-event tracking" mode with focus detection and
// UTF8-style extended coordinates. On failure, -1 is returned. On success, 0
// is returned, and mouse events will be published to notcurses_getc().
API int notcurses_mouse_enable(struct notcurses* n);

// Disable mouse events. Any events in the input queue can still be delivered.
API int notcurses_mouse_disable(struct notcurses* n);

// Refresh the physical screen to match what was last rendered (i.e., without
// reflecting any changes since the last call to notcurses_render()). This is
// primarily useful if the screen is externally corrupted, or if an
// NCKEY_RESIZE event has been read and you're not ready to render.
API int notcurses_refresh(struct notcurses* n, int* RESTRICT y, int* RESTRICT x);

API struct notcurses* ncplane_notcurses(struct ncplane* n);
API const struct notcurses* ncplane_notcurses_const(const struct ncplane* n);

// Return the dimensions of this ncplane.
API void ncplane_dim_yx(const struct ncplane* n, int* RESTRICT y, int* RESTRICT x);

// Get a reference to the standard plane (one matching our current idea of the
// terminal size) for this terminal. The standard plane always exists, and its
// origin is always at the uppermost, leftmost cell of the terminal.
API struct ncplane* notcurses_stdplane(struct notcurses* nc);
API const struct ncplane* notcurses_stdplane_const(const struct notcurses* nc);

// notcurses_stdplane(), plus free bonus dimensions written to non-NULL y/x!
static inline struct ncplane*
notcurses_stddim_yx(struct notcurses* nc, int* RESTRICT y, int* RESTRICT x){
  struct ncplane* s = notcurses_stdplane(nc); // can't fail
  ncplane_dim_yx(s, y, x); // accepts NULL
  return s;
}

static inline int
ncplane_dim_y(const struct ncplane* n){
  int dimy;
  ncplane_dim_yx(n, &dimy, NULL);
  return dimy;
}

static inline int
ncplane_dim_x(const struct ncplane* n){
  int dimx;
  ncplane_dim_yx(n, NULL, &dimx);
  return dimx;
}

// Return our current idea of the terminal dimensions in rows and cols.
static inline void
notcurses_term_dim_yx(const struct notcurses* n, int* RESTRICT rows, int* RESTRICT cols){
  ncplane_dim_yx(notcurses_stdplane_const(n), rows, cols);
}

// Retrieve the contents of the specified cell as last rendered. The EGC is
// returned, or NULL on error. This EGC must be free()d by the caller. The
// attrword and channels are written to 'attrword' and 'channels', respectively.
API char* notcurses_at_yx(struct notcurses* nc, int yoff, int xoff,
                          uint32_t* attrword, uint64_t* channels);

// Create a new ncplane at the specified offset (relative to the standard plane)
// and the specified size. The number of rows and columns must both be positive.
// This plane is initially at the top of the z-buffer, as if ncplane_move_top()
// had been called on it. The void* 'opaque' can be retrieved (and reset) later.
API struct ncplane* ncplane_new(struct notcurses* nc, int rows, int cols,
                                int yoff, int xoff, void* opaque);

API struct ncplane* ncplane_aligned(struct ncplane* n, int rows, int cols,
                                    int yoff, ncalign_e align, void* opaque);

// Create a plane bound to plane 'n'. Being bound to 'n' means that 'yoff' and
// 'xoff' are interpreted relative to that plane's origin, and that if that
// plane is moved later, this new plane is moved by the same amount.
API struct ncplane* ncplane_bound(struct ncplane* n, int rows, int cols,
                                  int yoff, int xoff, void* opaque);

// Plane 'n' will be unbound from its parent plane, if it is currently bound,
// and will be made a bound child of 'newparent', if 'newparent' is not NULL.
API struct ncplane* ncplane_reparent(struct ncplane* n, struct ncplane* newparent);

// Duplicate an existing ncplane. The new plane will have the same geometry,
// will duplicate all content, and will start with the same rendering state.
// The new plane will be immediately above the old one on the z axis.
API struct ncplane* ncplane_dup(const struct ncplane* n, void* opaque);

// provided a coordinate relative to the origin of 'src', map it to the same
// absolute coordinate relative to thte origin of 'dst'. either or both of 'y'
// and 'x' may be NULL. if 'dst' is NULL, it is taken to be the standard plane.
API void ncplane_translate(const struct ncplane* src, const struct ncplane* dst,
                           int* RESTRICT y, int* RESTRICT x);

// Fed absolute 'y'/'x' coordinates, determine whether that coordinate is
// within the ncplane 'n'. If not, return false. If so, return true. Either
// way, translate the absolute coordinates relative to 'n'. If the point is not
// within 'n', these coordinates will not be within the dimensions of the plane.
API bool ncplane_translate_abs(const struct ncplane* n, int* RESTRICT y, int* RESTRICT x);

// All planes are created with scrolling disabled. Scrolling can be dynamically
// controlled with ncplane_set_scrolling(). Returns true if scrolling was
// previously enabled, or false if it was disabled.
API bool ncplane_set_scrolling(struct ncplane* n, bool scrollp);

// Capabilities

// Returns a 16-bit bitmask of supported curses-style attributes
// (NCSTYLE_UNDERLINE, NCSTYLE_BOLD, etc.) The attribute is only
// indicated as supported if the terminal can support it together with color.
// For more information, see the "ncv" capability in terminfo(5).
API unsigned notcurses_supported_styles(const struct notcurses* nc);

// Returns the number of simultaneous colors claimed to be supported, or 1 if
// there is no color support. Note that several terminal emulators advertise
// more colors than they actually support, downsampling internally.
API int notcurses_palette_size(const struct notcurses* nc);

// Can we directly specify RGB values per cell, or only use palettes?
API bool notcurses_cantruecolor(const struct notcurses* nc);

// Can we fade? Fading requires either the "rgb" or "ccc" terminfo capability.
API bool notcurses_canfade(const struct notcurses* nc);

// Can we set the "hardware" palette? Requires the "ccc" terminfo capability.
API bool notcurses_canchangecolor(const struct notcurses* nc);

// Can we load images? This requires being built against FFmpeg/OIIO.
API bool notcurses_canopen_images(const struct notcurses* nc);

// Can we load videos? This requires being built against FFmpeg.
API bool notcurses_canopen_videos(const struct notcurses* nc);

// Is our encoding UTF-8? Requires LANG being set to a UTF8 locale.
API bool notcurses_canutf8(const struct notcurses* nc);

// Can we blit to Sixel?
API bool notcurses_cansixel(const struct notcurses* nc);

typedef struct ncstats {
  // purely increasing stats
  uint64_t renders;          // number of successful notcurses_render() runs
  uint64_t failed_renders;   // number of aborted renders, should be 0
  uint64_t render_bytes;     // bytes emitted to ttyfp
  int64_t render_max_bytes;  // max bytes emitted for a frame
  int64_t render_min_bytes;  // min bytes emitted for a frame
  uint64_t render_ns;        // nanoseconds spent in notcurses_render()
  int64_t render_max_ns;     // max ns spent in notcurses_render()
  int64_t render_min_ns;     // min ns spent in successful notcurses_render()
  uint64_t cellelisions;     // cells we elided entirely thanks to damage maps
  uint64_t cellemissions;    // cells we emitted due to inferred damage
  uint64_t fgelisions;       // RGB fg elision count
  uint64_t fgemissions;      // RGB fg emissions
  uint64_t bgelisions;       // RGB bg elision count
  uint64_t bgemissions;      // RGB bg emissions
  uint64_t defaultelisions;  // default color was emitted
  uint64_t defaultemissions; // default color was elided

  // current state -- these can decrease
  uint64_t fbbytes;          // total bytes devoted to all active framebuffers
  unsigned planes;           // number of planes currently in existence
} ncstats;

// Acquire an atomic snapshot of the notcurses object's stats.
API void notcurses_stats(const struct notcurses* nc, ncstats* stats);

// Reset all cumulative stats (immediate ones, such as fbbytes, are not reset).
API void notcurses_reset_stats(struct notcurses* nc, ncstats* stats);

// Resize the specified ncplane. The four parameters 'keepy', 'keepx',
// 'keepleny', and 'keeplenx' define a subset of the ncplane to keep,
// unchanged. This may be a section of size 0, though none of these four
// parameters may be negative. 'keepx' and 'keepy' are relative to the ncplane.
// They must specify a coordinate within the ncplane's totality. 'yoff' and
// 'xoff' are relative to 'keepy' and 'keepx', and place the upper-left corner
// of the resized ncplane. Finally, 'ylen' and 'xlen' are the dimensions of the
// ncplane after resizing. 'ylen' must be greater than or equal to 'keepleny',
// and 'xlen' must be greater than or equal to 'keeplenx'. It is an error to
// attempt to resize the standard plane. If either of 'keepleny' or 'keeplenx'
// is non-zero, both must be non-zero.
//
// Essentially, the kept material does not move. It serves to anchor the
// resized plane. If there is no kept material, the plane can move freely.
API int ncplane_resize(struct ncplane* n, int keepy, int keepx, int keepleny,
                       int keeplenx, int yoff, int xoff, int ylen, int xlen);

// Resize the plane, retaining what data we can (everything, unless we're
// shrinking in some dimension). Keep the origin where it is.
static inline int
ncplane_resize_simple(struct ncplane* n, int ylen, int xlen){
  int oldy, oldx;
  ncplane_dim_yx(n, &oldy, &oldx); // current dimensions of 'n'
  int keepleny = oldy > ylen ? ylen : oldy;
  int keeplenx = oldx > xlen ? xlen : oldx;
  return ncplane_resize(n, 0, 0, keepleny, keeplenx, 0, 0, ylen, xlen);
}

// Destroy the specified ncplane. None of its contents will be visible after
// the next call to notcurses_render(). It is an error to attempt to destroy
// the standard plane.
API int ncplane_destroy(struct ncplane* ncp);

// Set the ncplane's base cell to this cell. It will be used for purposes of
// rendering anywhere that the ncplane's gcluster is 0. Erasing the ncplane
// does not reset the base cell; this function must be called with a zero 'c'.
API int ncplane_set_base_cell(struct ncplane* ncp, const cell* c);

// Set the ncplane's base cell to this cell. It will be used for purposes of
// rendering anywhere that the ncplane's gcluster is 0. Erasing the ncplane
// does not reset the base cell; this function must be called with an empty
// 'egc'. 'egc' must be a single extended grapheme cluster.
API int ncplane_set_base(struct ncplane* ncp, const char* egc,
                         uint32_t attrword, uint64_t channels);

// Extract the ncplane's base cell into 'c'. The reference is invalidated if
// 'ncp' is destroyed.
API int ncplane_base(struct ncplane* ncp, cell* c);

// Move this plane relative to the standard plane. It is an error to attempt to
// move the standard plane.
API int ncplane_move_yx(struct ncplane* n, int y, int x);

// Get the origin of this plane relative to the standard plane.
API void ncplane_yx(const struct ncplane* n, int* RESTRICT y, int* RESTRICT x);

// Splice ncplane 'n' out of the z-buffer, and reinsert it at the top or bottom.
API void ncplane_move_top(struct ncplane* n);
API void ncplane_move_bottom(struct ncplane* n);

// Splice ncplane 'n' out of the z-buffer, and reinsert it above 'above'.
// Returns non-zero if 'n' is already in the desired location. 'n' and
// 'above' must not be the same plane.
API int ncplane_move_above(struct ncplane* RESTRICT n,
                           struct ncplane* RESTRICT above);

// Splice ncplane 'n' out of the z-buffer, and reinsert it below 'below'.
// Returns non-zero if 'n' is already in the desired location. 'n' and
// 'below' must not be the same plane.
API int ncplane_move_below(struct ncplane* RESTRICT n,
                           struct ncplane* RESTRICT below);

// Return the plane below this one, or NULL if this is at the bottom.
API struct ncplane* ncplane_below(struct ncplane* n);

// Rotate the plane π/2 radians clockwise or counterclockwise. This cannot
// be performed on arbitrary planes, because glyphs cannot be arbitrarily
// rotated. The glyphs which can be rotated are limited: line-drawing
// characters, spaces, half blocks, and full blocks. The plane must have
// an even number of columns. Use the ncvisual rotation for a more
// flexible approach.
API int ncplane_rotate_cw(struct ncplane* n);
API int ncplane_rotate_ccw(struct ncplane* n);

// Retrieve the current contents of the cell under the cursor. The EGC is
// returned, or NULL on error. This EGC must be free()d by the caller. The
// attrword and channels are written to 'attrword' and 'channels', respectively.
API char* ncplane_at_cursor(struct ncplane* n, uint32_t* attrword, uint64_t* channels);

// Retrieve the current contents of the cell under the cursor into 'c'. This
// cell is invalidated if the associated plane is destroyed.
static inline int
ncplane_at_cursor_cell(struct ncplane* n, cell* c){
  char* egc = ncplane_at_cursor(n, &c->attrword, &c->channels);
  if(!egc){
    return -1;
  }
  uint64_t channels = c->channels; // need to preserve wide flag
  int r = cell_load(n, c, egc);
  c->channels = channels;
  if(r < 0){
    free(egc);
  }
  return r;
}

// Retrieve the current contents of the specified cell. The EGC is returned, or
// NULL on error. This EGC must be free()d by the caller. The attrword and
// channels are written to 'attrword' and 'channels', respectively.
API char* ncplane_at_yx(const struct ncplane* n, int y, int x,
                        uint32_t* attrword, uint64_t* channels);

// Retrieve the current contents of the specified cell into 'c'. This cell is
// invalidated if the associated plane is destroyed.
static inline int
ncplane_at_yx_cell(struct ncplane* n, int y, int x, cell* c){
  char* egc = ncplane_at_yx(n, y, x, &c->attrword, &c->channels);
  if(!egc){
    return -1;
  }
  uint64_t channels = c->channels; // need to preserve wide flag
  int r = cell_load(n, c, egc);
  c->channels = channels;
  free(egc);
  return r;
}

// Create a flat string from the EGCs of the selected region of the ncplane
// 'nc'. Start at the plane's 'begy'x'begx' coordinate (which must lie on the
// plane), continuing for 'leny'x'lenx' cells. Either or both of 'leny' and
// 'lenx' can be specified as -1 to go through the boundary of the plane.
API char* ncplane_contents(const struct ncplane* nc, int begy, int begx,
                           int leny, int lenx);

// Manipulate the opaque user pointer associated with this plane.
// ncplane_set_userptr() returns the previous userptr after replacing
// it with 'opaque'. the others simply return the userptr.
API void* ncplane_set_userptr(struct ncplane* n, void* opaque);
API void* ncplane_userptr(struct ncplane* n);

API void ncplane_center_abs(const struct ncplane* n, int* RESTRICT y,
                            int* RESTRICT x);

// Return the column at which 'c' cols ought start in order to be aligned
// according to 'align' within ncplane 'n'. Returns INT_MAX on invalid 'align'.
// Undefined behavior on negative 'c'.
static inline int
ncplane_align(const struct ncplane* n, ncalign_e align, int c){
  if(align == NCALIGN_LEFT){
    return 0;
  }
  int cols = ncplane_dim_x(n);
  if(c > cols){
    return 0;
  }
  if(align == NCALIGN_CENTER){
    return (cols - c) / 2;
  }else if(align == NCALIGN_RIGHT){
    return cols - c;
  }
  return INT_MAX;
}

// Move the cursor to the specified position (the cursor needn't be visible).
// Returns -1 on error, including negative parameters, or ones exceeding the
// plane's dimensions.
API int ncplane_cursor_move_yx(struct ncplane* n, int y, int x);

// Move the cursor to 0, 0. Can't fail.
API void ncplane_home(struct ncplane* n);

// Get the current position of the cursor within n. y and/or x may be NULL.
API void ncplane_cursor_yx(const struct ncplane* n, int* RESTRICT y, int* RESTRICT x);

// Get the current channels or attribute word for ncplane 'n'.
API uint64_t ncplane_channels(const struct ncplane* n);
API uint32_t ncplane_attr(const struct ncplane* n);

// Replace the cell at the specified coordinates with the provided cell 'c',
// and advance the cursor by the width of the cell (but not past the end of the
// plane). On success, returns the number of columns the cursor was advanced.
// On failure, -1 is returned.
API int ncplane_putc_yx(struct ncplane* n, int y, int x, const cell* c);

// Call ncplane_putc_yx() for the current cursor location.
static inline int
ncplane_putc(struct ncplane* n, const cell* c){
  return ncplane_putc_yx(n, -1, -1, c);
}

// Replace the EGC underneath us, but retain the styling. The current styling
// of the plane will not be changed.
//
// Replace the cell at the specified coordinates with the provided 7-bit char
// 'c'. Advance the cursor by 1. On success, returns 1. On failure, returns -1.
// This works whether the underlying char is signed or unsigned.
static inline int
ncplane_putsimple_yx(struct ncplane* n, int y, int x, char c){
  cell ce = CELL_INITIALIZER((uint32_t)c, ncplane_attr(n), ncplane_channels(n));
  if(!cell_simple_p(&ce)){
    return -1;
  }
  return ncplane_putc_yx(n, y, x, &ce);
}

// Call ncplane_putsimple_yx() at the current cursor location.
static inline int
ncplane_putsimple(struct ncplane* n, char c){
  return ncplane_putsimple_yx(n, -1, -1, c);
}

// Replace the EGC underneath us, but retain the styling. The current styling
// of the plane will not be changed.
API int ncplane_putsimple_stainable(struct ncplane* n, char c);

// Replace the cell at the specified coordinates with the provided EGC, and
// advance the cursor by the width of the cluster (but not past the end of the
// plane). On success, returns the number of columns the cursor was advanced.
// On failure, -1 is returned. The number of bytes converted from gclust is
// written to 'sbytes' if non-NULL.
API int ncplane_putegc_yx(struct ncplane* n, int y, int x, const char* gclust, int* sbytes);

// Call ncplane_putegc() at the current cursor location.
static inline int
ncplane_putegc(struct ncplane* n, const char* gclust, int* sbytes){
  return ncplane_putegc_yx(n, -1, -1, gclust, sbytes);
}

// Replace the EGC underneath us, but retain the styling. The current styling
// of the plane will not be changed.
API int ncplane_putegc_stainable(struct ncplane* n, const char* gclust, int* sbytes);

// 0x0--0x10ffff can be UTF-8-encoded with only 4 bytes...but we aren't
// yet actively guarding against higher values getting into wcstombs FIXME
#define WCHAR_MAX_UTF8BYTES 6

// ncplane_putegc(), but following a conversion from wchar_t to UTF-8 multibyte.
static inline int
ncplane_putwegc(struct ncplane* n, const wchar_t* gclust, int* sbytes){
  // maximum of six UTF8-encoded bytes per wchar_t
  const size_t mbytes = (wcslen(gclust) * WCHAR_MAX_UTF8BYTES) + 1;
  char* mbstr = (char*)malloc(mbytes); // need cast for c++ callers
  if(mbstr == NULL){
    return -1;
  }
  size_t s = wcstombs(mbstr, gclust, mbytes);
  if(s == (size_t)-1){
    free(mbstr);
    return -1;
  }
  int ret = ncplane_putegc(n, mbstr, sbytes);
  free(mbstr);
  return ret;
}

// Call ncplane_putwegc() after successfully moving to y, x.
static inline int
ncplane_putwegc_yx(struct ncplane* n, int y, int x, const wchar_t* gclust,
                   int* sbytes){
  if(ncplane_cursor_move_yx(n, y, x)){
    return -1;
  }
  return ncplane_putwegc(n, gclust, sbytes);
}

// Replace the EGC underneath us, but retain the styling. The current styling
// of the plane will not be changed.
API int ncplane_putwegc_stainable(struct ncplane* n, const wchar_t* gclust, int* sbytes);

// Write a series of EGCs to the current location, using the current style.
// They will be interpreted as a series of columns (according to the definition
// of ncplane_putc()). Advances the cursor by some positive number of columns
// (though not beyond the end of the plane); this number is returned on success.
// On error, a non-positive number is returned, indicating the number of columns
// which were written before the error.
API int ncplane_putstr_yx(struct ncplane* n, int y, int x, const char* gclusters);

static inline int
ncplane_putstr(struct ncplane* n, const char* gclustarr){
  return ncplane_putstr_yx(n, -1, -1, gclustarr);
}

API int ncplane_putstr_aligned(struct ncplane* n, int y, ncalign_e align,
                               const char* s);

// Write a series of EGCs to the current location, using the current style.
// They will be interpreted as a series of columns (according to the definition
// of ncplane_putc()). Advances the cursor by some positive number of columns
// (though not beyond the end of the plane); this number is returned on success.
// On error, a non-positive number is returned, indicating the number of columns
// which were written before the error. No more than 's' bytes will be written.
API int ncplane_putnstr_yx(struct ncplane* n, int y, int x, size_t s, const char* gclusters);

static inline int
ncplane_putnstr(struct ncplane* n, size_t s, const char* gclustarr){
  return ncplane_putnstr_yx(n, -1, -1, s, gclustarr);
}

API int ncplane_putnstr_aligned(struct ncplane* n, int y, ncalign_e align,
                                size_t s, const char* gclustarr);

// ncplane_putstr(), but following a conversion from wchar_t to UTF-8 multibyte.
static inline int
ncplane_putwstr_yx(struct ncplane* n, int y, int x, const wchar_t* gclustarr){
  // maximum of six UTF8-encoded bytes per wchar_t
  const size_t mbytes = (wcslen(gclustarr) * WCHAR_MAX_UTF8BYTES) + 1;
  char* mbstr = (char*)malloc(mbytes); // need cast for c++ callers
  if(mbstr == NULL){
    return -1;
  }
  size_t s = wcstombs(mbstr, gclustarr, mbytes);
  if(s == (size_t)-1){
    free(mbstr);
    return -1;
  }
  int ret = ncplane_putstr_yx(n, y, x, mbstr);
  free(mbstr);
  return ret;
}

static inline int
ncplane_putwstr_aligned(struct ncplane* n, int y, ncalign_e align,
                        const wchar_t* gclustarr){
  int width = wcswidth(gclustarr, INT_MAX);
  int xpos = ncplane_align(n, align, width);
  return ncplane_putwstr_yx(n, y, xpos, gclustarr);
}

static inline int
ncplane_putwstr(struct ncplane* n, const wchar_t* gclustarr){
  return ncplane_putwstr_yx(n, -1, -1, gclustarr);
}

// Replace the cell at the specified coordinates with the provided wide char
// 'w'. Advance the cursor by the character's width as reported by wcwidth().
// On success, returns 1. On failure, returns -1.
static inline int
ncplane_putwc_yx(struct ncplane* n, int y, int x, wchar_t w){
  wchar_t warr[2] = { w, L'\0' };
  return ncplane_putwstr_yx(n, y, x, warr);
}

// Call ncplane_putwc() at the current cursor position.
static inline int
ncplane_putwc(struct ncplane* n, wchar_t w){
  return ncplane_putwc_yx(n, -1, -1, w);
}

// The ncplane equivalents of printf(3) and vprintf(3).
API int ncplane_vprintf_aligned(struct ncplane* n, int y, ncalign_e align,
                                const char* format, va_list ap);

API int ncplane_vprintf_yx(struct ncplane* n, int y, int x,
                           const char* format, va_list ap);

static inline int
ncplane_vprintf(struct ncplane* n, const char* format, va_list ap){
  return ncplane_vprintf_yx(n, -1, -1, format, ap);
}

static inline int
ncplane_printf(struct ncplane* n, const char* format, ...)
  __attribute__ ((format (printf, 2, 3)));

static inline int
ncplane_printf(struct ncplane* n, const char* format, ...){
  va_list va;
  va_start(va, format);
  int ret = ncplane_vprintf(n, format, va);
  va_end(va);
  return ret;
}

static inline int
ncplane_printf_yx(struct ncplane* n, int y, int x, const char* format, ...)
  __attribute__ ((format (printf, 4, 5)));

static inline int
ncplane_printf_yx(struct ncplane* n, int y, int x, const char* format, ...){
  va_list va;
  va_start(va, format);
  int ret = ncplane_vprintf_yx(n, y, x, format, va);
  va_end(va);
  return ret;
}

static inline int
ncplane_printf_aligned(struct ncplane* n, int y, ncalign_e align,
                       const char* format, ...)
  __attribute__ ((format (printf, 4, 5)));

static inline int
ncplane_printf_aligned(struct ncplane* n, int y, ncalign_e align, const char* format, ...){
  va_list va;
  va_start(va, format);
  int ret = ncplane_vprintf_aligned(n, y, align, format, va);
  va_end(va);
  return ret;
}

// Write the specified text to the plane, breaking lines sensibly, beginning at
// the specified line. Returns the number of columns written. When breaking a
// line, the line will be cleared to the end of the plane (the last line will
// *not* be so cleared). The number of bytes written from the input is written
// to '*bytes' if it is not NULL. Cleared columns are included in the return
// value, but *not* included in the number of bytes written. Leaves the cursor
// at the end of output. A partial write will be accomplished as far as it can;
// determine whether the write completed by inspecting '*bytes'.
API int ncplane_puttext(struct ncplane* n, int y, ncalign_e align,
                        const char* text, size_t* bytes);

// Draw horizontal or vertical lines using the specified cell, starting at the
// current cursor position. The cursor will end at the cell following the last
// cell output (even, perhaps counter-intuitively, when drawing vertical
// lines), just as if ncplane_putc() was called at that spot. Return the
// number of cells drawn on success. On error, return the negative number of
// cells drawn.
API int ncplane_hline_interp(struct ncplane* n, const cell* c, int len,
                             uint64_t c1, uint64_t c2);

static inline int
ncplane_hline(struct ncplane* n, const cell* c, int len){
  return ncplane_hline_interp(n, c, len, c->channels, c->channels);
}

API int ncplane_vline_interp(struct ncplane* n, const cell* c, int len,
                             uint64_t c1, uint64_t c2);

static inline int
ncplane_vline(struct ncplane* n, const cell* c, int len){
  return ncplane_vline_interp(n, c, len, c->channels, c->channels);
}

#define NCBOXMASK_TOP    0x0001
#define NCBOXMASK_RIGHT  0x0002
#define NCBOXMASK_BOTTOM 0x0004
#define NCBOXMASK_LEFT   0x0008
#define NCBOXGRAD_TOP    0x0010
#define NCBOXGRAD_RIGHT  0x0020
#define NCBOXGRAD_BOTTOM 0x0040
#define NCBOXGRAD_LEFT   0x0080
#define NCBOXCORNER_MASK 0x0300
#define NCBOXCORNER_SHIFT 8u

// Draw a box with its upper-left corner at the current cursor position, and its
// lower-right corner at 'ystop'x'xstop'. The 6 cells provided are used to draw the
// upper-left, ur, ll, and lr corners, then the horizontal and vertical lines.
// 'ctlword' is defined in the least significant byte, where bits [7, 4] are a
// gradient mask, and [3, 0] are a border mask:
//  * 7, 3: top
//  * 6, 2: right
//  * 5, 1: bottom
//  * 4, 0: left
// If the gradient bit is not set, the styling from the hl/vl cells is used for
// the horizontal and vertical lines, respectively. If the gradient bit is set,
// the color is linearly interpolated between the two relevant corner cells.
//
// By default, vertexes are drawn whether their connecting edges are drawn or
// not. The value of the bits corresponding to NCBOXCORNER_MASK control this,
// and are interpreted as the number of connecting edges necessary to draw a
// given corner. At 0 (the default), corners are always drawn. At 3, corners
// are never drawn (as at most 2 edges can touch a box's corner).
API int ncplane_box(struct ncplane* n, const cell* ul, const cell* ur,
                    const cell* ll, const cell* lr, const cell* hline,
                    const cell* vline, int ystop, int xstop,
                    unsigned ctlword);

// Draw a box with its upper-left corner at the current cursor position, having
// dimensions 'ylen'x'xlen'. See ncplane_box() for more information. The
// minimum box size is 2x2, and it cannot be drawn off-screen.
static inline int
ncplane_box_sized(struct ncplane* n, const cell* ul, const cell* ur,
                  const cell* ll, const cell* lr, const cell* hline,
                  const cell* vline, int ylen, int xlen, unsigned ctlword){
  int y, x;
  ncplane_cursor_yx(n, &y, &x);
  return ncplane_box(n, ul, ur, ll, lr, hline, vline, y + ylen - 1,
                     x + xlen - 1, ctlword);
}

static inline int
ncplane_perimeter(struct ncplane* n, const cell* ul, const cell* ur,
                  const cell* ll, const cell* lr, const cell* hline,
                  const cell* vline, unsigned ctlword){
  if(ncplane_cursor_move_yx(n, 0, 0)){
    return -1;
  }
  int dimy, dimx;
  ncplane_dim_yx(n, &dimy, &dimx);
  return ncplane_box_sized(n, ul, ur, ll, lr, hline, vline, dimy, dimx, ctlword);
}

// Starting at the specified coordinate, if its glyph is different from that of
// 'c', 'c' is copied into it, and the original glyph is considered the fill
// target. We do the same to all cardinally-connected cells having this same
// fill target. Returns the number of cells polyfilled. An invalid initial y, x
// is an error. Returns the number of cells filled, or -1 on error.
API int ncplane_polyfill_yx(struct ncplane* n, int y, int x, const cell* c);

// Draw a gradient with its upper-left corner at the current cursor position,
// stopping at 'ystop'x'xstop'. The glyph composed of 'egc' and 'attrword' is
// used for all cells. The channels specified by 'ul', 'ur', 'll', and 'lr'
// are composed into foreground and background gradients. To do a vertical
// gradient, 'ul' ought equal 'ur' and 'll' ought equal 'lr'. To do a
// horizontal gradient, 'ul' ought equal 'll' and 'ur' ought equal 'ul'. To
// color everything the same, all four channels should be equivalent. The
// resulting alpha values are equal to incoming alpha values. Returns the
// number of cells filled on success, or -1 on failure.
//
// Palette-indexed color is not supported.
//
// Preconditions for gradient operations (error otherwise):
//
//  all: only RGB colors, unless all four channels match as default
//  all: all alpha values must be the same
//  1x1: all four colors must be the same
//  1xN: both top and both bottom colors must be the same (vertical gradient)
//  Nx1: both left and both right colors must be the same (horizontal gradient)
API int ncplane_gradient(struct ncplane* n, const char* egc, uint32_t attrword,
                         uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr,
                         int ystop, int xstop);

// Do a high-resolution gradient using upper blocks and synced backgrounds.
// This doubles the number of vertical gradations, but restricts you to
// half blocks (appearing to be full blocks). Returns the number of cells
// filled on success, or -1 on error.
API int ncplane_highgradient(struct ncplane* n, uint32_t ul, uint32_t ur,
                             uint32_t ll, uint32_t lr, int ystop, int xstop);

// Draw a gradient with its upper-left corner at the current cursor position,
// having dimensions 'ylen'x'xlen'. See ncplane_gradient for more information.
static inline int
ncplane_gradient_sized(struct ncplane* n, const char* egc, uint32_t attrword,
                       uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr,
                       int ylen, int xlen){
  if(ylen < 1 || xlen < 1){
    return -1;
  }
  int y, x;
  ncplane_cursor_yx(n, &y, &x);
  return ncplane_gradient(n, egc, attrword, ul, ur, ll, lr, y + ylen - 1, x + xlen - 1);
}

static inline int
ncplane_highgradient_sized(struct ncplane* n, uint32_t ul, uint32_t ur,
                           uint32_t ll, uint32_t lr, int ylen, int xlen){
  if(ylen < 1 || xlen < 1){
    return -1;
  }
  int y, x;
  if(!notcurses_canutf8(ncplane_notcurses_const(n))){
    // this works because the uin32_ts we pass in will be promoted to uint64_ts
    // via extension, and the space will employ the background. mwahh!
    return ncplane_gradient_sized(n, " ", 0, ul, ur, ll, lr, ylen, xlen);
  }
  ncplane_cursor_yx(n, &y, &x);
  return ncplane_highgradient(n, ul, ur, ll, lr, y + ylen - 1, x + xlen - 1);
}

// Set the given style throughout the specified region, keeping content and
// channels unchanged. Returns the number of cells set, or -1 on failure.
API int ncplane_format(struct ncplane* n, int ystop, int xstop, uint32_t attrword);

// Set the given channels throughout the specified region, keeping content and
// attributes unchanged. Returns the number of cells set, or -1 on failure.
API int ncplane_stain(struct ncplane* n, int ystop, int xstop, uint64_t ul,
                      uint64_t ur, uint64_t ll, uint64_t lr);

// Merge the ncplane 'src' down onto the ncplane 'dst'. This is most rigorously
// defined as "write to 'dst' the frame that would be rendered were the entire
// stack made up only of 'src' and, below it, 'dst', and 'dst' was the entire
// rendering region." Merging is independent of the position of 'src' viz 'dst'
// on the z-axis. If 'src' does not intersect with 'dst', 'dst' will not be
// changed, but it is not an error. The source plane still exists following
// this operation. If 'dst' is NULL, it will be interpreted as the standard
// plane. Do not supply the same plane for both 'src' and 'dst'.
API int ncplane_mergedown(struct ncplane* RESTRICT src, struct ncplane* RESTRICT dst);

// Erase every cell in the ncplane, resetting all attributes to normal, all
// colors to the default color, and all cells to undrawn. All cells associated
// with this ncplane is invalidated, and must not be used after the call,
// *excluding* the base cell. The cursor is homed.
API void ncplane_erase(struct ncplane* n);

#define NCPALETTESIZE 256

// Returns the result of blending two channels. 'blends' indicates how heavily
// 'c1' ought be weighed. If 'blends' is 0, 'c1' will be entirely replaced by
// 'c2'. If 'c1' is otherwise the default color, 'c1' will not be touched,
// since we can't blend default colors. Likewise, if 'c2' is a default color,
// it will not be used (unless 'blends' is 0).
//
// Palette-indexed colors do not blend, and since we need the attrword to store
// them, we just don't fuck wit' 'em here. Do not pass me palette-indexed
// channels! I will eat them.
static inline unsigned
channels_blend(unsigned c1, unsigned c2, unsigned* blends){
  if(channel_alpha(c2) == CELL_ALPHA_TRANSPARENT){
    return c1; // do *not* increment *blends
  }
  unsigned rsum, gsum, bsum;
  channel_rgb(c2, &rsum, &gsum, &bsum);
  bool c2default = channel_default_p(c2);
  if(*blends == 0){
    // don't just return c2, or you set wide status and all kinds of crap
    if(channel_default_p(c2)){
      channel_set_default(&c1);
    }else{
      channel_set_rgb(&c1, rsum, gsum, bsum);
    }
    channel_set_alpha(&c1, channel_alpha(c2));
  }else if(!c2default && !channel_default_p(c1)){
    rsum = (channel_r(c1) * *blends + rsum) / (*blends + 1);
    gsum = (channel_g(c1) * *blends + gsum) / (*blends + 1);
    bsum = (channel_b(c1) * *blends + bsum) / (*blends + 1);
    channel_set_rgb(&c1, rsum, gsum, bsum);
    channel_set_alpha(&c1, channel_alpha(c2));
  }
  ++*blends;
  return c1;
}

// Extract the 32-bit background channel from a cell.
static inline unsigned
cell_bchannel(const cell* cl){
  return channels_bchannel(cl->channels);
}

// Extract the 32-bit foreground channel from a cell.
static inline unsigned
cell_fchannel(const cell* cl){
  return channels_fchannel(cl->channels);
}

// Set the 32-bit background channel of a cell.
static inline uint64_t
cell_set_bchannel(cell* cl, uint32_t channel){
  return channels_set_bchannel(&cl->channels, channel);
}

// Set the 32-bit foreground channel of a cell.
static inline uint64_t
cell_set_fchannel(cell* cl, uint32_t channel){
  return channels_set_fchannel(&cl->channels, channel);
}

// do not pass palette-indexed channels!
static inline uint64_t
cell_blend_fchannel(cell* cl, unsigned channel, unsigned* blends){
  return cell_set_fchannel(cl, channels_blend(cell_fchannel(cl), channel, blends));
}

static inline uint64_t
cell_blend_bchannel(cell* cl, unsigned channel, unsigned* blends){
  return cell_set_bchannel(cl, channels_blend(cell_bchannel(cl), channel, blends));
}

// Extract 24 bits of foreground RGB from 'cell', shifted to LSBs.
static inline unsigned
cell_fg(const cell* cl){
  return channels_fg(cl->channels);
}

// Extract 24 bits of background RGB from 'cell', shifted to LSBs.
static inline unsigned
cell_bg(const cell* cl){
  return channels_bg(cl->channels);
}

// Extract 2 bits of foreground alpha from 'cell', shifted to LSBs.
static inline unsigned
cell_fg_alpha(const cell* cl){
  return channels_fg_alpha(cl->channels);
}

// Extract 2 bits of background alpha from 'cell', shifted to LSBs.
static inline unsigned
cell_bg_alpha(const cell* cl){
  return channels_bg_alpha(cl->channels);
}

// Extract 24 bits of foreground RGB from 'cell', split into components.
static inline unsigned
cell_fg_rgb(const cell* cl, unsigned* r, unsigned* g, unsigned* b){
  return channels_fg_rgb(cl->channels, r, g, b);
}

// Extract 24 bits of background RGB from 'cell', split into components.
static inline unsigned
cell_bg_rgb(const cell* cl, unsigned* r, unsigned* g, unsigned* b){
  return channels_bg_rgb(cl->channels, r, g, b);
}

// Set the r, g, and b cell for the foreground component of this 64-bit
// 'cell' variable, and mark it as not using the default color.
static inline int
cell_set_fg_rgb(cell* cl, int r, int g, int b){
  return channels_set_fg_rgb(&cl->channels, r, g, b);
}

// Same, but clipped to [0..255].
static inline void
cell_set_fg_rgb_clipped(cell* cl, int r, int g, int b){
  channels_set_fg_rgb_clipped(&cl->channels, r, g, b);
}

// Same, but with an assembled 24-bit RGB value.
static inline int
cell_set_fg(cell* c, uint32_t channel){
  return channels_set_fg(&c->channels, channel);
}

// Set the cell's foreground palette index, set the foreground palette index
// bit, set it foreground-opaque, and clear the foreground default color bit.
static inline int
cell_set_fg_palindex(cell* cl, int idx){
  if(idx < 0 || idx >= NCPALETTESIZE){
    return -1;
  }
  cl->channels |= CELL_FGDEFAULT_MASK;
  cl->channels |= CELL_FG_PALETTE;
  cell_set_fg_alpha(cl, CELL_ALPHA_OPAQUE);
  cl->attrword &= 0xffff00ff;
  cl->attrword |= (idx << 8u);
  return 0;
}

static inline unsigned
cell_fg_palindex(const cell* cl){
  return (cl->attrword & 0x0000ff00) >> 8u;
}

// Set the r, g, and b cell for the background component of this 64-bit
// 'cell' variable, and mark it as not using the default color.
static inline int
cell_set_bg_rgb(cell* cl, int r, int g, int b){
  return channels_set_bg_rgb(&cl->channels, r, g, b);
}

// Same, but clipped to [0..255].
static inline void
cell_set_bg_rgb_clipped(cell* cl, int r, int g, int b){
  channels_set_bg_rgb_clipped(&cl->channels, r, g, b);
}

// Same, but with an assembled 24-bit RGB value. A value over 0xffffff
// will be rejected, with a non-zero return value.
static inline int
cell_set_bg(cell* c, uint32_t channel){
  return channels_set_bg(&c->channels, channel);
}

// Set the cell's background palette index, set the background palette index
// bit, set it background-opaque, and clear the background default color bit.
static inline int
cell_set_bg_palindex(cell* cl, int idx){
  if(idx < 0 || idx >= NCPALETTESIZE){
    return -1;
  }
  cl->channels |= CELL_BGDEFAULT_MASK;
  cl->channels |= CELL_BG_PALETTE;
  cell_set_bg_alpha(cl, CELL_ALPHA_OPAQUE);
  cl->attrword &= 0xffffff00;
  cl->attrword |= idx;
  return 0;
}

static inline unsigned
cell_bg_palindex(const cell* cl){
  return cl->attrword & 0x000000ff;
}

// Is the foreground using the "default foreground color"?
static inline bool
cell_fg_default_p(const cell* cl){
  return channels_fg_default_p(cl->channels);
}

static inline bool
cell_fg_palindex_p(const cell* cl){
  return channels_fg_palindex_p(cl->channels);
}

// Is the background using the "default background color"? The "default
// background color" must generally be used to take advantage of
// terminal-effected transparency.
static inline bool
cell_bg_default_p(const cell* cl){
  return channels_bg_default_p(cl->channels);
}

static inline bool
cell_bg_palindex_p(const cell* cl){
  return channels_bg_palindex_p(cl->channels);
}

// Extract the 32-bit working background channel from an ncplane.
static inline unsigned
ncplane_bchannel(const struct ncplane* nc){
  return channels_bchannel(ncplane_channels(nc));
}

// Extract the 32-bit working foreground channel from an ncplane.
static inline unsigned
ncplane_fchannel(const struct ncplane* nc){
  return channels_fchannel(ncplane_channels(nc));
}

API void ncplane_set_channels(struct ncplane* nc, uint64_t channels);

API void ncplane_set_attr(struct ncplane* nc, uint32_t attrword);

// Extract 24 bits of working foreground RGB from an ncplane, shifted to LSBs.
static inline unsigned
ncplane_fg(const struct ncplane* nc){
  return channels_fg(ncplane_channels(nc));
}

// Extract 24 bits of working background RGB from an ncplane, shifted to LSBs.
static inline unsigned
ncplane_bg(const struct ncplane* nc){
  return channels_bg(ncplane_channels(nc));
}

// Extract 2 bits of foreground alpha from 'struct ncplane', shifted to LSBs.
static inline unsigned
ncplane_fg_alpha(const struct ncplane* nc){
  return channels_fg_alpha(ncplane_channels(nc));
}

// Is the plane's foreground using the "default foreground color"?
static inline bool
ncplane_fg_default_p(const struct ncplane* nc){
  return channels_fg_default_p(ncplane_channels(nc));
}

// Extract 2 bits of background alpha from 'struct ncplane', shifted to LSBs.
static inline unsigned
ncplane_bg_alpha(const struct ncplane* nc){
  return channels_bg_alpha(ncplane_channels(nc));
}

// Is the plane's background using the "default background color"?
static inline bool
ncplane_bg_default_p(const struct ncplane* nc){
  return channels_bg_default_p(ncplane_channels(nc));
}

// Extract 24 bits of foreground RGB from 'n', split into components.
static inline unsigned
ncplane_fg_rgb(const struct ncplane* n, unsigned* r, unsigned* g, unsigned* b){
  return channels_fg_rgb(ncplane_channels(n), r, g, b);
}

// Extract 24 bits of background RGB from 'n', split into components.
static inline unsigned
ncplane_bg_rgb(const struct ncplane* n, unsigned* r, unsigned* g, unsigned* b){
  return channels_bg_rgb(ncplane_channels(n), r, g, b);
}

// Set the current fore/background color using RGB specifications. If the
// terminal does not support directly-specified 3x8b cells (24-bit "TrueColor",
// indicated by the "RGB" terminfo capability), the provided values will be
// interpreted in some lossy fashion. None of r, g, or b may exceed 255.
// "HP-like" terminals require setting foreground and background at the same
// time using "color pairs"; notcurses will manage color pairs transparently.
API int ncplane_set_fg_rgb(struct ncplane* n, int r, int g, int b);
API int ncplane_set_bg_rgb(struct ncplane* n, int r, int g, int b);

// Same, but clipped to [0..255].
API void ncplane_set_bg_rgb_clipped(struct ncplane* n, int r, int g, int b);
API void ncplane_set_fg_rgb_clipped(struct ncplane* n, int r, int g, int b);

// Same, but with rgb assembled into a channel (i.e. lower 24 bits).
API int ncplane_set_fg(struct ncplane* n, unsigned channel);
API int ncplane_set_bg(struct ncplane* n, unsigned channel);

// Use the default color for the foreground/background.
API void ncplane_set_fg_default(struct ncplane* n);
API void ncplane_set_bg_default(struct ncplane* n);

// Set the ncplane's foreground palette index, set the foreground palette index
// bit, set it foreground-opaque, and clear the foreground default color bit.
API int ncplane_set_fg_palindex(struct ncplane* n, int idx);
API int ncplane_set_bg_palindex(struct ncplane* n, int idx);

// Set the alpha parameters for ncplane 'n'.
API int ncplane_set_fg_alpha(struct ncplane* n, int alpha);
API int ncplane_set_bg_alpha(struct ncplane* n, int alpha);

// Set the specified style bits for the ncplane 'n', whether they're actively
// supported or not.
API void ncplane_styles_set(struct ncplane* n, unsigned stylebits);

// Add the specified styles to the ncplane's existing spec.
API void ncplane_styles_on(struct ncplane* n, unsigned stylebits);

// Remove the specified styles from the ncplane's existing spec.
API void ncplane_styles_off(struct ncplane* n, unsigned stylebits);

// Return the current styling for this ncplane.
API unsigned ncplane_styles(const struct ncplane* n);

// Called for each fade iteration on 'ncp'. If anything but 0 is returned,
// the fading operation ceases immediately, and that value is propagated out.
// The recommended absolute display time target is passed in 'tspec'.
typedef int (*fadecb)(struct notcurses* nc, struct ncplane* ncp,
                      const struct timespec*, void* curry);

// Fade the ncplane out over the provided time, calling 'fader' at each
// iteration. Requires a terminal which supports truecolor, or at least palette
// modification (if the terminal uses a palette, our ability to fade planes is
// limited, and affected by the complexity of the rest of the screen).
API int ncplane_fadeout(struct ncplane* n, const struct timespec* ts,
                        fadecb fader, void* curry);

// Fade the ncplane in over the specified time. Load the ncplane with the
// target cells without rendering, then call this function. When it's done, the
// ncplane will have reached the target levels, starting from zeroes.
API int ncplane_fadein(struct ncplane* n, const struct timespec* ts,
                       fadecb fader, void* curry);

// Rather than the simple ncplane_fade{in/out}(), ncfadectx_setup() can be
// paired with a loop over ncplane_fade{in/out}_iteration() + ncfadectx_free().
API struct ncfadectx* ncfadectx_setup(struct ncplane* n);

// Return the number of iterations through which 'nctx' will fade.
API int ncfadectx_iterations(const struct ncfadectx* nctx);

// Fade out through 'iter' iterations, where
// 'iter' < 'ncfadectx_iterations(nctx)'.
API int ncplane_fadeout_iteration(struct ncplane* n, struct ncfadectx* nctx,
                                  int iter, fadecb fader, void* curry);

// Fade in through 'iter' iterations, where
// 'iter' < 'ncfadectx_iterations(nctx)'.
API int ncplane_fadein_iteration(struct ncplane* n, struct ncfadectx* nctx,
                                  int iter, fadecb fader, void* curry);

// Pulse the plane in and out until the callback returns non-zero, relying on
// the callback 'fader' to initiate rendering. 'ts' defines the half-period
// (i.e. the transition from black to full brightness, or back again). Proper
// use involves preparing (but not rendering) an ncplane, then calling
// ncplane_pulse(), which will fade in from black to the specified colors.
API int ncplane_pulse(struct ncplane* n, const struct timespec* ts, fadecb fader, void* curry);

// Release the resources associated with 'nctx'.
API void ncfadectx_free(struct ncfadectx* nctx);

// load up six cells with the EGCs necessary to draw a box. returns 0 on
// success, -1 on error. on error, any cells this function might
// have loaded before the error are cell_release()d. There must be at least
// six EGCs in gcluster.
static inline int
cells_load_box(struct ncplane* n, uint32_t attrs, uint64_t channels,
               cell* ul, cell* ur, cell* ll, cell* lr,
               cell* hl, cell* vl, const char* gclusters){
  int ulen;
  if((ulen = cell_prime(n, ul, gclusters, attrs, channels)) > 0){
    if((ulen = cell_prime(n, ur, gclusters += ulen, attrs, channels)) > 0){
      if((ulen = cell_prime(n, ll, gclusters += ulen, attrs, channels)) > 0){
        if((ulen = cell_prime(n, lr, gclusters += ulen, attrs, channels)) > 0){
          if((ulen = cell_prime(n, hl, gclusters += ulen, attrs, channels)) > 0){
            if((ulen = cell_prime(n, vl, gclusters += ulen, attrs, channels)) > 0){
              return 0;
            }
            cell_release(n, hl);
          }
          cell_release(n, lr);
        }
        cell_release(n, ll);
      }
      cell_release(n, ur);
    }
    cell_release(n, ul);
  }
  return -1;
}

API int cells_rounded_box(struct ncplane* n, uint32_t attr, uint64_t channels,
                          cell* ul, cell* ur, cell* ll, cell* lr,
                          cell* hl, cell* vl);

static inline int
ncplane_rounded_box(struct ncplane* n, uint32_t attr, uint64_t channels,
                    int ystop, int xstop, unsigned ctlword){
  int ret = 0;
  cell ul = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
  cell ll = CELL_TRIVIAL_INITIALIZER, lr = CELL_TRIVIAL_INITIALIZER;
  cell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
  if((ret = cells_rounded_box(n, attr, channels, &ul, &ur, &ll, &lr, &hl, &vl)) == 0){
    ret = ncplane_box(n, &ul, &ur, &ll, &lr, &hl, &vl, ystop, xstop, ctlword);
  }
  cell_release(n, &ul); cell_release(n, &ur);
  cell_release(n, &ll); cell_release(n, &lr);
  cell_release(n, &hl); cell_release(n, &vl);
  return ret;
}

static inline int
ncplane_perimeter_rounded(struct ncplane* n, uint32_t attrword,
                          uint64_t channels, unsigned ctlword){
  if(ncplane_cursor_move_yx(n, 0, 0)){
    return -1;
  }
  int dimy, dimx;
  ncplane_dim_yx(n, &dimy, &dimx);
  cell ul = CELL_TRIVIAL_INITIALIZER;
  cell ur = CELL_TRIVIAL_INITIALIZER;
  cell ll = CELL_TRIVIAL_INITIALIZER;
  cell lr = CELL_TRIVIAL_INITIALIZER;
  cell vl = CELL_TRIVIAL_INITIALIZER;
  cell hl = CELL_TRIVIAL_INITIALIZER;
  if(cells_rounded_box(n, attrword, channels, &ul, &ur, &ll, &lr, &hl, &vl)){
    return -1;
  }
  int r = ncplane_box_sized(n, &ul, &ur, &ll, &lr, &hl, &vl, dimy, dimx, ctlword);
  cell_release(n, &ul); cell_release(n, &ur);
  cell_release(n, &ll); cell_release(n, &lr);
  cell_release(n, &hl); cell_release(n, &vl);
  return r;
}

static inline int
ncplane_rounded_box_sized(struct ncplane* n, uint32_t attr, uint64_t channels,
                          int ylen, int xlen, unsigned ctlword){
  int y, x;
  ncplane_cursor_yx(n, &y, &x);
  return ncplane_rounded_box(n, attr, channels, y + ylen - 1,
                             x + xlen - 1, ctlword);
}

API int cells_double_box(struct ncplane* n, uint32_t attr, uint64_t channels,
                         cell* ul, cell* ur, cell* ll, cell* lr,
                         cell* hl, cell* vl);

static inline int
ncplane_double_box(struct ncplane* n, uint32_t attr, uint64_t channels,
                   int ystop, int xstop, unsigned ctlword){
  int ret = 0;
  cell ul = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
  cell ll = CELL_TRIVIAL_INITIALIZER, lr = CELL_TRIVIAL_INITIALIZER;
  cell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
  if((ret = cells_double_box(n, attr, channels, &ul, &ur, &ll, &lr, &hl, &vl)) == 0){
    ret = ncplane_box(n, &ul, &ur, &ll, &lr, &hl, &vl, ystop, xstop, ctlword);
  }
  cell_release(n, &ul); cell_release(n, &ur);
  cell_release(n, &ll); cell_release(n, &lr);
  cell_release(n, &hl); cell_release(n, &vl);
  return ret;
}

static inline int
ncplane_perimeter_double(struct ncplane* n, uint32_t attrword,
                         uint64_t channels, unsigned ctlword){
  if(ncplane_cursor_move_yx(n, 0, 0)){
    return -1;
  }
  int dimy, dimx;
  ncplane_dim_yx(n, &dimy, &dimx);
  cell ul = CELL_TRIVIAL_INITIALIZER;
  cell ur = CELL_TRIVIAL_INITIALIZER;
  cell ll = CELL_TRIVIAL_INITIALIZER;
  cell lr = CELL_TRIVIAL_INITIALIZER;
  cell vl = CELL_TRIVIAL_INITIALIZER;
  cell hl = CELL_TRIVIAL_INITIALIZER;
  if(cells_double_box(n, attrword, channels, &ul, &ur, &ll, &lr, &hl, &vl)){
    return -1;
  }
  int r = ncplane_box_sized(n, &ul, &ur, &ll, &lr, &hl, &vl, dimy, dimx, ctlword);
  cell_release(n, &ul); cell_release(n, &ur);
  cell_release(n, &ll); cell_release(n, &lr);
  cell_release(n, &hl); cell_release(n, &vl);
  return r;
}

static inline int
ncplane_double_box_sized(struct ncplane* n, uint32_t attr, uint64_t channels,
                         int ylen, int xlen, unsigned ctlword){
  int y, x;
  ncplane_cursor_yx(n, &y, &x);
  return ncplane_double_box(n, attr, channels, y + ylen - 1,
                            x + xlen - 1, ctlword);
}

// Open a visual at 'file', extract a codec and parameters, decode the first
// image to memory.
API struct ncvisual* ncvisual_from_file(const char* file, nc_err_e* ncerr);

// Prepare an ncvisual, and its underlying plane, based off RGBA content in
// memory at 'rgba'. 'rgba' must be a flat array of 32-bit 8bpc RGBA pixels.
// These must be arranged in 'rowstride' lines, where the first 'cols' * 4b
// are actual data. There must be 'rows' lines. The total size of 'rgba'
// must thus be at least (rows * rowstride) bytes, of which (rows * cols * 4)
// bytes are actual data. The resulting plane will be ceil('rows'/2)x'cols'.
API struct ncvisual* ncvisual_from_rgba(const void* rgba, int rows,
                                        int rowstride, int cols);

// ncvisual_from_rgba(), but 'bgra' is arranged as BGRA.
API struct ncvisual* ncvisual_from_bgra(const void* rgba, int rows,
                                        int rowstride, int cols);

// Promote an ncplane 'n' to an ncvisual. The plane may contain only spaces,
// half blocks, and full blocks. The latter will be checked, and any other
// glyph will result in a NULL being returned. This function exists so that
// planes can be subjected to ncvisual transformations. If possible, it's
// better to create the ncvisual from memory using ncvisual_from_rgba().
API struct ncvisual* ncvisual_from_plane(const struct ncplane* n,
                                         ncblitter_e blit,
                                         int begy, int begx,
                                         int leny, int lenx);

#define NCVISUAL_OPTION_NODEGRADE 0x0001ull // fail rather than degrading
#define NCVISUAL_OPTION_BLEND     0x0002ull // use CELL_ALPHA_BLEND with visual

struct ncvisual_options {
  // if no ncplane is provided, one will be created using the exact size
  // necessary to render the source with perfect fidelity (this might be
  // smaller or larger than the rendering area). if provided, style is
  // taken into account, relative to the provided ncplane.
  struct ncplane* n;
  // the style is ignored if no ncplane is provided (it ought be NCSCALE_NONE
  // in this case). otherwise, the source is stretched/scaled relative to the
  // provided ncplane.
  ncscale_e scaling;
  // if an ncplane is provided, y and x specify where the visual will be
  // rendered on that plane. otherwise, they specify where the created ncplane
  // will be placed.
  int y, x;
  // the section of the visual that ought be rendered. for the entire visual,
  // pass an origin of 0, 0 and a size of 0, 0 (or the true height and width).
  // these numbers are all in terms of ncvisual pixels.
  int begy, begx; // origin of rendered section
  int leny, lenx; // size of rendered section
  // use NCBLIT_DEFAULT if you don't care, to use NCBLIT_2x2 (assuming
  // UTF8) or NCBLIT_1x1 (in an ASCII environment)
  ncblitter_e blitter; // glyph set to use (maps input to output cells)
  uint64_t flags; // bitmask over NCVISUAL_OPTION_*
};

// Create an RGBA flat array from the selected region of the ncplane 'nc'.
// Start at the plane's 'begy'x'begx' coordinate (which must lie on the
// plane), continuing for 'leny'x'lenx' cells. Either or both of 'leny' and
// 'lenx' can be specified as -1 to go through the boundary of the plane.
// Only glyphs from the specified blitset may be present.
API uint32_t* ncplane_rgba(const struct ncplane* nc, ncblitter_e blit,
                           int begy, int begx, int leny, int lenx);

// Get the size and ratio of ncvisual pixels to output cells along the y
// ('toy') and x ('tox') axes. A ncvisual of '*y'X'*x' pixels will require
// ('*y' * '*toy')X('x' * 'tox') cells for full output. Returns non-zero
// for an invalid 'vopts->blitter'. Scaling is taken into consideration.
API int ncvisual_geom(const struct notcurses* nc, const struct ncvisual* n,
                      const struct ncvisual_options* vopts,
                      int* y, int* x, int* toy, int* tox);

// Destroy an ncvisual. Rendered elements will not be disrupted, but the visual
// can be neither decoded nor rendered any further.
API void ncvisual_destroy(struct ncvisual* ncv);

// extract the next frame from an ncvisual. returns NCERR_EOF on end of file,
// and NCERR_SUCCESS on success, otherwise some other NCERR.
API nc_err_e ncvisual_decode(struct ncvisual* nc);

// Rotate the visual 'rads' radians. Only M_PI/2 and -M_PI/2 are
// supported at the moment, but this will change FIXME.
API nc_err_e ncvisual_rotate(struct ncvisual* n, double rads);

// Resize the visual so that it is 'rows' X 'columns'. This is a lossy
// transformation, unless the size is unchanged.
API nc_err_e ncvisual_resize(struct ncvisual* n, int rows, int cols);

// Polyfill at the specified location within the ncvisual 'n', using 'rgba'.
API int ncvisual_polyfill_yx(struct ncvisual* n, int y, int x, uint32_t rgba);

// Get the specified pixel from the specified ncvisual.
API int ncvisual_at_yx(const struct ncvisual* n, int y, int x, uint32_t* pixel);

// Set the specified pixel in the specified ncvisual.
API int ncvisual_set_yx(const struct ncvisual* n, int y, int x, uint32_t pixel);

// Render the decoded frame to the specified ncplane (if one is not provided,
// one will be created, having the exact size necessary to display the visual.
// In this case, 'style' must be NCSTYLE_NONE). A subregion of the visual can
// be rendered using 'begx', 'begy', 'lenx', and 'leny'. Negative values for
// 'begy' or 'begx' are an error. It is an error to specify any region beyond
// the boundaries of the frame. Returns the plane to which we drew (if ncv->n
// is NULL, a new plane will be created).
API struct ncplane* ncvisual_render(struct notcurses* nc, struct ncvisual* ncv,
                                    const struct ncvisual_options* vopts);

// If a subtitle ought be displayed at this time, return a heap-allocated copy
// of the UTF8 text.
API char* ncvisual_subtitle(const struct ncvisual* ncv);

// Called for each frame rendered from 'ncv'. If anything but 0 is returned,
// the streaming operation ceases immediately, and that value is propagated out.
// The recommended absolute display time target is passed in 'tspec'.
typedef int (*streamcb)(struct ncvisual*, struct ncvisual_options*,
                        const struct timespec*, void*);

// Shut up and display my frames! Provide as an argument to ncvisual_stream().
// If you'd like subtitles to be decoded, provide an ncplane as the curry. If the
// curry is NULL, subtitles will not be displayed.
API int ncvisual_simple_streamer(struct ncvisual* ncv, struct ncvisual_options* vopts,
                                 const struct timespec* tspec, void* curry);

// Stream the entirety of the media, according to its own timing. Blocking,
// obviously. streamer may be NULL; it is otherwise called for each frame, and
// its return value handled as outlined for streamcb. If streamer() returns
// non-zero, the stream is aborted, and that value is returned. By convention,
// return a positive number to indicate intentional abort from within
// streamer(). 'timescale' allows the frame duration time to be scaled. For a
// visual naturally running at 30FPS, a 'timescale' of 0.1 will result in
// 300FPS, and a 'timescale' of 10 will result in 3FPS. It is an error to
// supply 'timescale' less than or equal to 0.
API int ncvisual_stream(struct notcurses* nc, struct ncvisual* ncv,
                        nc_err_e* ncerr, float timescale, streamcb streamer,
                        const struct ncvisual_options* vopts, void* curry);

// Blit a flat array 'data' of RGBA 32-bit values to the ncplane 'vopts->n',
// which mustn't be NULL. the blit begins at 'vopts->y' and 'vopts->x' relative
// to the specified plane. Each source row ought occupy 'linesize' bytes (this
// might be greater than 'vopts->lenx' * 4 due to padding or partial blits). A
// subregion of the input can be specified with the 'begy'x'begx' and
// 'leny'x'lenx' fields from 'vopts'. Returns the number of pixels blitted, or
// -1 on error.
API int ncblit_rgba(const void* data, int linesize,
                    const struct ncvisual_options* vopts);

// Same as ncblit_rgba(), but for BGRx.
API int ncblit_bgrx(const void* data, int linesize,
                    const struct ncvisual_options* vopts);

// An ncreel is a notcurses region devoted to displaying zero or more
// line-oriented, contained panels between which the user may navigate. If at
// least one panel exists, there is an active panel. As much of the active
// panel as is possible is always displayed. If there is space left over, other
// panels are included in the display. Panels can come and go at any time, and
// can grow or shrink at any time.
//
// This structure is amenable to line- and page-based navigation via keystrokes,
// scrolling gestures, trackballs, scrollwheels, touchpads, and verbal commands.

// is scrolling infinite (can one move down or up forever, or is an end
// reached?). if true, 'circular' specifies how to handle the special case of
// an incompletely-filled reel.
#define NCREEL_OPTION_INFINITESCROLL 0x0001ull
// is navigation circular (does moving down from the last panel move to the
// first, and vice versa)? only meaningful when infinitescroll is true. if
// infinitescroll is false, this must be false.
#define NCREEL_OPTION_CIRCULAR       0x0002ull

typedef struct ncreel_options {
  // notcurses can draw a border around the ncreel, and also around the
  // component tablets. inhibit borders by setting all valid bits in the masks.
  // partially inhibit borders by setting individual bits in the masks. the
  // appropriate attr and pair values will be used to style the borders.
  // focused and non-focused tablets can have different styles. you can instead
  // draw your own borders, or forgo borders entirely.
  unsigned bordermask; // bitfield; 1s will not be drawn (see bordermaskbits)
  uint64_t borderchan; // attributes used for ncreel border
  unsigned tabletmask; // bitfield; same as bordermask but for tablet borders
  uint64_t tabletchan; // tablet border styling channel
  uint64_t focusedchan;// focused tablet border styling channel
  uint64_t bgchannel;  // background colors
  uint64_t flags;      // bitfield over NCREEL_OPTION_*
} ncreel_options;

struct nctablet;
struct ncreel;

// Create an ncreel according to the provided specifications. Returns NULL on
// failure. nc must be a valid ncplane*.
API struct ncreel* ncreel_create(struct ncplane* nc, const ncreel_options* popts);

// Returns the ncplane on which this ncreel lives.
API struct ncplane* ncreel_plane(struct ncreel* pr);

// Tablet draw callback, provided a tablet (from which the ncplane and userptr
// may be extracted), the first column that may be used, the first row that may
// be used, the first column that may not be used, the first row that may not
// be used, and a bool indicating whether output ought be clipped at the top
// (true) or bottom (false). Rows and columns are zero-indexed, and both are
// relative to the tablet's plane.
//
// Regarding clipping: it is possible that the tablet is only partially
// displayed on the screen. If so, it is either partially present on the top of
// the screen, or partially present at the bottom. In the former case, the top
// is clipped (cliptop will be true), and output ought start from the end. In
// the latter case, cliptop is false, and output ought start from the beginning.
//
// Returns the number of lines of output, which ought be less than or equal to
// maxy - begy, and non-negative (negative values might be used in the future).
typedef int (*tabletcb)(struct nctablet* t, int begx, int begy, int maxx,
                        int maxy, bool cliptop);

// Add a new nctablet to the provided ncreel, having the callback object
// opaque. Neither, either, or both of after and before may be specified. If
// neither is specified, the new tablet can be added anywhere on the reel. If
// one or the other is specified, the tablet will be added before or after the
// specified tablet. If both are specified, the tablet will be added to the
// resulting location, assuming it is valid (after->next == before->prev); if
// it is not valid, or there is any other error, NULL will be returned.
API struct nctablet* ncreel_add(struct ncreel* pr, struct nctablet* after,
                                struct nctablet* before, tabletcb cb,
                                void* opaque);

// Return the number of nctablets in the ncreel.
API int ncreel_tabletcount(const struct ncreel* pr);

// Delete the tablet specified by t from the ncreel specified by pr. Returns
// -1 if the tablet cannot be found.
API int ncreel_del(struct ncreel* pr, struct nctablet* t);

// Delete the active tablet. Returns -1 if there are no tablets.
API int ncreel_del_focused(struct ncreel* pr);

// Redraw the ncreel in its entirety.
API int ncreel_redraw(struct ncreel* pr);

// Offer the input to the ncreel. If it's relevant, this function returns
// true, and the input ought not be processed further. If it's irrelevant to
// the reel, false is returned. Relevant inputs include:
//  * a mouse click on a tablet (focuses tablet)
//  * a mouse scrollwheel event (rolls reel)
//  * up, down, pgup, or pgdown (navigates among items)
API bool ncreel_offer_input(struct ncreel* n, const struct ncinput* nc);

// Return the focused tablet, if any tablets are present. This is not a copy;
// be careful to use it only for the duration of a critical section.
API struct nctablet* ncreel_focused(struct ncreel* pr);

// Change focus to the next tablet, if one exists
API struct nctablet* ncreel_next(struct ncreel* pr);

// Change focus to the previous tablet, if one exists
API struct nctablet* ncreel_prev(struct ncreel* pr);

// Destroy an ncreel allocated with ncreel_create(). Returns non-zero on failure.
API int ncreel_destroy(struct ncreel* pr);

// Returns a pointer to a user pointer associated with this nctablet.
API void* nctablet_userptr(struct nctablet* t);

// Access the ncplane associated with this nctablet, if one exists.
API struct ncplane* nctablet_ncplane(struct nctablet* t);

// The number of columns is one fewer, as the STRLEN expressions must leave
// an extra byte open in case 'µ' (U+00B5, 0xC2 0xB5) shows up. PREFIXCOLUMNS
// is the maximum number of columns used by a mult == 1000 (standard)
// ncmetric() call. IPREFIXCOLUMNS is the maximum number of columns used by a
// mult == 1024 (digital information) ncmetric(). BPREFIXSTRLEN is the maximum
// number of columns used by a mult == 1024 call making use of the 'i' suffix.
// This is the true number of columns; to set up a printf()-style maximum
// field width, you should use [IB]PREFIXFMT (see below).
#define PREFIXCOLUMNS 7
#define IPREFIXCOLUMNS 8
#define BPREFIXCOLUMNS 9
#define PREFIXSTRLEN (PREFIXCOLUMNS + 1)  // Does not include a '\0' (xxx.xxU)
#define IPREFIXSTRLEN (IPREFIXCOLUMNS + 1) //  Does not include a '\0' (xxxx.xxU)
#define BPREFIXSTRLEN (BPREFIXCOLUMNS + 1) // Does not include a '\0' (xxxx.xxUi), i == prefix
// Used as arguments to a variable field width (i.e. "%*s" -- these are the *).
// We need this convoluted grotesquery to properly handle 'µ'.
#define NCMETRICFWIDTH(x, cols) ((int)(strlen(x) - mbswidth(x) + (cols)))
#define PREFIXFMT(x) NCMETRICFWIDTH((x), PREFIXCOLUMNS), (x)
#define IPREFIXFMT(x) NCMETRIXFWIDTH((x), IPREFIXCOLUMNS), (x)
#define BPREFIXFMT(x) NCMETRICFWIDTH((x), BPREFIXCOLUMNS), (x)

// Takes an arbitrarily large number, and prints it into a fixed-size buffer by
// adding the necessary SI suffix. Usually, pass a |[IB]PREFIXSTRLEN+1|-sized
// buffer to generate up to |[IB]PREFIXCOLUMNS| columns' worth of EGCs. The
// characteristic can occupy up through |mult-1| characters (3 for 1000, 4 for
// 1024). The mantissa can occupy either zero or two characters.
//
// Floating-point is never used, because an IEEE758 double can only losslessly
// represent integers through 2^53-1.
//
// 2^64-1 is 18446744073709551615, 18.45E(xa). KMGTPEZY thus suffice to handle
// an 89-bit uintmax_t. Beyond Z(etta) and Y(otta) lie lands unspecified by SI.
// 2^-63 is 0.000000000000000000108, 1.08a(tto).
// val: value to print
// decimal: scaling. '1' if none has taken place.
// buf: buffer in which string will be generated
// omitdec: inhibit printing of all-0 decimal portions
// mult: base of suffix system (almost always 1000 or 1024)
// uprefix: character to print following suffix ('i' for kibibytes basically).
//   only printed if suffix is actually printed (input >= mult).
API const char* ncmetric(uintmax_t val, uintmax_t decimal, char* buf,
                         int omitdec, uintmax_t mult, int uprefix);

// Mega, kilo, gigafoo. Use PREFIXSTRLEN + 1.
static inline const char*
qprefix(uintmax_t val, uintmax_t decimal, char* buf, int omitdec){
  return ncmetric(val, decimal, buf, omitdec, 1000, '\0');
}

// Mibi, kebi, gibibytes. Use BPREFIXSTRLEN + 1.
static inline const char*
bprefix(uintmax_t val, uintmax_t decimal, char* buf, int omitdec){
  return ncmetric(val, decimal, buf, omitdec, 1024, 'i');
}

API void notcurses_cursor_enable(struct notcurses* nc);
API void notcurses_cursor_disable(struct notcurses* nc);

// Palette API. Some terminals only support 256 colors, but allow the full
// palette to be specified with arbitrary RGB colors. In all cases, it's more
// performant to use indexed colors, since it's much less data to write to the
// terminal. If you can limit yourself to 256 colors, that' probably best.

typedef struct palette256 {
  // We store the RGB values as a regular ol' channel
  uint32_t chans[NCPALETTESIZE];
} palette256;

// Create a new palette store. It will be initialized with notcurses's best
// knowledge of the currently configured palette.
API palette256* palette256_new(struct notcurses* nc);

// Attempt to configure the terminal with the provided palette 'p'. Does not
// transfer ownership of 'p'; palette256_free() can still be called.
API int palette256_use(struct notcurses* nc, const palette256* p);

// Manipulate entries in the palette store 'p'. These are *not* locked.
static inline int
palette256_set_rgb(palette256* p, int idx, int r, int g, int b){
  if(idx < 0 || (size_t)idx > sizeof(p->chans) / sizeof(*p->chans)){
    return -1;
  }
  return channel_set_rgb(&p->chans[idx], r, g, b);
}

static inline int
palette256_set(palette256* p, int idx, unsigned rgb){
  if(idx < 0 || (size_t)idx > sizeof(p->chans) / sizeof(*p->chans)){
    return -1;
  }
  return channel_set(&p->chans[idx], rgb);
}

static inline int
palette256_get_rgb(const palette256* p, int idx, unsigned* RESTRICT r, unsigned* RESTRICT g, unsigned* RESTRICT b){
  if(idx < 0 || (size_t)idx > sizeof(p->chans) / sizeof(*p->chans)){
    return -1;
  }
  return channel_rgb(p->chans[idx], r, g, b);
}

// Free the palette store 'p'.
API void palette256_free(palette256* p);

// Convert the plane's content to greyscale.
API void ncplane_greyscale(struct ncplane* n);

// selection widget -- an ncplane with a title header and a body section. the
// body section supports infinite scrolling up and down. the widget looks like:
//
//                                 ╭──────────────────────────╮
//                                 │This is the primary header│
//   ╭──────────────────────this is the secondary header──────╮
//   │                                                        │
//   │ option1   Long text #1                                 │
//   │ option2   Long text #2                                 │
//   │ option3   Long text #3                                 │
//   │ option4   Long text #4                                 │
//   │ option5   Long text #5                                 │
//   │ option6   Long text #6                                 │
//   │                                                        │
//   ╰────────────────────────────────────here's the footer───╯
//
// At all times, exactly one item is selected.

struct ncselector_item {
  char* option;
  char* desc;
  size_t opcolumns;   // filled in by library
  size_t desccolumns; // filled in by library
};

typedef struct ncselector_options {
  char* title; // title may be NULL, inhibiting riser, saving two rows.
  char* secondary; // secondary may be NULL
  char* footer; // footer may be NULL
  struct ncselector_item* items; // initial items and descriptions
  unsigned itemcount; // number of initial items and descriptions
  // default item (selected at start), must be < itemcount unless 'itemcount'
  // is 0, in which case 'defidx' must also be 0
  unsigned defidx;
  // maximum number of options to display at once, 0 to use all available space
  unsigned maxdisplay;
  // exhaustive styling options
  uint64_t opchannels;   // option channels
  uint64_t descchannels; // description channels
  uint64_t titlechannels;// title channels
  uint64_t footchannels; // secondary and footer channels
  uint64_t boxchannels;  // border channels
  uint64_t bgchannels;   // background channels, used only in body
  uint64_t flags;        // bitfield of NCSELECTOR_OPTION_*
} ncselector_options;

API struct ncselector* ncselector_create(struct ncplane* n, int y, int x,
                                         const ncselector_options* opts);

API int ncselector_additem(struct ncselector* n, const struct ncselector_item* item);
API int ncselector_delitem(struct ncselector* n, const char* item);

// Return reference to the selected option, or NULL if there are no items.
API const char* ncselector_selected(const struct ncselector* n);

// Return a reference to the ncselector's underlying ncplane.
API struct ncplane* ncselector_plane(struct ncselector* n);

// Move up or down in the list. A reference to the newly-selected item is
// returned, or NULL if there are no items in the list.
API const char* ncselector_previtem(struct ncselector* n);
API const char* ncselector_nextitem(struct ncselector* n);

// Offer the input to the ncselector. If it's relevant, this function returns
// true, and the input ought not be processed further. If it's irrelevant to
// the selector, false is returned. Relevant inputs include:
//  * a mouse click on an item
//  * a mouse scrollwheel event
//  * a mouse click on the scrolling arrows
//  * up, down, pgup, or pgdown on an unrolled menu (navigates among items)
API bool ncselector_offer_input(struct ncselector* n, const struct ncinput* nc);

// Destroy the ncselector. If 'item' is not NULL, the last selected option will
// be strdup()ed and assigned to '*item' (and must be free()d by the caller).
API void ncselector_destroy(struct ncselector* n, char** item);

struct ncmselector_item {
  char* option;
  char* desc;
  bool selected;
};

// multiselection widget -- a selector supporting multiple selections.
//
//      ╭────────────────────────────────────────────────────────────────╮
//      │ this is truly an awfully long example of a MULTISELECTOR title │
//╭─────┴─────────────────────────────pick one (you will die regardless)─┤
//│  ↑                                                                   │
//│ ☐ 1 Across the Atlantic Ocean, there was a place called North America│
//│ ☐ 2 Discovered by an Italian in the employ of the queen of Spain     │
//│ ☐ 3 Colonized extensively by the Spanish and the French              │
//│ ☐ 4 Developed into a rich nation by Dutch-supplied African slaves    │
//│ ☐ 5 And thus became the largest English-speaking nation on earth     │
//│ ☐ 6 Namely, the United States of America                             │
//│ ☐ 7 The inhabitants of the United States called themselves Yankees   │
//│ ☐ 8 For some reason                                                  │
//│ ☐ 9 And, eventually noticing the rest of the world was there,        │
//│ ☐ 10 Decided to rule it.                                             │
//│  ↓                                                                   │
//╰─────────────────────────press q to exit (there is sartrev("no exit")─╯
//
// Unlike the selector widget, zero to all of the items can be selected, but
// also the widget does not support adding or removing items at runtime.
typedef struct ncmultiselector_options {
  char* title; // title may be NULL, inhibiting riser, saving two rows.
  char* secondary; // secondary may be NULL
  char* footer; // footer may be NULL
  struct ncmselector_item* items; // initial items, descriptions, and statuses
  unsigned itemcount; // number of items and descriptions, can't be 0
  // maximum number of options to display at once, 0 to use all available space
  unsigned maxdisplay;
  // exhaustive styling options
  uint64_t opchannels;   // option channels
  uint64_t descchannels; // description channels
  uint64_t titlechannels;// title channels
  uint64_t footchannels; // secondary and footer channels
  uint64_t boxchannels;  // border channels
  uint64_t bgchannels;   // background channels, used only in body
  uint64_t flags;        // bitfield of NCMULTISELECTOR_OPTION_*
} ncmultiselector_options;

API struct ncmultiselector* ncmultiselector_create(struct ncplane* n, int y, int x,
                                                   const ncmultiselector_options* opts);

// Return selected vector. An array of bools must be provided, along with its
// length. If that length doesn't match the itemcount, it is an error.
API int ncmultiselector_selected(struct ncmultiselector* n, bool* selected, unsigned count);

// Return a reference to the ncmultiselector's underlying ncplane.
API struct ncplane* ncmultiselector_plane(struct ncmultiselector* n);

// Offer the input to the ncmultiselector. If it's relevant, this function
// returns true, and the input ought not be processed further. If it's
// irrelevant to the multiselector, false is returned. Relevant inputs include:
//  * a mouse click on an item
//  * a mouse scrollwheel event
//  * a mouse click on the scrolling arrows
//  * up, down, pgup, or pgdown on an unrolled menu (navigates among items)
API bool ncmultiselector_offer_input(struct ncmultiselector* n, const struct ncinput* nc);

// Destroy the ncmultiselector.
API void ncmultiselector_destroy(struct ncmultiselector* n, char** item);

// Menus. Horizontal menu bars are supported, on the top and/or bottom rows.
// If the menu bar is longer than the screen, it will be only partially
// visible. Menus may be either visible or invisible by default. In the event of
// a screen resize, menus will be automatically moved/resized. Elements can be
// dynamically enabled or disabled at all levels (menu, section, and item),

struct ncmenu_item {
  char* desc;           // utf-8 menu item, NULL for horizontal separator
  ncinput shortcut;     // shortcut, all should be distinct
};

struct ncmenu_section {
  char* name;             // utf-8 c string
  int itemcount;
  struct ncmenu_item* items;
  ncinput shortcut;       // shortcut, will be underlined if present in name
};

#define NCMENU_OPTION_BOTTOM 0x0001ull // bottom row (as opposed to top row)
#define NCMENU_OPTION_HIDING 0x0002ull // hide the menu when not being used

typedef struct ncmenu_options {
  struct ncmenu_section* sections; // array of 'sectioncount' menu_sections
  int sectioncount;                // must be positive
  uint64_t headerchannels;         // styling for header
  uint64_t sectionchannels;        // styling for sections
  uint64_t flags;                  // flag word of NCMENU_OPTION_*
} ncmenu_options;

// Create a menu with the specified options. Menus are currently bound to an
// overall notcurses object (as opposed to a particular plane), and are
// implemented as ncplanes kept atop other ncplanes.
API struct ncmenu* ncmenu_create(struct ncplane* nc, const ncmenu_options* opts);

// Unroll the specified menu section, making the menu visible if it was
// invisible, and rolling up any menu section that is already unrolled.
API int ncmenu_unroll(struct ncmenu* n, int sectionidx);

// Roll up any unrolled menu section, and hide the menu if using hiding.
API int ncmenu_rollup(struct ncmenu* n);

// Unroll the previous/next section (relative to current unrolled). If no
// section is unrolled, the first section will be unrolled.
API int ncmenu_nextsection(struct ncmenu* n);
API int ncmenu_prevsection(struct ncmenu* n);

// Move to the previous/next item within the currently unrolled section. If no
// section is unrolled, the first section will be unrolled.
API int ncmenu_nextitem(struct ncmenu* n);
API int ncmenu_previtem(struct ncmenu* n);

// Return the selected item description, or NULL if no section is unrolled. If
// 'ni' is not NULL, and the selected item has a shortcut, 'ni' will be filled
// in with that shortcut--this can allow faster matching.
API const char* ncmenu_selected(const struct ncmenu* n, struct ncinput* ni);

// Return the ncplane backing this ncmenu.
API struct ncplane* ncmenu_plane(struct ncmenu* n);

// Offer the input to the ncmenu. If it's relevant, this function returns true,
// and the input ought not be processed further. If it's irrelevant to the
// menu, false is returned. Relevant inputs include:
//  * mouse movement over a hidden menu
//  * a mouse click on a menu section (the section is unrolled)
//  * a mouse click outside of an unrolled menu (the menu is rolled up)
//  * left or right on an unrolled menu (navigates among sections)
//  * up or down on an unrolled menu (navigates among items)
//  * escape on an unrolled menu (the menu is rolled up)
API bool ncmenu_offer_input(struct ncmenu* n, const struct ncinput* nc);

// Destroy a menu created with ncmenu_create().
API int ncmenu_destroy(struct ncmenu* n);

// Plots. Given a rectilinear area, an ncplot can graph samples along some axis.
// There is some underlying independent variable--this could be e.g. measurement
// sequence number, or measurement time. Samples are tagged with this variable, which
// should never fall, but may grow non-monotonically. The desired range in terms
// of the underlying independent variable is provided at creation time. The
// desired domain can be specified, or can be autosolved. Granularity of the
// dependent variable depends on glyph selection.
//
// For instance, perhaps we're sampling load as a time series. We want to
// display an hour's worth of samples in 40 columns and 5 rows. We define the
// x-axis to be the independent variable, time. We'll stamp at second
// granularity. In this case, there are 60 * 60 == 3600 total elements in the
// range. Each column will thus cover a 90s span. Using vertical blocks (the
// most granular glyph), we have 8 * 5 == 40 levels of domain. If we report the
// following samples, starting at 0, using autosolving, we will observe:
//
// 60   -- 1%       |domain:   1--1, 0: 20 levels
// 120  -- 50%      |domain:  1--50, 0: 0 levels, 1: 40 levels
// 180  -- 50%      |domain:  1--50, 0: 0 levels, 1: 40 levels, 2: 40 levels
// 240  -- 100%     |domain:  1--75, 0: 1, 1: 27, 2: 40
// 271  -- 100%     |domain: 1--100, 0: 0, 1: 20, 2: 30, 3: 40
// 300  -- 25%      |domain:  1--75, 0: 0, 1: 27, 2: 40, 3: 33
//
// At the end, we have data in 4 90s spans: [0--89], [90--179], [180--269], and
// [270--359]. The first two spans have one sample each, while the second two
// have two samples each. Samples within a span are averaged (FIXME we could
// probably do better), so the results are 0, 50, 75, and 62.5. Scaling each of
// these out of 90 and multiplying by 40 gets our resulting levels. The final
// domain is 75 rather than 100 due to the averaging of 100+25/2->62.5 in the
// third span, at which point the maximum span value is once again 75.
//
// The 20 levels at first is a special case. When the domain is only 1 unit,
// and autoscaling is in play, assign 50%.
//
// This options structure works for both the ncuplot (uint64_t) and ncdplot
// (double) types.
#define NCPLOT_OPTION_LABELTICKSD   0x0001ull // show labels for dependent axis
#define NCPLOT_OPTION_EXPONENTIALD  0x0002ull // exponential dependent axis
#define NCPLOT_OPTION_VERTICALI     0x0004ull // independent axis is vertical
#define NCPLOT_OPTION_NODEGRADE     0x0008ull // fail rather than degrade blitter
#define NCPLOT_OPTION_DETECTMAXONLY 0x0010ull // use domain detection only for max

typedef struct ncplot_options {
  // channels for the maximum and minimum levels. linear or exponential
  // interpolation will be applied across the domain between these two.
  uint64_t maxchannel;
  uint64_t minchannel;
  // if you don't care, pass NCBLIT_DEFAULT and get NCBLIT_8x1 (assuming
  // UTF8) or NCBLIT_1x1 (in an ASCII environment)
  ncblitter_e gridtype; // number of "pixels" per row x column
  // independent variable can either be a contiguous range, or a finite set
  // of keys. for a time range, say the previous hour sampled with second
  // resolution, the independent variable would be the range [0..3600): 3600.
  // if rangex is 0, it is dynamically set to the number of columns.
  int rangex;
  uint64_t flags;      // bitfield over NCPLOT_OPTION_*
} ncplot_options;

// Use the provided plane 'n' for plotting according to the options 'opts'.
// The plot will make free use of the entirety of the plane.
// for domain autodiscovery, set miny == maxy == 0.
API struct ncuplot* ncuplot_create(struct ncplane* n, const ncplot_options* opts,
                                   uint64_t miny, uint64_t maxy);
API struct ncdplot* ncdplot_create(struct ncplane* n, const ncplot_options* opts,
                                   double miny, double maxy);

// Return a reference to the ncplot's underlying ncplane.
API struct ncplane* ncuplot_plane(struct ncuplot* n);
API struct ncplane* ncdplot_plane(struct ncdplot* n);

// Add to or set the value corresponding to this x. If x is beyond the current
// x window, the x window is advanced to include x, and values passing beyond
// the window are lost. The first call will place the initial window. The plot
// will be redrawn, but notcurses_render() is not called.
API int ncuplot_add_sample(struct ncuplot* n, uint64_t x, uint64_t y);
API int ncdplot_add_sample(struct ncdplot* n, uint64_t x, double y);
API int ncuplot_set_sample(struct ncuplot* n, uint64_t x, uint64_t y);
API int ncdplot_set_sample(struct ncdplot* n, uint64_t x, double y);
API int ncuplot_sample(const struct ncuplot* n, uint64_t x, uint64_t* y);
API int ncdplot_sample(const struct ncdplot* n, uint64_t x, double* y);

API void ncuplot_destroy(struct ncuplot* n);
API void ncdplot_destroy(struct ncdplot* n);

typedef int(*ncfdplane_callback)(struct ncfdplane* n, const void* buf, size_t s, void* curry);
typedef int(*ncfdplane_done_cb)(struct ncfdplane* n, int fderrno, void* curry);

// read from an fd until EOF (or beyond, if follow is set), invoking the user's
// callback each time. runs in its own context. on EOF or error, the finalizer
// callback will be invoked, and the user ought destroy the ncfdplane. the
// data is *not* guaranteed to be nul-terminated, and may contain arbitrary
// zeroes.
typedef struct ncfdplane_options {
  void* curry;    // parameter provided to callbacks
  bool follow;    // keep reading after hitting end? (think tail -f)
  uint64_t flags; // bitfield over NCOPTION_FDPLANE_*
} ncfdplane_options;

// Create an ncfdplane around the fd 'fd'. Consider this function to take
// ownership of the file descriptor, which will be closed in ncfdplane_destroy().
API struct ncfdplane* ncfdplane_create(struct ncplane* n, const ncfdplane_options* opts,
                          int fd, ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn);

API struct ncplane* ncfdplane_plane(struct ncfdplane* n);

API int ncfdplane_destroy(struct ncfdplane* n);

typedef struct ncsubproc_options {
  void* curry;
  uint64_t restart_period; // restart this many seconds after an exit (watch)
  uint64_t flags;          // bitfield over NCOPTION_SUBPROC_*
} ncsubproc_options;

// see exec(2). p-types use $PATH. e-type passes environment vars.
API struct ncsubproc* ncsubproc_createv(struct ncplane* n, const ncsubproc_options* opts,
                                        const char* bin,  char* const arg[],
                                        ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn);
API struct ncsubproc* ncsubproc_createvp(struct ncplane* n, const ncsubproc_options* opts,
                                         const char* bin,  char* const arg[],
                                         ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn);
API struct ncsubproc* ncsubproc_createvpe(struct ncplane* n, const ncsubproc_options* opts,
                       const char* bin,  char* const arg[], char* const env[],
                       ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn);

API struct ncplane* ncsubproc_plane(struct ncsubproc* n);

API int ncsubproc_destroy(struct ncsubproc* n);

// Draw a QR code at the current position on the plane. If there is insufficient
// room to draw the code here, or there is any other error, non-zero will be
// returned. Otherwise, the QR code "version" (size) is returned. The QR code
// is (version * 4 + 17) columns wide, and ⌈version * 4 + 17⌉ rows tall (the
// properly-scaled values are written back to '*ymax' and '*xmax').
API int ncplane_qrcode(struct ncplane* n, ncblitter_e blitter, int* ymax,
                       int* xmax, const void* data, size_t len);

#define NCREADER_OPTION_HORSCROLL  0x0001ull
#define NCREADER_OPTION_VERSCROLL  0x0002ull

typedef struct ncreader_options {
  uint64_t tchannels; // channels used for input
  uint64_t echannels; // channels used for empty space
  uint32_t tattrword; // attributes used for input
  uint32_t eattrword; // attributes used for empty space
  char* egc;          // egc used for empty space
  int physrows;
  int physcols;
  uint64_t flags;     // bitfield of NCREADER_OPTION_*
} ncreader_options;

// ncreaders provide freeform input in a (possibly multiline) region,
// supporting readline keybindings. 'rows' and 'cols' both must be negative.
// there are no restrictions on 'y' or 'x'. creates its own plane.
API struct ncreader* ncreader_create(struct ncplane* nc, int y, int x,
                                     const ncreader_options* opts);

// empty the ncreader of any user input, and home the cursor.
API int ncreader_clear(struct ncreader* n);

API struct ncplane* ncreader_plane(struct ncreader* n);

// Offer the input to the ncreader. If it's relevant, this function returns
// true, and the input ought not be processed further. Almost all inputs
// are relevant to an ncreader, save synthesized ones.
API bool ncreader_offer_input(struct ncreader* n, const struct ncinput* ni);

// return a heap-allocated copy of the current (UTF-8) contents.
API char* ncreader_contents(const struct ncreader* n);

// destroy the reader and its bound plane. if 'contents' is not NULL, the
// UTF-8 input will be heap-duplicated and written to 'contents'.
API void ncreader_destroy(struct ncreader* n, char** contents);

// Dump selected notcurses state to the supplied 'debugfp'. Output is freeform,
// and subject to change. It includes geometry of all planes.
API void notcurses_debug(struct notcurses* nc, FILE* debugfp);

// a system for rendering RGBA pixels as text glyphs
struct blitset {
  ncblitter_e geom;
  int width;
  int height;
  // the EGCs which form the various levels of a given geometry. if the geometry
  // is wide, things are arranged with the rightmost side increasing most
  // quickly, i.e. it can be indexed as height arrays of 1 + height glyphs. i.e.
  // the first five braille EGCs are all 0 on the left, [0..4] on the right.
  const wchar_t* egcs;
  int (*blit)(struct ncplane* nc, int placey, int placex, int linesize,
              const void* data, int begy, int begx, int leny, int lenx,
              bool bgr, bool blendcolors);
  bool fill;
};

API extern const struct blitset notcurses_blitters[];

#undef API

#ifdef __cplusplus
} // extern "C"
#endif

#endif
