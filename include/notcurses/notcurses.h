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
// take host byte order and turn it into network (reverse on LE, no-op on BE),
// then reverse that, guaranteeing LE. htole(x) == ltohe(x).
#if defined(__linux__) || defined(__gnu_hurd__)
#include <byteswap.h>
#define htole(x) (bswap_32(htonl(x)))
#else
#include <sys/endian.h>
#define htole(x) (bswap32(htonl(x)))
#endif
#include <netinet/in.h>
#include <notcurses/nckeys.h>

#ifdef __cplusplus
extern "C" {
#define RESTRICT
#else
#define RESTRICT restrict
#endif

#define API __attribute__((visibility("default")))
#define ALLOC __attribute__((malloc)) __attribute__((warn_unused_result))

// Get a human-readable string describing the running Notcurses version.
API const char* notcurses_version(void);
// Cannot be inline, as we want to get the versions of the actual Notcurses
// library we loaded, not what we compile against.
API void notcurses_version_components(int* major, int* minor, int* patch, int* tweak);

struct notcurses; // Notcurses state for a given terminal, composed of ncplanes
struct ncplane;   // a drawable Notcurses surface, composed of cells
struct ncvisual;  // a visual bit of multimedia opened with LibAV|OIIO
struct ncuplot;   // uint64_t histogram
struct ncdplot;   // double histogram
struct ncprogbar; // progress bar
struct ncfdplane; // i/o wrapper to dump file descriptor to plane
struct ncsubproc; // ncfdplane wrapper with subprocess management
struct ncselector;// widget supporting selecting 1 from a list of options
struct ncmultiselector; // widget supporting selecting 0..n from n options
struct ncreader;  // widget supporting free string input ala readline
struct ncfadectx; // context for a palette fade operation

// we never blit full blocks, but instead spaces (more efficient) with the
// background set to the desired foreground.
typedef enum {
  NCBLIT_DEFAULT, // let the ncvisual pick
  NCBLIT_1x1,     // space, compatible with ASCII
  NCBLIT_2x1,     // halves + 1x1 (space)     ▄▀
  NCBLIT_2x2,     // quadrants + 2x1          ▗▐ ▖▀▟▌▙
  NCBLIT_3x2,     // sextants (*NOT* 2x2)     🬀🬁🬂🬃🬄🬅🬆🬇🬈🬉🬊🬋🬌🬍🬎🬏🬐🬑🬒🬓🬔🬕🬖🬗🬘🬙🬚🬛🬜🬝🬞🬟🬠🬡🬢🬣🬤🬥🬦🬧🬨🬩🬪🬫🬬🬭🬮🬯🬰🬱🬲🬳🬴🬵🬶🬷🬸🬹🬺🬻
  NCBLIT_4x1,     // four vertical levels     █▆▄▂
  NCBLIT_BRAILLE, // 4 rows, 2 cols (braille) ⡀⡄⡆⡇⢀⣀⣄⣆⣇⢠⣠⣤⣦⣧⢰⣰⣴⣶⣷⢸⣸⣼⣾⣿
  NCBLIT_8x1,     // eight vertical levels    █▇▆▅▄▃▂▁
  NCBLIT_SIXEL,   // not yet implemented
} ncblitter_e;

// Alignment within a plane or terminal. Left/right-justified, or centered.
typedef enum {
  NCALIGN_UNALIGNED,
  NCALIGN_LEFT,
  NCALIGN_CENTER,
  NCALIGN_RIGHT,
} ncalign_e;

// How to scale an ncvisual during rendering. NCSCALE_NONE will apply no
// scaling. NCSCALE_SCALE scales a visual to the plane's size, maintaining
// aspect ratio. NCSCALE_STRETCH stretches and scales the image in an
// attempt to fill the entirety of the plane. NCSCALE_NONE_HIRES and
// NCSCALE_SCALE_HIRES behave like their counterparts, but admit blitters
// which don't preserve aspect ratio.
typedef enum {
  NCSCALE_NONE,
  NCSCALE_SCALE,
  NCSCALE_STRETCH,
  NCSCALE_NONE_HIRES,
  NCSCALE_SCALE_HIRES,
} ncscale_e;

// Returns the number of columns occupied by a multibyte (UTF-8) string, or
// -1 if a non-printable/illegal character is encountered.
API int ncstrwidth(const char* mbs);

// input functions like notcurses_getc() return ucs32-encoded char32_t. convert
// a series of char32_t to utf8. result must be at least 4 bytes per input
// char32_t (6 bytes per char32_t will future-proof against Unicode expansion).
// the number of bytes used is returned, or -1 if passed illegal ucs32, or too
// small of a buffer.
API int notcurses_ucs32_to_utf8(const char32_t* ucs32, unsigned ucs32count,
                                unsigned char* resultbuf, size_t buflen);

// extract these bits to get a channel's alpha value
#define CHANNEL_ALPHA_MASK    0x30000000ull
// background cannot be highcontrast, only foreground
#define CELL_ALPHA_HIGHCONTRAST 0x30000000ull
#define CELL_ALPHA_TRANSPARENT  0x20000000ull
#define CELL_ALPHA_BLEND        0x10000000ull
#define CELL_ALPHA_OPAQUE       0x00000000ull

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
#define NCPALETTESIZE 256
// if this bit *and* CELL_FGDEFAULT_MASK are set, we're using a
// palette-indexed foreground color
#define CELL_FG_PALETTE         (CELL_BG_PALETTE << 32u)
// extract these bits to get the background alpha mask
#define CELL_BG_ALPHA_MASK      CHANNEL_ALPHA_MASK
// extract these bits to get the foreground alpha mask
#define CELL_FG_ALPHA_MASK      (CELL_BG_ALPHA_MASK << 32u)

// initialize a 64-bit channel pair with specified RGB fg/bg
#define CHANNELS_RGB_INITIALIZER(fr, fg, fb, br, bg, bb) \
  (((((uint64_t)(fr) << 16u) + ((uint64_t)(fg) << 8u) + (uint64_t)(fb)) << 32ull) + \
   (((br) << 16u) + ((bg) << 8u) + (bb)) + CELL_BGDEFAULT_MASK + CELL_FGDEFAULT_MASK)

#define CHANNEL_RGB_INITIALIZER(r, g, b) \
  (((uint32_t)r << 16u) + ((uint32_t)g << 8u) + (b) + CELL_BGDEFAULT_MASK)

// These lowest-level functions manipulate a 64-bit channel encoding directly.
// Users will typically manipulate ncplane and cell channels through those APIs,
// rather than calling these directly.

// Extract the 8-bit red component from a 32-bit channel.
static inline unsigned
channel_r(uint32_t channel){
  return (channel & 0xff0000u) >> 16u;
}

// Extract the 8-bit green component from a 32-bit channel.
static inline unsigned
channel_g(uint32_t channel){
  return (channel & 0x00ff00u) >> 8u;
}

// Extract the 8-bit blue component from a 32-bit channel.
static inline unsigned
channel_b(uint32_t channel){
  return (channel & 0x0000ffu);
}

// Extract the three 8-bit R/G/B components from a 32-bit channel.
static inline unsigned
channel_rgb8(uint32_t channel, unsigned* RESTRICT r, unsigned* RESTRICT g,
            unsigned* RESTRICT b){
  *r = channel_r(channel);
  *g = channel_g(channel);
  *b = channel_b(channel);
  return channel;
}

// Set the three 8-bit components of a 32-bit channel, and mark it as not using
// the default color. Retain the other bits unchanged.
static inline int
channel_set_rgb8(uint32_t* channel, int r, int g, int b){
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

// Set the three 8-bit components of a 32-bit channel, and mark it as not using
// the default color. Retain the other bits unchanged. r, g, and b will be
// clipped to the range [0..255].
static inline void
channel_set_rgb8_clipped(unsigned* channel, int r, int g, int b){
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
  return channel & CHANNEL_ALPHA_MASK;
}

static inline unsigned
channel_palindex(uint32_t channel){
  return channel & 0xff;
}

// Set the 2-bit alpha component of the 32-bit channel.
static inline int
channel_set_alpha(unsigned* channel, unsigned alpha){
  if(alpha & ~CHANNEL_ALPHA_MASK){
    return -1;
  }
  *channel = alpha | (*channel & ~CHANNEL_ALPHA_MASK);
  if(alpha != CELL_ALPHA_OPAQUE){
    *channel |= CELL_BGDEFAULT_MASK;
  }
  return 0;
}

static inline int
channel_set_palindex(uint32_t* channel, int idx){
  if(idx < 0 || idx >= NCPALETTESIZE){
    return -1;
  }
  *channel |= CELL_BGDEFAULT_MASK;
  *channel |= CELL_BG_PALETTE;
  channel_set_alpha(channel, CELL_ALPHA_OPAQUE);
  *channel &= 0xff000000ull;
  *channel |= idx;
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

static inline unsigned
channels_fg_palindex(uint64_t channels){
  return channel_palindex(channels_fchannel(channels));
}

static inline unsigned
channels_bg_palindex(uint64_t channels){
  return channel_palindex(channels_bchannel(channels));
}

// Extract 24 bits of foreground RGB from 'channels', shifted to LSBs.
static inline unsigned
channels_fg_rgb(uint64_t channels){
  return channels_fchannel(channels) & CELL_BG_RGB_MASK;
}

// Extract 24 bits of background RGB from 'channels', shifted to LSBs.
static inline unsigned
channels_bg_rgb(uint64_t channels){
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
channels_fg_rgb8(uint64_t channels, unsigned* r, unsigned* g, unsigned* b){
  return channel_rgb8(channels_fchannel(channels), r, g, b);
}

// Extract 24 bits of background RGB from 'channels', split into subchannels.
static inline unsigned
channels_bg_rgb8(uint64_t channels, unsigned* r, unsigned* g, unsigned* b){
  return channel_rgb8(channels_bchannel(channels), r, g, b);
}

// Set the r, g, and b channels for the foreground component of this 64-bit
// 'channels' variable, and mark it as not using the default color.
static inline int
channels_set_fg_rgb8(uint64_t* channels, int r, int g, int b){
  uint32_t channel = channels_fchannel(*channels);
  if(channel_set_rgb8(&channel, r, g, b) < 0){
    return -1;
  }
  *channels = ((uint64_t)channel << 32llu) | (*channels & 0xffffffffllu);
  return 0;
}

// Same, but clips to [0..255].
static inline void
channels_set_fg_rgb8_clipped(uint64_t* channels, int r, int g, int b){
  uint32_t channel = channels_fchannel(*channels);
  channel_set_rgb8_clipped(&channel, r, g, b);
  *channels = ((uint64_t)channel << 32llu) | (*channels & 0xffffffffllu);
}

// Set the 2-bit alpha component of the foreground channel.
static inline int
channels_set_fg_alpha(uint64_t* channels, unsigned alpha){
  uint32_t channel = channels_fchannel(*channels);
  if(channel_set_alpha(&channel, alpha) < 0){
    return -1;
  }
  *channels = ((uint64_t)channel << 32llu) | (*channels & 0xffffffffllu);
  return 0;
}

static inline int
channels_set_fg_palindex(uint64_t* channels, int idx){
  uint32_t channel = channels_fchannel(*channels);
  if(channel_set_palindex(&channel, idx) < 0){
    return -1;
  }
  *channels = ((uint64_t)channel << 32llu) | (*channels & 0xffffffffllu);
  return 0;
}

// Same, but set an assembled 24 bit channel at once.
static inline int
channels_set_fg_rgb(uint64_t* channels, unsigned rgb){
  uint32_t channel = channels_fchannel(*channels);
  if(channel_set(&channel, rgb) < 0){
    return -1;
  }
  *channels = ((uint64_t)channel << 32llu) | (*channels & 0xffffffffllu);
  return 0;
}

// Set the r, g, and b channels for the background component of this 64-bit
// 'channels' variable, and mark it as not using the default color.
static inline int
channels_set_bg_rgb8(uint64_t* channels, int r, int g, int b){
  uint32_t channel = channels_bchannel(*channels);
  if(channel_set_rgb8(&channel, r, g, b) < 0){
    return -1;
  }
  channels_set_bchannel(channels, channel);
  return 0;
}

// Same, but clips to [0..255].
static inline void
channels_set_bg_rgb8_clipped(uint64_t* channels, int r, int g, int b){
  uint32_t channel = channels_bchannel(*channels);
  channel_set_rgb8_clipped(&channel, r, g, b);
  channels_set_bchannel(channels, channel);
}

// Set the 2-bit alpha component of the background channel.
static inline int
channels_set_bg_alpha(uint64_t* channels, unsigned alpha){
  if(alpha == CELL_ALPHA_HIGHCONTRAST){ // forbidden for background alpha
    return -1;
  }
  uint32_t channel = channels_bchannel(*channels);
  if(channel_set_alpha(&channel, alpha) < 0){
    return -1;
  }
  channels_set_bchannel(channels, channel);
  return 0;
}

// Set the cell's background palette index, set the background palette index
// bit, set it background-opaque, and clear the background default color bit.
static inline int
channels_set_bg_palindex(uint64_t* channels, int idx){
  uint32_t channel = channels_bchannel(*channels);
  if(channel_set_palindex(&channel, idx) < 0){
    return -1;
  }
  channels_set_bchannel(channels, channel);
  return 0;
}

// Same, but set an assembled 24 bit channel at once.
static inline int
channels_set_bg_rgb(uint64_t* channels, unsigned rgb){
  uint32_t channel = channels_bchannel(*channels);
  if(channel_set(&channel, rgb) < 0){
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
  uint32_t channel = channels_fchannel(*channels);
  channel_set_default(&channel);
  *channels = ((uint64_t)channel << 32llu) | (*channels & 0xffffffffllu);
  return *channels;
}

// Mark the background channel as using its default color.
static inline uint64_t
channels_set_bg_default(uint64_t* channels){
  uint32_t channel = channels_bchannel(*channels);
  channel_set_default(&channel);
  channels_set_bchannel(channels, channel);
  return *channels;
}

// An nccell corresponds to a single character cell on some plane, which can be
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
// other things, like cuneiform. True display width is a *function of the
// font and terminal*. Among the longest Unicode codepoints is
//
//    U+FDFD ARABIC LIGATURE BISMILLAH AR-RAHMAN AR-RAHEEM ﷽
//
// wcwidth() rather optimistically claims this most exalted glyph to occupy
// a single column. BiDi text is too complicated for me to even get into here.
// Be assured there are no easy answers; ours is indeed a disturbing Universe.
//
// Each nccell occupies 16 static bytes (128 bits). The surface is thus ~1.6MB
// for a (pretty large) 500x200 terminal. At 80x43, it's less than 64KB.
// Dynamic requirements (the egcpool) can add up to 16MB to an ncplane, but
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
//
// This structure is exposed only so that most functions can be inlined. Do not
// directly modify or access the fields of this structure; use the API.
typedef struct nccell {
  // These 32 bits, together with the associated plane's associated egcpool,
  // completely define this cell's EGC. Unless the EGC requires more than four
  // bytes to encode as UTF-8, it will be inlined here. If more than four bytes
  // are required, it will be spilled into the egcpool. In either case, there's
  // a NUL-terminated string available without copying, because (1) the egcpool
  // is all NUL-terminated sequences and (2) the fifth byte of this struct (the
  // gcluster_backstop field, see below) is guaranteed to be zero, as are any
  // unused bytes in gcluster.
  //
  // The gcluster + gcluster_backstop thus form a valid C string of between 0
  // and 4 non-NUL bytes. Interpreting them in this fashion requires that
  // gcluster be stored as a little-endian number (strings have no byte order).
  // This gives rise to three simple rules:
  //
  //  * when storing to gcluster from a numeric, always use htole()
  //  * when loading from gcluster for numeric use, always use htole()
  //  * when referencing gcluster as a string, always use a pointer cast
  //
  // Uses of gcluster ought thus always have exactly one htole() or pointer
  // cast associated with them, and we otherwise always work as host-endian.
  //
  // A spilled EGC is indicated by the value 0x01XXXXXX. This cannot alias a
  // true supra-ASCII EGC, because UTF-8 only encodes bytes <= 0x80 when they
  // are single-byte ASCII-derived values. The XXXXXX is interpreted as a 24-bit
  // index into the egcpool. These pools may thus be up to 16MB.
  //
  // The cost of this scheme is that the character 0x01 (SOH) cannot be encoded
  // in a nccell, which is absolutely fine because what 70s horseshit is SOH?
  // It must not be allowed through the API, or havoc will result.
  uint32_t gcluster;          // 4B → 4B little endian EGC
  uint8_t gcluster_backstop;  // 1B → 5B (8 bits of zero)
  // we store the column width in this field. for a multicolumn EGC of N
  // columns, there will be N nccells, and each has a width of N...for now.
  // eventually, such an EGC will set more than one subsequent cell to
  // WIDE_RIGHT, and this won't be necessary. it can then be used as a
  // bytecount. see #1203. FIXME iff width >= 2, the cell is part of a
  // multicolumn glyph. whether a cell is the left or right side of the glyph
  // can be determined by checking whether ->gcluster is zero.
  uint8_t width;              // 1B → 6B (8 bits of EGC column width)
  uint16_t stylemask;         // 2B → 8B (16 bits of NCSTYLE_* attributes)
  // (channels & 0x8000000000000000ull): blitted to upper-left quadrant
  // (channels & 0x4000000000000000ull): foreground is *not* "default color"
  // (channels & 0x3000000000000000ull): foreground alpha (2 bits)
  // (channels & 0x0800000000000000ull): foreground uses palette index
  // (channels & 0x0400000000000000ull): blitted to upper-right quadrant
  // (channels & 0x0200000000000000ull): blitted to lower-left quadrant
  // (channels & 0x0100000000000000ull): blitted to lower-right quadrant
  // (channels & 0x00ffffff00000000ull): foreground in 3x8 RGB (rrggbb) / pindex
  // (channels & 0x0000000080000000ull): reserved, must be 0
  // (channels & 0x0000000040000000ull): background is *not* "default color"
  // (channels & 0x0000000030000000ull): background alpha (2 bits)
  // (channels & 0x0000000008000000ull): background uses palette index
  // (channels & 0x0000000007000000ull): reserved, must be 0
  // (channels & 0x0000000000ffffffull): background in 3x8 RGB (rrggbb) / pindex
  // At render time, these 24-bit values are quantized down to terminal
  // capabilities, if necessary. There's a clear path to 10-bit support should
  // we one day need it, but keep things cagey for now. "default color" is
  // best explained by color(3NCURSES). ours is the same concept. until the
  // "not default color" bit is set, any color you load will be ignored.
  uint64_t channels;          // + 8B == 16B
} nccell;

typedef nccell cell; // FIXME backwards-compat, remove in ABI3

#define CELL_TRIVIAL_INITIALIZER { .gcluster = 0, .gcluster_backstop = 0, .width = 0, .stylemask = 0, .channels = 0, }
// do *not* load invalid EGCs using these macros! there is no way for us to
// protect against such misuse here. problems *will* ensue. similarly, do not
// set channel flags other than colors/alpha.
#define CELL_CHAR_INITIALIZER(c) { .gcluster = (htole(c)), .gcluster_backstop = 0, .width = (uint8_t)wcwidth(c), .stylemask = 0, .channels = 0, }
#define CELL_INITIALIZER(c, s, chan) { .gcluster = (htole(c)), .gcluster_backstop = 0, .width = (uint8_t)wcwidth(c), .stylemask = (s), .channels = (chan), }

static inline void
cell_init(nccell* c){
  memset(c, 0, sizeof(*c));
}

// Breaks the UTF-8 string in 'gcluster' down, setting up the nccell 'c'.
// Returns the number of bytes copied out of 'gcluster', or -1 on failure. The
// styling of the cell is left untouched, but any resources are released.
API int cell_load(struct ncplane* n, nccell* c, const char* gcluster);

// cell_load(), plus blast the styling with 'attr' and 'channels'.
static inline int
cell_prime(struct ncplane* n, nccell* c, const char* gcluster,
           uint32_t stylemask, uint64_t channels){
  c->stylemask = stylemask;
  c->channels = channels;
  int ret = cell_load(n, c, gcluster);
  return ret;
}

// Duplicate 'c' into 'targ'; both must be/will be bound to 'n'. Returns -1 on
// failure, and 0 on success.
API int cell_duplicate(struct ncplane* n, nccell* targ, const nccell* c);

// Release resources held by the nccell 'c'.
API void cell_release(struct ncplane* n, nccell* c);

#define NCSTYLE_MASK      0x03ffu
#define NCSTYLE_STANDOUT  0x0080u
#define NCSTYLE_UNDERLINE 0x0040u
#define NCSTYLE_REVERSE   0x0020u
#define NCSTYLE_BLINK     0x0010u
#define NCSTYLE_DIM       0x0008u
#define NCSTYLE_BOLD      0x0004u
#define NCSTYLE_INVIS     0x0002u
#define NCSTYLE_PROTECT   0x0001u
#define NCSTYLE_ITALIC    0x0100u
#define NCSTYLE_STRUCK    0x0200u
#define NCSTYLE_NONE      0

// Set the specified style bits for the nccell 'c', whether they're actively
// supported or not. Only the lower 16 bits are meaningful.
static inline void
cell_set_styles(nccell* c, unsigned stylebits){
  c->stylemask = stylebits & NCSTYLE_MASK;
}

// Extract the style bits from the nccell.
static inline unsigned
cell_styles(const nccell* c){
  return c->stylemask;
}

// Add the specified styles (in the LSBs) to the nccell's existing spec,
// whether they're actively supported or not.
static inline void
cell_on_styles(nccell* c, unsigned stylebits){
  c->stylemask |= (stylebits & NCSTYLE_MASK);
}

// Remove the specified styles (in the LSBs) from the nccell's existing spec.
static inline void
cell_off_styles(nccell* c, unsigned stylebits){
  c->stylemask &= ~(stylebits & NCSTYLE_MASK);
}

// Use the default color for the foreground.
static inline void
cell_set_fg_default(nccell* c){
  channels_set_fg_default(&c->channels);
}

// Use the default color for the background.
static inline void
cell_set_bg_default(nccell* c){
  channels_set_bg_default(&c->channels);
}

static inline int
cell_set_fg_alpha(nccell* c, int alpha){
  return channels_set_fg_alpha(&c->channels, alpha);
}

static inline int
cell_set_bg_alpha(nccell* c, int alpha){
  return channels_set_bg_alpha(&c->channels, alpha);
}

// Is the cell part of a multicolumn element?
static inline bool
cell_double_wide_p(const nccell* c){
  return (c->width >= 2);
}

// Is this the right half of a wide character?
static inline bool
cell_wide_right_p(const nccell* c){
  return cell_double_wide_p(c) && c->gcluster == 0;
}

// Is this the left half of a wide character?
static inline bool
cell_wide_left_p(const nccell* c){
  return cell_double_wide_p(c) && c->gcluster;
}

// return a pointer to the NUL-terminated EGC referenced by 'c'. this pointer
// can be invalidated by any further operation on the plane 'n', so...watch out!
API const char* cell_extended_gcluster(const struct ncplane* n, const nccell* c);

// copy the UTF8-encoded EGC out of the nccell. the result is not tied to any
// ncplane, and persists across erases / destruction.
ALLOC static inline char*
cell_strdup(const struct ncplane* n, const nccell* c){
  return strdup(cell_extended_gcluster(n, c));
}

// Extract the three elements of a nccell.
static inline char*
cell_extract(const struct ncplane* n, const nccell* c,
             uint16_t* stylemask, uint64_t* channels){
  if(stylemask){
    *stylemask = c->stylemask;
  }
  if(channels){
    *channels = c->channels;
  }
  return cell_strdup(n, c);
}

// Returns true if the two nccells are distinct EGCs, attributes, or channels.
// The actual egcpool index needn't be the same--indeed, the planes needn't even
// be the same. Only the expanded EGC must be equal. The EGC must be bit-equal;
// it would probably be better to test whether they're Unicode-equal FIXME.
static inline bool
cellcmp(const struct ncplane* n1, const nccell* RESTRICT c1,
        const struct ncplane* n2, const nccell* RESTRICT c2){
  if(c1->stylemask != c2->stylemask){
    return true;
  }
  if(c1->channels != c2->channels){
    return true;
  }
  return strcmp(cell_extended_gcluster(n1, c1), cell_extended_gcluster(n2, c2));
}

// Load a 7-bit char 'ch' into the nccell 'c'. Returns the number of bytes
// used, or -1 on error.
static inline int
cell_load_char(struct ncplane* n, nccell* c, char ch){
  char gcluster[2];
  gcluster[0] = ch;
  gcluster[1] = '\0';
  return cell_load(n, c, gcluster);
}

// Load a UTF-8 encoded EGC of up to 4 bytes into the nccell 'c'. Returns the
// number of bytes used, or -1 on error.
static inline int
cell_load_egc32(struct ncplane* n, nccell* c, uint32_t egc){
  char gcluster[sizeof(egc) + 1];
  egc = htole(egc);
  memcpy(gcluster, &egc, sizeof(egc));
  gcluster[4] = '\0';
  return cell_load(n, c, gcluster);
}

// These log levels consciously map cleanly to those of libav; Notcurses itself
// does not use this full granularity. The log level does not affect the opening
// and closing banners, which can be disabled via the notcurses_option struct's
// 'suppress_banner'. Note that if stderr is connected to the same terminal on
// which we're rendering, any kind of logging will disrupt the output.
typedef enum {
  NCLOGLEVEL_SILENT,  // default. print nothing once fullscreen service begins
  NCLOGLEVEL_PANIC,   // print diagnostics immediately related to crashing
  NCLOGLEVEL_FATAL,   // we're hanging around, but we've had a horrible fault
  NCLOGLEVEL_ERROR,   // we can't keep doing this, but we can do other things
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

// NCOPTION_RETAIN_CURSOR was removed in 1.6.18. It ought be repurposed. FIXME.

// Notcurses typically prints version info in notcurses_init() and performance
// info in notcurses_stop(). This inhibits that output.
#define NCOPTION_SUPPRESS_BANNERS    0x0020ull

// If smcup/rmcup capabilities are indicated, Notcurses defaults to making use
// of the "alternate screen". This flag inhibits use of smcup/rmcup.
#define NCOPTION_NO_ALTERNATE_SCREEN 0x0040ull

// Do not modify the font. Notcurses might attempt to change the font slightly,
// to support certain glyphs (especially on the Linux console). If this is set,
// no such modifications will be made. Note that font changes will not affect
// anything but the virtual console/terminal in which Notcurses is running.
#define NCOPTION_NO_FONT_CHANGES     0x0080ull

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

// Lex a margin argument according to the standard Notcurses definition. There
// can be either a single number, which will define all margins equally, or
// there can be four numbers separated by commas.
API int notcurses_lex_margins(const char* op, notcurses_options* opts);

// Lex a blitter.
API int notcurses_lex_blitter(const char* op, ncblitter_e* blitter);

// Get the name of a blitter.
API const char* notcurses_str_blitter(ncblitter_e blitter);

// Lex a visual scaling mode (one of "none", "stretch", or "scale").
API int notcurses_lex_scalemode(const char* op, ncscale_e* scalemode);

// Get the name of a scaling mode.
API const char* notcurses_str_scalemode(ncscale_e scalemode);

// Initialize a Notcurses context on the connected terminal at 'fp'. 'fp' must
// be a tty. You'll usually want stdout. NULL can be supplied for 'fp', in
// which case /dev/tty will be opened. Returns NULL on error, including any
// failure initializing terminfo.
API ALLOC struct notcurses* notcurses_init(const notcurses_options* opts, FILE* fp);

// The same as notcurses_init(), but without any multimedia functionality,
// allowing for a svelter binary. Link with notcurses-core if this is used.
API ALLOC struct notcurses* notcurses_core_init(const notcurses_options* opts, FILE* fp);

// Destroy a Notcurses context.
API int notcurses_stop(struct notcurses* nc);

// Return the topmost plane of the pile containing 'n'.
API struct ncplane* ncpile_top(struct ncplane* n);

// Return the bottommost plane of the pile containing 'n'.
API struct ncplane* ncpile_bottom(struct ncplane* n);

// Renders the pile of which 'n' is a part. Rendering this pile again will blow
// away the render. To actually write out the render, call ncpile_rasterize().
API int ncpile_render(struct ncplane* n);

// Make the physical screen match the last rendered frame from the pile of
// which 'n' is a part. This is a blocking call. Don't call this before the
// pile has been rendered (doing so will likely result in a blank screen).
API int ncpile_rasterize(struct ncplane* n);

// Renders and rasterizes the standard pile in one shot. Blocking call.
API int notcurses_render(struct notcurses* nc);

// Perform the rendering and rasterization portion of notcurses_render(), but
// do not write the resulting buffer out to the terminal. Using this function,
// the user can control the writeout process, and render a second frame while
// writing another. The returned buffer must be freed by the caller.
API int notcurses_render_to_buffer(struct notcurses* nc, char** buf, size_t* buflen);

// Write the last rendered frame, in its entirety, to 'fp'. If
// notcurses_render() has not yet been called, nothing will be written.
API int notcurses_render_to_file(struct notcurses* nc, FILE* fp);

// Return the topmost ncplane, of which there is always at least one.
API struct ncplane* notcurses_top(struct notcurses* n);

// Return the bottommost ncplane, of which there is always at least one.
API struct ncplane* notcurses_bottom(struct notcurses* n);

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

// compare two ncinput structs for data equality. we can't just use memcmp()
// due to potential padding in the struct (especially wrt bools) and seqnum.
static inline bool
ncinput_equal_p(const struct ncinput* n1, const struct ncinput* n2){
  if(n1->id != n2->id){
    return false;
  }
  if(n1->y != n2->y || n1->x != n2->x){
    return false;
  }
  if(n1->alt != n2->alt || n1->shift != n2->shift || n1->ctrl != n2->ctrl){
    return false;
  }
  // do not check seqnum!
  return true;
}

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
// descriptor becomes available, you can call notcurses_getc_nblock(),
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

// Disable signals originating from the terminal's line discipline, i.e.
// SIGINT (^C), SIGQUIT (^\), and SIGTSTP (^Z). They are enabled by default.
API int notcurses_linesigs_disable(struct notcurses* n);

// Restore signals originating from the terminal's line discipline, i.e.
// SIGINT (^C), SIGQUIT (^\), and SIGTSTP (^Z), if disabled.
API int notcurses_linesigs_enable(struct notcurses* n);

// Refresh the physical screen to match what was last rendered (i.e., without
// reflecting any changes since the last call to notcurses_render()). This is
// primarily useful if the screen is externally corrupted, or if an
// NCKEY_RESIZE event has been read and you're not yet ready to render.
API int notcurses_refresh(struct notcurses* n, int* RESTRICT y, int* RESTRICT x);

// Extract the Notcurses context to which this plane is attached.
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

static inline const struct ncplane*
notcurses_stddim_yx_const(const struct notcurses* nc, int* RESTRICT y, int* RESTRICT x){
  const struct ncplane* s = notcurses_stdplane_const(nc); // can't fail
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

// Retrieve the contents of the specified cell as last rendered. Returns the EGC
// or NULL on error. This EGC must be free()d by the caller. The stylemask and
// channels are written to 'stylemask' and 'channels', respectively.
API char* notcurses_at_yx(struct notcurses* nc, int yoff, int xoff,
                          uint16_t* stylemask, uint64_t* channels);

// Horizontal alignment relative to the parent plane. Use 'align' instead of 'x'.
#define NCPLANE_OPTION_HORALIGNED 0x0001ull

typedef struct ncplane_options {
  int y;            // vertical placement relative to parent plane
  int x;            // horizontal placement relative to parent plane
  int rows;         // number of rows, must be positive
  int cols;         // number of columns, must be positive
  void* userptr;    // user curry, may be NULL
  const char* name; // name (used only for debugging), may be NULL
  int (*resizecb)(struct ncplane*); // callback when parent is resized
  uint64_t flags;   // closure over NCPLANE_OPTION_*
} ncplane_options;

// Create a new ncplane bound to plane 'n', at the offset 'y'x'x' (relative to
// the origin of 'n') and the specified size. The number of 'rows' and 'cols'
// must both be positive. This plane is initially at the top of the z-buffer,
// as if ncplane_move_top() had been called on it. The void* 'userptr' can be
// retrieved (and reset) later. A 'name' can be set, used in debugging.
API ALLOC struct ncplane* ncplane_create(struct ncplane* n, const ncplane_options* nopts);

// Same as ncplane_create(), but creates a new pile. The returned plane will
// be the top, bottom, and root of this new pile.
API ALLOC struct ncplane* ncpile_create(struct notcurses* nc, const ncplane_options* nopts);

// This function will be removed in ABI3 in favor of ncplane_create().
// It persists in ABI2 only for backwards compatibility.
API ALLOC struct ncplane* ncplane_new(struct ncplane* n, int rows, int cols, int y, int x, void* opaque, const char* name)
  __attribute__ ((deprecated));

// Suitable for use as a 'resizecb', this will resize the plane to the visual
// region's size. It is used for the standard plane.
API int ncplane_resize_maximize(struct ncplane* n);

// Suitable for use as a 'resizecb'. This will realign the plane 'n' against
// its parent, using the alignment specified at ncplane_create()-time.
API int ncplane_resize_realign(struct ncplane* n);

// Replace the ncplane's existing resizecb with 'resizecb' (which may be NULL).
// The standard plane's resizecb may not be changed.
API void ncplane_set_resizecb(struct ncplane* n, int(*resizecb)(struct ncplane*));

// Returns the ncplane's current resize callback.
API int (*ncplane_resizecb(const struct ncplane* n))(struct ncplane*);

// Plane 'n' will be unbound from its parent plane, and will be made a bound
// child of 'newparent'. It is an error if 'n' or 'newparent' are NULL. If
// 'newparent' is equal to 'n', 'n' becomes the root of a new pile, unless 'n'
// is already the root of a pile, in which case this is a no-op. Returns 'n'.
// The standard plane cannot be reparented. Any planes bound to 'n' are
// reparented to the previous parent of 'n'.
API struct ncplane* ncplane_reparent(struct ncplane* n, struct ncplane* newparent);

// The same as ncplane_reparent(), except any planes bound to 'n' come along
// with it to its new destination. Their z-order is maintained. If 'newparent'
// is an ancestor of 'n', NULL is returned, and no changes are made.
API struct ncplane* ncplane_reparent_family(struct ncplane* n, struct ncplane* newparent);

// Duplicate an existing ncplane. The new plane will have the same geometry,
// will duplicate all content, and will start with the same rendering state.
// The new plane will be immediately above the old one on the z axis, and will
// be bound to the same parent. Bound planes are *not* duplicated; the new
// plane is bound to the parent of 'n', but has no bound planes.
API ALLOC struct ncplane* ncplane_dup(const struct ncplane* n, void* opaque);

// provided a coordinate relative to the origin of 'src', map it to the same
// absolute coordinate relative to the origin of 'dst'. either or both of 'y'
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
API unsigned notcurses_palette_size(const struct notcurses* nc);

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

// Can we reliably use Unicode 13 sextants?
API bool notcurses_cansextant(const struct notcurses* nc);

// Can we reliably use Unicode Braille?
API bool notcurses_canbraille(const struct notcurses* nc);

// Can we blit to Sixel?
API bool notcurses_cansixel(const struct notcurses* nc);

typedef struct ncstats {
  // purely increasing stats
  uint64_t renders;          // successful ncpile_render() runs
  uint64_t writeouts;        // successful ncpile_rasterize() runs
  uint64_t failed_renders;   // aborted renders, should be 0
  uint64_t failed_writeouts; // aborted writes
  // FIXME these next three all ought be "writeout" or "raster"
  uint64_t render_bytes;     // bytes emitted to ttyfp
  int64_t render_max_bytes;  // max bytes emitted for a frame
  int64_t render_min_bytes;  // min bytes emitted for a frame
  uint64_t render_ns;        // nanoseconds spent rendering
  int64_t render_max_ns;     // max ns spent in render for a frame
  int64_t render_min_ns;     // min ns spent in render for a frame
  uint64_t writeout_ns;      // nanoseconds spent writing frames to terminal
  int64_t writeout_max_ns;   // max ns spent writing out a frame
  int64_t writeout_min_ns;   // min ns spent writing out a frame
  uint64_t cellelisions;     // cells we elided entirely thanks to damage maps
  uint64_t cellemissions;    // total number of cells emitted to terminal
  uint64_t fgelisions;       // RGB fg elision count
  uint64_t fgemissions;      // RGB fg emissions
  uint64_t bgelisions;       // RGB bg elision count
  uint64_t bgemissions;      // RGB bg emissions
  uint64_t defaultelisions;  // default color was emitted
  uint64_t defaultemissions; // default color was elided
  uint64_t refreshes;        // refresh requests (non-optimized redraw)

  // current state -- these can decrease
  uint64_t fbbytes;          // total bytes devoted to all active framebuffers
  unsigned planes;           // number of planes currently in existence

  // FIXME placed here for ABI compatibility; move up for ABI3
  uint64_t raster_ns;        // nanoseconds spent rasterizing
  int64_t raster_max_ns;     // max ns spent in raster for a frame
  int64_t raster_min_ns;     // min ns spent in raster for a frame
} ncstats;

// Allocate an ncstats object. Use this rather than allocating your own, since
// future versions of Notcurses might enlarge this structure.
API ALLOC ncstats* notcurses_stats_alloc(const struct notcurses* nc);

// Acquire an atomic snapshot of the Notcurses object's stats.
API void notcurses_stats(const struct notcurses* nc, ncstats* stats);

// Reset all cumulative stats (immediate ones, such as fbbytes, are not reset).
API void notcurses_stats_reset(struct notcurses* nc, ncstats* stats);

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
API int ncplane_destroy(struct ncplane* n);

// Set the ncplane's base nccell to 'c'. The base cell is used for purposes of
// rendering anywhere that the ncplane's gcluster is 0. Note that the base cell
// is not affected by ncplane_erase(). 'c' must not be a secondary cell from a
// multicolumn EGC.
API int ncplane_set_base_cell(struct ncplane* n, const nccell* c);

// Set the ncplane's base nccell. It will be used for purposes of rendering
// anywhere that the ncplane's gcluster is 0. Note that the base cell is not
// affected by ncplane_erase(). 'egc' must be an extended grapheme cluster.
API int ncplane_set_base(struct ncplane* n, const char* egc,
                         uint32_t stylemask, uint64_t channels);

// Extract the ncplane's base nccell into 'c'. The reference is invalidated if
// 'ncp' is destroyed.
API int ncplane_base(struct ncplane* n, nccell* c);

// Move this plane relative to the standard plane, or the plane to which it is
// bound (if it is bound to a plane). It is an error to attempt to move the
// standard plane.
API int ncplane_move_yx(struct ncplane* n, int y, int x);

// Get the origin of plane 'n' relative to its bound plane, or pile (if 'n' is
// a root plane). To get absolute coordinates, use ncplane_abs_yx().
API void ncplane_yx(const struct ncplane* n, int* RESTRICT y, int* RESTRICT x);
API int ncplane_y(const struct ncplane* n);
API int ncplane_x(const struct ncplane* n);

// Get the origin of plane 'n' relative to its pile. Either or both of 'x' and
// 'y' may be NULL.
API void ncplane_abs_yx(const struct ncplane* n, int* RESTRICT y, int* RESTRICT x);
API int ncplane_abs_y(const struct ncplane* n);
API int ncplane_abs_x(const struct ncplane* n);

// Get the plane to which the plane 'n' is bound, if any.
API struct ncplane* ncplane_parent(struct ncplane* n);
API const struct ncplane* ncplane_parent_const(const struct ncplane* n);

// Return non-zero iff 'n' is a proper descendent of 'ancestor'.
static inline int
ncplane_descendant_p(const struct ncplane* n, const struct ncplane* ancestor){
  for(const struct ncplane* parent = ncplane_parent_const(n) ; parent != ancestor ; parent = ncplane_parent_const(parent)){
    if(ncplane_parent_const(parent) == parent){ // reached a root plane
      return 0;
    }
  }
  return 1;
}

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
API struct ncplane* ncplane_above(struct ncplane* n);

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
// stylemask and channels are written to 'stylemask' and 'channels', respectively.
API char* ncplane_at_cursor(struct ncplane* n, uint16_t* stylemask, uint64_t* channels);

// Retrieve the current contents of the cell under the cursor into 'c'. This
// cell is invalidated if the associated plane is destroyed. Returns the number
// of bytes in the EGC, or -1 on error.
API int ncplane_at_cursor_cell(struct ncplane* n, nccell* c);

// Retrieve the current contents of the specified cell. The EGC is returned, or
// NULL on error. This EGC must be free()d by the caller. The stylemask and
// channels are written to 'stylemask' and 'channels', respectively.
API char* ncplane_at_yx(const struct ncplane* n, int y, int x,
                        uint16_t* stylemask, uint64_t* channels);

// Retrieve the current contents of the specified cell into 'c'. This cell is
// invalidated if the associated plane is destroyed. Returns the number of
// bytes in the EGC, or -1 on error.
API int ncplane_at_yx_cell(struct ncplane* n, int y, int x, nccell* c);

// Create a flat string from the EGCs of the selected region of the ncplane
// 'n'. Start at the plane's 'begy'x'begx' coordinate (which must lie on the
// plane), continuing for 'leny'x'lenx' cells. Either or both of 'leny' and
// 'lenx' can be specified as -1 to go through the boundary of the plane.
API char* ncplane_contents(const struct ncplane* n, int begy, int begx,
                           int leny, int lenx);

// Manipulate the opaque user pointer associated with this plane.
// ncplane_set_userptr() returns the previous userptr after replacing
// it with 'opaque'. the others simply return the userptr.
API void* ncplane_set_userptr(struct ncplane* n, void* opaque);
API void* ncplane_userptr(struct ncplane* n);

API void ncplane_center_abs(const struct ncplane* n, int* RESTRICT y,
                            int* RESTRICT x);

// Return the offset into 'availcols' at which 'cols' ought be output given the
// requirements of 'align'. Return -INT_MAX if unaligned or in case of invalid
// 'align'. Undefined behavior on negative 'cols'.
static inline int
notcurses_align(int availcols, ncalign_e align, int cols){
  if(cols < 0){
    return -INT_MAX;
  }
  if(align == NCALIGN_LEFT){
    return 0;
  }
  if(cols > availcols){
    return 0;
  }
  if(align == NCALIGN_CENTER){
    return (availcols - cols) / 2;
  }
  if(align == NCALIGN_RIGHT){
    return availcols - cols;
  }
  return -INT_MAX;
}

// Return the column at which 'c' cols ought start in order to be aligned
// according to 'align' within ncplane 'n'. Return -INT_MAX if unaligned
// or in case of invalid 'align'. Undefined behavior on negative 'c'.
static inline int
ncplane_align(const struct ncplane* n, ncalign_e align, int c){
  return notcurses_align(ncplane_dim_x(n), align, c);
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

// Return the current styling for this ncplane.
API uint16_t ncplane_styles(const struct ncplane* n);

// Replace the cell at the specified coordinates with the provided cell 'c',
// and advance the cursor by the width of the cell (but not past the end of the
// plane). On success, returns the number of columns the cursor was advanced.
// 'c' must already be associated with 'n'. On failure, -1 is returned.
API int ncplane_putc_yx(struct ncplane* n, int y, int x, const nccell* c);

// Call ncplane_putc_yx() for the current cursor location.
static inline int
ncplane_putc(struct ncplane* n, const nccell* c){
  return ncplane_putc_yx(n, -1, -1, c);
}

// Replace the cell at the specified coordinates with the provided 7-bit char
// 'c'. Advance the cursor by 1. On success, returns 1. On failure, returns -1.
// This works whether the underlying char is signed or unsigned.
static inline int
ncplane_putchar_yx(struct ncplane* n, int y, int x, char c){
  nccell ce = CELL_INITIALIZER((uint32_t)c, ncplane_styles(n), ncplane_channels(n));
  return ncplane_putc_yx(n, y, x, &ce);
}

// Call ncplane_putchar_yx() at the current cursor location.
static inline int
ncplane_putchar(struct ncplane* n, char c){
  return ncplane_putchar_yx(n, -1, -1, c);
}

// Replace the EGC underneath us, but retain the styling. The current styling
// of the plane will not be changed.
API int ncplane_putchar_stained(struct ncplane* n, char c);

// Replace the cell at the specified coordinates with the provided EGC, and
// advance the cursor by the width of the cluster (but not past the end of the
// plane). On success, returns the number of columns the cursor was advanced.
// On failure, -1 is returned. The number of bytes converted from gclust is
// written to 'sbytes' if non-NULL.
API int ncplane_putegc_yx(struct ncplane* n, int y, int x, const char* gclust, int* sbytes);

// Call ncplane_putegc_yx() at the current cursor location.
static inline int
ncplane_putegc(struct ncplane* n, const char* gclust, int* sbytes){
  return ncplane_putegc_yx(n, -1, -1, gclust, sbytes);
}

// Replace the EGC underneath us, but retain the styling. The current styling
// of the plane will not be changed.
API int ncplane_putegc_stained(struct ncplane* n, const char* gclust, int* sbytes);

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
API int ncplane_putwegc_stained(struct ncplane* n, const wchar_t* gclust, int* sbytes);

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

// Replace a string's worth of glyphs at the current cursor location, but
// retain the styling. The current styling of the plane will not be changed.
API int ncplane_putstr_stained(struct ncplane* n, const char* s);

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
  if(xpos < 0){
    return -1;
  }
  return ncplane_putwstr_yx(n, y, xpos, gclustarr);
}

API int ncplane_putwstr_stained(struct ncplane* n, const wchar_t* gclustarr);

static inline int
ncplane_putwstr(struct ncplane* n, const wchar_t* gclustarr){
  return ncplane_putwstr_yx(n, -1, -1, gclustarr);
}

// Replace the cell at the specified coordinates with the provided wide char
// 'w'. Advance the cursor by the character's width as reported by wcwidth().
// On success, returns the number of columns written. On failure, returns -1.
static inline int
ncplane_putwc_yx(struct ncplane* n, int y, int x, wchar_t w){
  wchar_t warr[2] = { w, L'\0' };
  return ncplane_putwstr_yx(n, y, x, warr);
}

// Write 'w' at the current cursor position, using the plane's current styling.
static inline int
ncplane_putwc(struct ncplane* n, wchar_t w){
  return ncplane_putwc_yx(n, -1, -1, w);
}

// Write 'w' at the current cursor position, using any preexisting styling
// at that cell.
static inline int
ncplane_putwc_stained(struct ncplane* n, wchar_t w){
  wchar_t warr[2] = { w, L'\0' };
  return ncplane_putwstr_stained(n, warr);
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

API int ncplane_vprintf_stained(struct ncplane* n, const char* format, va_list ap);

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

static inline int
ncplane_printf_stained(struct ncplane* n, const char* format, ...)
  __attribute__ ((format (printf, 2, 3)));

static inline int
ncplane_printf_stained(struct ncplane* n, const char* format, ...){
  va_list va;
  va_start(va, format);
  int ret = ncplane_vprintf_stained(n, format, va);
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
// determine whether the write completed by inspecting '*bytes'. Can output to
// multiple rows even in the absence of scrolling, but not more rows than are
// available. With scrolling enabled, arbitrary amounts of data can be emitted.
// All provided whitespace is preserved -- ncplane_puttext() followed by an
// appropriate ncplane_contents() will read back the original output.
//
// If 'y' is -1, the first row of output is taken relative to the current
// cursor: it will be left-, right-, or center-aligned in whatever remains
// of the row. On subsequent rows -- or if 'y' is not -1 -- the entire row can
// be used, and alignment works normally.
//
// A newline at any point will move the cursor to the next row.
API int ncplane_puttext(struct ncplane* n, int y, ncalign_e align,
                        const char* text, size_t* bytes);

// Draw horizontal or vertical lines using the specified cell, starting at the
// current cursor position. The cursor will end at the cell following the last
// cell output (even, perhaps counter-intuitively, when drawing vertical
// lines), just as if ncplane_putc() was called at that spot. Return the
// number of cells drawn on success. On error, return the negative number of
// cells drawn.
API int ncplane_hline_interp(struct ncplane* n, const nccell* c, int len,
                             uint64_t c1, uint64_t c2);

static inline int
ncplane_hline(struct ncplane* n, const nccell* c, int len){
  return ncplane_hline_interp(n, c, len, c->channels, c->channels);
}

API int ncplane_vline_interp(struct ncplane* n, const nccell* c, int len,
                             uint64_t c1, uint64_t c2);

static inline int
ncplane_vline(struct ncplane* n, const nccell* c, int len){
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
// are never drawn (since at most 2 edges can touch a box's corner).
API int ncplane_box(struct ncplane* n, const nccell* ul, const nccell* ur,
                    const nccell* ll, const nccell* lr, const nccell* hline,
                    const nccell* vline, int ystop, int xstop,
                    unsigned ctlword);

// Draw a box with its upper-left corner at the current cursor position, having
// dimensions 'ylen'x'xlen'. See ncplane_box() for more information. The
// minimum box size is 2x2, and it cannot be drawn off-screen.
static inline int
ncplane_box_sized(struct ncplane* n, const nccell* ul, const nccell* ur,
                  const nccell* ll, const nccell* lr, const nccell* hline,
                  const nccell* vline, int ylen, int xlen, unsigned ctlword){
  int y, x;
  ncplane_cursor_yx(n, &y, &x);
  return ncplane_box(n, ul, ur, ll, lr, hline, vline, y + ylen - 1,
                     x + xlen - 1, ctlword);
}

static inline int
ncplane_perimeter(struct ncplane* n, const nccell* ul, const nccell* ur,
                  const nccell* ll, const nccell* lr, const nccell* hline,
                  const nccell* vline, unsigned ctlword){
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
API int ncplane_polyfill_yx(struct ncplane* n, int y, int x, const nccell* c);

// Draw a gradient with its upper-left corner at the current cursor position,
// stopping at 'ystop'x'xstop'. The glyph composed of 'egc' and 'stylemask' is
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
API int ncplane_gradient(struct ncplane* n, const char* egc, uint32_t stylemask,
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
ncplane_gradient_sized(struct ncplane* n, const char* egc, uint32_t stylemask,
                       uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr,
                       int ylen, int xlen){
  if(ylen < 1 || xlen < 1){
    return -1;
  }
  int y, x;
  ncplane_cursor_yx(n, &y, &x);
  return ncplane_gradient(n, egc, stylemask, ul, ur, ll, lr,
                          y + ylen - 1, x + xlen - 1);
}

// ncplane_gradent_sized() meets ncplane_highgradient().
API int ncplane_highgradient_sized(struct ncplane* n, uint32_t ul, uint32_t ur,
                                   uint32_t ll, uint32_t lr, int ylen, int xlen);

// Set the given style throughout the specified region, keeping content and
// channels unchanged. Returns the number of cells set, or -1 on failure.
API int ncplane_format(struct ncplane* n, int ystop, int xstop, uint32_t stylemask);

// Set the given channels throughout the specified region, keeping content and
// attributes unchanged. Returns the number of cells set, or -1 on failure.
API int ncplane_stain(struct ncplane* n, int ystop, int xstop, uint64_t ul,
                      uint64_t ur, uint64_t ll, uint64_t lr);

// If 'src' does not intersect with 'dst', 'dst' will not be changed, but it is
// not an error. If 'dst' is NULL, the operation will target the standard plane.
API int ncplane_mergedown_simple(const struct ncplane* RESTRICT src,
                                 struct ncplane* RESTRICT dst);

// Merge the ncplane 'src' down onto the ncplane 'dst'. This is most rigorously
// defined as "write to 'dst' the frame that would be rendered were the entire
// stack made up only of the specified subregion of 'src' and, below it, the
// subregion of 'dst' having the specified origin. Merging is independent of
// the position of 'src' viz 'dst' on the z-axis. It is an error to define a
// subregion of zero area, or that is not entirely contained within 'src'. It
// is an error to define a target origin such that the projected subregion is
// not entirely contained within 'dst'.  Behavior is undefined if 'src' and
// 'dst' are equivalent. 'dst' is modified, but 'src' remains unchanged.
API int ncplane_mergedown(const struct ncplane* RESTRICT src,
                          struct ncplane* RESTRICT dst,
                          int begsrcy, int begsrcx, int leny, int lenx,
                          int dsty, int dstx);

// Erase every cell in the ncplane (each cell is initialized to the null glyph
// and the default channels/styles). All cells associated with this ncplane are
// invalidated, and must not be used after the call, *excluding* the base cell.
// The cursor is homed. The plane's active attributes are unaffected.
API void ncplane_erase(struct ncplane* n);

// Extract the 32-bit background channel from a cell.
static inline uint32_t
cell_bchannel(const nccell* cl){
  return channels_bchannel(cl->channels);
}

// Extract the 32-bit foreground channel from a cell.
static inline uint32_t
cell_fchannel(const nccell* cl){
  return channels_fchannel(cl->channels);
}

// Set the 32-bit background channel of an nccell.
static inline uint64_t
cell_set_bchannel(nccell* cl, uint32_t channel){
  return channels_set_bchannel(&cl->channels, channel);
}

// Set the 32-bit foreground channel of an nccell.
static inline uint64_t
cell_set_fchannel(nccell* cl, uint32_t channel){
  return channels_set_fchannel(&cl->channels, channel);
}

// Extract 24 bits of foreground RGB from 'cl', shifted to LSBs.
static inline uint32_t
cell_fg_rgb(const nccell* cl){
  return channels_fg_rgb(cl->channels);
}

// Extract 24 bits of background RGB from 'cl', shifted to LSBs.
static inline uint32_t
cell_bg_rgb(const nccell* cl){
  return channels_bg_rgb(cl->channels);
}

// Extract 2 bits of foreground alpha from 'cl', shifted to LSBs.
static inline uint32_t
cell_fg_alpha(const nccell* cl){
  return channels_fg_alpha(cl->channels);
}

// Extract 2 bits of background alpha from 'cl', shifted to LSBs.
static inline uint32_t
cell_bg_alpha(const nccell* cl){
  return channels_bg_alpha(cl->channels);
}

// Extract 24 bits of foreground RGB from 'cl', split into components.
static inline uint32_t
cell_fg_rgb8(const nccell* cl, unsigned* r, unsigned* g, unsigned* b){
  return channels_fg_rgb8(cl->channels, r, g, b);
}

// Extract 24 bits of background RGB from 'cl', split into components.
static inline uint32_t
cell_bg_rgb8(const nccell* cl, unsigned* r, unsigned* g, unsigned* b){
  return channels_bg_rgb8(cl->channels, r, g, b);
}

// Set the r, g, and b cell for the foreground component of this 64-bit
// 'cl' variable, and mark it as not using the default color.
static inline int
cell_set_fg_rgb8(nccell* cl, int r, int g, int b){
  return channels_set_fg_rgb8(&cl->channels, r, g, b);
}

// Same, but clipped to [0..255].
static inline void
cell_set_fg_rgb8_clipped(nccell* cl, int r, int g, int b){
  channels_set_fg_rgb8_clipped(&cl->channels, r, g, b);
}

// Same, but with an assembled 24-bit RGB value.
static inline int
cell_set_fg_rgb(nccell* c, uint32_t channel){
  return channels_set_fg_rgb(&c->channels, channel);
}

// Set the cell's foreground palette index, set the foreground palette index
// bit, set it foreground-opaque, and clear the foreground default color bit.
static inline int
cell_set_fg_palindex(nccell* cl, int idx){
  return channels_set_fg_palindex(&cl->channels, idx);
}

static inline uint32_t
cell_fg_palindex(const nccell* cl){
  return channels_fg_palindex(cl->channels);
}

// Set the r, g, and b cell for the background component of this 64-bit
// 'cl' variable, and mark it as not using the default color.
static inline int
cell_set_bg_rgb8(nccell* cl, int r, int g, int b){
  return channels_set_bg_rgb8(&cl->channels, r, g, b);
}

// Same, but clipped to [0..255].
static inline void
cell_set_bg_rgb8_clipped(nccell* cl, int r, int g, int b){
  channels_set_bg_rgb8_clipped(&cl->channels, r, g, b);
}

// Same, but with an assembled 24-bit RGB value. A value over 0xffffff
// will be rejected, with a non-zero return value.
static inline int
cell_set_bg_rgb(nccell* c, uint32_t channel){
  return channels_set_bg_rgb(&c->channels, channel);
}

// Set the cell's background palette index, set the background palette index
// bit, set it background-opaque, and clear the background default color bit.
static inline int
cell_set_bg_palindex(nccell* cl, int idx){
  return channels_set_bg_palindex(&cl->channels, idx);
}

static inline uint32_t
cell_bg_palindex(const nccell* cl){
  return channels_bg_palindex(cl->channels);
}

// Is the foreground using the "default foreground color"?
static inline bool
cell_fg_default_p(const nccell* cl){
  return channels_fg_default_p(cl->channels);
}

static inline bool
cell_fg_palindex_p(const nccell* cl){
  return channels_fg_palindex_p(cl->channels);
}

// Is the background using the "default background color"? The "default
// background color" must generally be used to take advantage of
// terminal-effected transparency.
static inline bool
cell_bg_default_p(const nccell* cl){
  return channels_bg_default_p(cl->channels);
}

static inline bool
cell_bg_palindex_p(const nccell* cl){
  return channels_bg_palindex_p(cl->channels);
}

// Extract the 32-bit working background channel from an ncplane.
static inline uint32_t
ncplane_bchannel(const struct ncplane* n){
  return channels_bchannel(ncplane_channels(n));
}

// Extract the 32-bit working foreground channel from an ncplane.
static inline uint32_t
ncplane_fchannel(const struct ncplane* n){
  return channels_fchannel(ncplane_channels(n));
}

API void ncplane_set_channels(struct ncplane* n, uint64_t channels);

// Set the specified style bits for the ncplane 'n', whether they're actively
// supported or not.
API void ncplane_set_styles(struct ncplane* n, unsigned stylebits);

// Add the specified styles to the ncplane's existing spec.
API void ncplane_on_styles(struct ncplane* n, unsigned stylebits);

// Remove the specified styles from the ncplane's existing spec.
API void ncplane_off_styles(struct ncplane* n, unsigned stylebits);

// Deprecated forms of above.
API void ncplane_styles_set(struct ncplane* n, unsigned stylebits)
  __attribute__ ((deprecated));
API void ncplane_styles_on(struct ncplane* n, unsigned stylebits)
  __attribute__ ((deprecated));
API void ncplane_styles_off(struct ncplane* n, unsigned stylebits)
  __attribute__ ((deprecated));

// Extract 24 bits of working foreground RGB from an ncplane, shifted to LSBs.
static inline uint32_t
ncplane_fg_rgb(const struct ncplane* n){
  return channels_fg_rgb(ncplane_channels(n));
}

// Extract 24 bits of working background RGB from an ncplane, shifted to LSBs.
static inline uint32_t
ncplane_bg_rgb(const struct ncplane* n){
  return channels_bg_rgb(ncplane_channels(n));
}

// Extract 2 bits of foreground alpha from 'struct ncplane', shifted to LSBs.
static inline uint32_t
ncplane_fg_alpha(const struct ncplane* n){
  return channels_fg_alpha(ncplane_channels(n));
}

// Is the plane's foreground using the "default foreground color"?
static inline bool
ncplane_fg_default_p(const struct ncplane* n){
  return channels_fg_default_p(ncplane_channels(n));
}

// Extract 2 bits of background alpha from 'struct ncplane', shifted to LSBs.
static inline uint32_t
ncplane_bg_alpha(const struct ncplane* n){
  return channels_bg_alpha(ncplane_channels(n));
}

// Is the plane's background using the "default background color"?
static inline bool
ncplane_bg_default_p(const struct ncplane* n){
  return channels_bg_default_p(ncplane_channels(n));
}

// Extract 24 bits of foreground RGB from 'n', split into components.
static inline uint32_t
ncplane_fg_rgb8(const struct ncplane* n, unsigned* r, unsigned* g, unsigned* b){
  return channels_fg_rgb8(ncplane_channels(n), r, g, b);
}

// Extract 24 bits of background RGB from 'n', split into components.
static inline uint32_t
ncplane_bg_rgb8(const struct ncplane* n, unsigned* r, unsigned* g, unsigned* b){
  return channels_bg_rgb8(ncplane_channels(n), r, g, b);
}

// Set an entire 32-bit channel of the plane
API uint64_t ncplane_set_fchannel(struct ncplane* n, uint32_t channel);
API uint64_t ncplane_set_bchannel(struct ncplane* n, uint32_t channel);

// Set the current fore/background color using RGB specifications. If the
// terminal does not support directly-specified 3x8b cells (24-bit "TrueColor",
// indicated by the "RGB" terminfo capability), the provided values will be
// interpreted in some lossy fashion. None of r, g, or b may exceed 255.
// "HP-like" terminals require setting foreground and background at the same
// time using "color pairs"; Notcurses will manage color pairs transparently.
API int ncplane_set_fg_rgb8(struct ncplane* n, int r, int g, int b);
API int ncplane_set_bg_rgb8(struct ncplane* n, int r, int g, int b);

// Same, but clipped to [0..255].
API void ncplane_set_bg_rgb8_clipped(struct ncplane* n, int r, int g, int b);
API void ncplane_set_fg_rgb8_clipped(struct ncplane* n, int r, int g, int b);

// Same, but with rgb assembled into a channel (i.e. lower 24 bits).
API int ncplane_set_fg_rgb(struct ncplane* n, uint32_t channel);
API int ncplane_set_bg_rgb(struct ncplane* n, uint32_t channel);

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

// Called for each fade iteration on 'ncp'. If anything but 0 is returned,
// the fading operation ceases immediately, and that value is propagated out.
// The recommended absolute display time target is passed in 'tspec'.
typedef int (*fadecb)(struct notcurses* nc, struct ncplane* n,
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
cells_load_box(struct ncplane* n, uint32_t styles, uint64_t channels,
               nccell* ul, nccell* ur, nccell* ll, nccell* lr,
               nccell* hl, nccell* vl, const char* gclusters){
  int ulen;
  if((ulen = cell_prime(n, ul, gclusters, styles, channels)) > 0){
    if((ulen = cell_prime(n, ur, gclusters += ulen, styles, channels)) > 0){
      if((ulen = cell_prime(n, ll, gclusters += ulen, styles, channels)) > 0){
        if((ulen = cell_prime(n, lr, gclusters += ulen, styles, channels)) > 0){
          if((ulen = cell_prime(n, hl, gclusters += ulen, styles, channels)) > 0){
            if(cell_prime(n, vl, gclusters + ulen, styles, channels) > 0){
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

API int cells_rounded_box(struct ncplane* n, uint32_t styles, uint64_t channels,
                          nccell* ul, nccell* ur, nccell* ll,
                          nccell* lr, nccell* hl, nccell* vl);

static inline int
ncplane_rounded_box(struct ncplane* n, uint32_t styles, uint64_t channels,
                    int ystop, int xstop, unsigned ctlword){
  int ret = 0;
  nccell ul = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
  nccell ll = CELL_TRIVIAL_INITIALIZER, lr = CELL_TRIVIAL_INITIALIZER;
  nccell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
  if((ret = cells_rounded_box(n, styles, channels, &ul, &ur, &ll, &lr, &hl, &vl)) == 0){
    ret = ncplane_box(n, &ul, &ur, &ll, &lr, &hl, &vl, ystop, xstop, ctlword);
  }
  cell_release(n, &ul); cell_release(n, &ur);
  cell_release(n, &ll); cell_release(n, &lr);
  cell_release(n, &hl); cell_release(n, &vl);
  return ret;
}

static inline int
ncplane_perimeter_rounded(struct ncplane* n, uint32_t stylemask,
                          uint64_t channels, unsigned ctlword){
  if(ncplane_cursor_move_yx(n, 0, 0)){
    return -1;
  }
  int dimy, dimx;
  ncplane_dim_yx(n, &dimy, &dimx);
  nccell ul = CELL_TRIVIAL_INITIALIZER;
  nccell ur = CELL_TRIVIAL_INITIALIZER;
  nccell ll = CELL_TRIVIAL_INITIALIZER;
  nccell lr = CELL_TRIVIAL_INITIALIZER;
  nccell vl = CELL_TRIVIAL_INITIALIZER;
  nccell hl = CELL_TRIVIAL_INITIALIZER;
  if(cells_rounded_box(n, stylemask, channels, &ul, &ur, &ll, &lr, &hl, &vl)){
    return -1;
  }
  int r = ncplane_box_sized(n, &ul, &ur, &ll, &lr, &hl, &vl, dimy, dimx, ctlword);
  cell_release(n, &ul); cell_release(n, &ur);
  cell_release(n, &ll); cell_release(n, &lr);
  cell_release(n, &hl); cell_release(n, &vl);
  return r;
}

static inline int
ncplane_rounded_box_sized(struct ncplane* n, uint32_t styles, uint64_t channels,
                          int ylen, int xlen, unsigned ctlword){
  int y, x;
  ncplane_cursor_yx(n, &y, &x);
  return ncplane_rounded_box(n, styles, channels, y + ylen - 1,
                             x + xlen - 1, ctlword);
}

API int cells_double_box(struct ncplane* n, uint32_t styles, uint64_t channels,
                         nccell* ul, nccell* ur, nccell* ll,
                         nccell* lr, nccell* hl, nccell* vl);

static inline int
ncplane_double_box(struct ncplane* n, uint32_t styles, uint64_t channels,
                   int ystop, int xstop, unsigned ctlword){
  int ret = 0;
  nccell ul = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
  nccell ll = CELL_TRIVIAL_INITIALIZER, lr = CELL_TRIVIAL_INITIALIZER;
  nccell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
  if((ret = cells_double_box(n, styles, channels, &ul, &ur, &ll, &lr, &hl, &vl)) == 0){
    ret = ncplane_box(n, &ul, &ur, &ll, &lr, &hl, &vl, ystop, xstop, ctlword);
  }
  cell_release(n, &ul); cell_release(n, &ur);
  cell_release(n, &ll); cell_release(n, &lr);
  cell_release(n, &hl); cell_release(n, &vl);
  return ret;
}

static inline int
ncplane_perimeter_double(struct ncplane* n, uint32_t stylemask,
                         uint64_t channels, unsigned ctlword){
  if(ncplane_cursor_move_yx(n, 0, 0)){
    return -1;
  }
  int dimy, dimx;
  ncplane_dim_yx(n, &dimy, &dimx);
  nccell ul = CELL_TRIVIAL_INITIALIZER;
  nccell ur = CELL_TRIVIAL_INITIALIZER;
  nccell ll = CELL_TRIVIAL_INITIALIZER;
  nccell lr = CELL_TRIVIAL_INITIALIZER;
  nccell vl = CELL_TRIVIAL_INITIALIZER;
  nccell hl = CELL_TRIVIAL_INITIALIZER;
  if(cells_double_box(n, stylemask, channels, &ul, &ur, &ll, &lr, &hl, &vl)){
    return -1;
  }
  int r = ncplane_box_sized(n, &ul, &ur, &ll, &lr, &hl, &vl, dimy, dimx, ctlword);
  cell_release(n, &ul); cell_release(n, &ur);
  cell_release(n, &ll); cell_release(n, &lr);
  cell_release(n, &hl); cell_release(n, &vl);
  return r;
}

static inline int
ncplane_double_box_sized(struct ncplane* n, uint32_t styles, uint64_t channels,
                         int ylen, int xlen, unsigned ctlword){
  int y, x;
  ncplane_cursor_yx(n, &y, &x);
  return ncplane_double_box(n, styles, channels, y + ylen - 1,
                            x + xlen - 1, ctlword);
}

// Open a visual at 'file', extract a codec and parameters, decode the first
// image to memory.
API ALLOC struct ncvisual* ncvisual_from_file(const char* file);

// Prepare an ncvisual, and its underlying plane, based off RGBA content in
// memory at 'rgba'. 'rgba' must be a flat array of 32-bit 8bpc RGBA pixels.
// These must be arranged in 'rowstride' lines, where the first 'cols' * 4b
// are actual data. There must be 'rows' lines. The total size of 'rgba'
// must thus be at least (rows * rowstride) bytes, of which (rows * cols * 4)
// bytes are actual data. Resulting planes are ceil('rows' / 2) x 'cols'.
API ALLOC struct ncvisual* ncvisual_from_rgba(const void* rgba, int rows,
                                              int rowstride, int cols);

// ncvisual_from_rgba(), but 'bgra' is arranged as BGRA.
API ALLOC struct ncvisual* ncvisual_from_bgra(const void* rgba, int rows,
                                              int rowstride, int cols);

// Promote an ncplane 'n' to an ncvisual. The plane may contain only spaces,
// half blocks, and full blocks. The latter will be checked, and any other
// glyph will result in a NULL being returned. This function exists so that
// planes can be subjected to ncvisual transformations. If possible, it's
// better to create the ncvisual from memory using ncvisual_from_rgba().
API ALLOC struct ncvisual* ncvisual_from_plane(const struct ncplane* n,
                                               ncblitter_e blit,
                                               int begy, int begx,
                                               int leny, int lenx);

#define NCVISUAL_OPTION_NODEGRADE 0x0001ull // fail rather than degrade
#define NCVISUAL_OPTION_BLEND     0x0002ull // use CELL_ALPHA_BLEND with visual

struct ncvisual_options {
  // if no ncplane is provided, one will be created using the exact size
  // necessary to render the source with perfect fidelity (this might be
  // smaller or larger than the rendering area).
  struct ncplane* n;
  // the scaling is ignored if no ncplane is provided (it ought be NCSCALE_NONE
  // in this case). otherwise, the source is stretched/scaled relative to the
  // provided ncplane.
  ncscale_e scaling;
  // if an ncplane is provided, y and x specify where the visual will be
  // rendered on that plane. otherwise, they specify where the created ncplane
  // will be placed relative to the standard plane's origin.
  int y, x;
  // the section of the visual that ought be rendered. for the entire visual,
  // pass an origin of 0, 0 and a size of 0, 0 (or the true height and width).
  // these numbers are all in terms of ncvisual pixels. negative values are
  // prohibited.
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
API ALLOC uint32_t* ncplane_rgba(const struct ncplane* n, ncblitter_e blit,
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

// extract the next frame from an ncvisual. returns 1 on end of file, 0 on
// success, and -1 on failure.
API int ncvisual_decode(struct ncvisual* nc);

// decode the next frame ala ncvisual_decode(), but if we have reached the end,
// rewind to the first frame of the ncvisual. a subsequent 'ncvisual_render()'
// will render the first frame, as if the ncvisual had been closed and reopened.
// the return values remain the same as those of ncvisual_decode().
API int ncvisual_decode_loop(struct ncvisual* nc);

// Rotate the visual 'rads' radians. Only M_PI/2 and -M_PI/2 are
// supported at the moment, but this will change FIXME.
API int ncvisual_rotate(struct ncvisual* n, double rads);

// Resize the visual so that it is 'rows' X 'columns'. This is a lossy
// transformation, unless the size is unchanged.
API int ncvisual_resize(struct ncvisual* n, int rows, int cols)
  __attribute__ ((nonnull (1)));

// Polyfill at the specified location within the ncvisual 'n', using 'rgba'.
API int ncvisual_polyfill_yx(struct ncvisual* n, int y, int x, uint32_t rgba)
  __attribute__ ((nonnull (1)));

// Get the specified pixel from the specified ncvisual.
API int ncvisual_at_yx(const struct ncvisual* n, int y, int x, uint32_t* pixel)
  __attribute__ ((nonnull (1, 4)));

// Set the specified pixel in the specified ncvisual.
API int ncvisual_set_yx(const struct ncvisual* n, int y, int x, uint32_t pixel)
  __attribute__ ((nonnull (1)));

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
API ALLOC char* ncvisual_subtitle(const struct ncvisual* ncv);

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
                        float timescale, streamcb streamer,
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

// The ncpixel API facilitates direct management of the pixels within an
// ncvisual (ncvisuals keep a backing store of 32-bit RGBA pixels, and render
// them down to terminal graphics in ncvisual_render()).
//
// Per libav, we "store as BGRA on little-endian, and ARGB on big-endian".
// This is an RGBA *byte-order* scheme. libav emits bytes, not words. Those
// bytes are R-G-B-A. When read as words, on little endian this will be ABGR,
// and on big-endian this will be RGBA. force everything to LE ABGR, a no-op on
// and thus favoring little-endian. Take that, big-endian mafia!

// Extract the 8-bit alpha component from a pixel
static inline unsigned
ncpixel_a(uint32_t pixel){
  return (htole(pixel) & 0xff000000ul) >> 24u;
}

// Extract the 8-bit red component from an ABGR pixel
static inline unsigned
ncpixel_r(uint32_t pixel){
  return (htole(pixel) & 0x000000fful);
}

// Extract the 8-bit green component from an ABGR pixel
static inline unsigned
ncpixel_g(uint32_t pixel){
  return (htole(pixel) & 0x0000ff00ul) >> 8u;
}

// Extract the 8-bit blue component from an ABGR pixel
static inline unsigned
ncpixel_b(uint32_t pixel){
  return (htole(pixel) & 0x00ff0000ul) >> 16u;
}

// Set the 8-bit alpha component of an ABGR pixel
static inline int
ncpixel_set_a(uint32_t* pixel, int a){
  if(a > 255 || a < 0){
    return -1;
  }
  *pixel = htole((htole(*pixel) & 0x00fffffful) | (a << 24u));
  return 0;
}

// Set the 8-bit red component of an ABGR pixel
static inline int
ncpixel_set_r(uint32_t* pixel, int r){
  if(r > 255 || r < 0){
    return -1;
  }
  *pixel = htole((htole(*pixel) & 0xffffff00ul) | r);
  return 0;
}

// Set the 8-bit green component of an ABGR pixel
static inline int
ncpixel_set_g(uint32_t* pixel, int g){
  if(g > 255 || g < 0){
    return -1;
  }
  *pixel = htole((htole(*pixel) & 0xffff00fful) | (g << 8u));
  return 0;
}

// Set the 8-bit blue component of an ABGR pixel
static inline int
ncpixel_set_b(uint32_t* pixel, int b){
  if(b > 255 || b < 0){
    return -1;
  }
  *pixel = htole((htole(*pixel) & 0xff00fffful) | (b << 16u));
  return 0;
}

// Construct a libav-compatible ABGR pixel, clipping at [0, 255).
static inline uint32_t
ncpixel(int r, int g, int b){
  uint32_t pixel = 0;
  ncpixel_set_a(&pixel, 0xff);
  if(r < 0) r = 0;
  if(r > 255) r = 255;
  ncpixel_set_r(&pixel, r);
  if(g < 0) g = 0;
  if(g > 255) g = 255;
  ncpixel_set_g(&pixel, g);
  if(b < 0) b = 0;
  if(b > 255) b = 255;
  ncpixel_set_b(&pixel, b);
  return pixel;
}

// set the RGB values of an RGB pixel
static inline int
ncpixel_set_rgb8(uint32_t* pixel, int r, int g, int b){
  if(ncpixel_set_r(pixel, r) || ncpixel_set_g(pixel, g) || ncpixel_set_b(pixel, b)){
    return -1;
  }
  return 0;
}

// An ncreel is a Notcurses region devoted to displaying zero or more
// line-oriented, contained tablets between which the user may navigate. If at
// least one tablets exists, there is a "focused tablet". As much of the focused
// tablet as is possible is always displayed. If there is space left over, other
// tablets are included in the display. Tablets can come and go at any time, and
// can grow or shrink at any time.
//
// This structure is amenable to line- and page-based navigation via keystrokes,
// scrolling gestures, trackballs, scrollwheels, touchpads, and verbal commands.

// is scrolling infinite (can one move down or up forever, or is an end
// reached?). if true, 'circular' specifies how to handle the special case of
// an incompletely-filled reel.
#define NCREEL_OPTION_INFINITESCROLL 0x0001ull
// is navigation circular (does moving down from the last tablet move to the
// first, and vice versa)? only meaningful when infinitescroll is true. if
// infinitescroll is false, this must be false.
#define NCREEL_OPTION_CIRCULAR       0x0002ull

typedef struct ncreel_options {
  // Notcurses can draw a border around the ncreel, and also around the
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
  uint64_t flags;      // bitfield over NCREEL_OPTION_*
} ncreel_options;

struct nctablet;
struct ncreel;

// Take over the ncplane 'nc' and use it to draw a reel according to 'popts'.
// The plane will be destroyed by ncreel_destroy(); this transfers ownership.
API ALLOC struct ncreel* ncreel_create(struct ncplane* n, const ncreel_options* popts)
  __attribute__ ((nonnull (1)));

// Returns the ncplane on which this ncreel lives.
API struct ncplane* ncreel_plane(struct ncreel* pr)
  __attribute__ ((nonnull (1)));

// Tablet draw callback, provided a tablet (from which the ncplane and userptr
// may be extracted), and a bool indicating whether output ought be drawn from
// the top (true) or bottom (false). Returns non-negative count of output lines,
// which must be less than or equal to ncplane_dim_y(nctablet_plane(t)).
typedef int (*tabletcb)(struct nctablet* t, bool drawfromtop);

// Add a new nctablet to the provided ncreel 'nr', having the callback object
// 'opaque'. Neither, either, or both of 'after' and 'before' may be specified.
// If neither is specified, the new tablet can be added anywhere on the reel.
// If one or the other is specified, the tablet will be added before or after
// the specified tablet. If both are specified, the tablet will be added to the
// resulting location, assuming it is valid (after->next == before->prev); if
// it is not valid, or there is any other error, NULL will be returned.
API ALLOC struct nctablet* ncreel_add(struct ncreel* nr, struct nctablet* after,
                                      struct nctablet* before, tabletcb cb,
                                      void* opaque)
  __attribute__ ((nonnull (1)));

// Return the number of nctablets in the ncreel 'nr'.
API int ncreel_tabletcount(const struct ncreel* nr)
  __attribute__ ((nonnull (1)));

// Delete the tablet specified by t from the ncreel 'nr'. Returns -1 if the
// tablet cannot be found.
API int ncreel_del(struct ncreel* nr, struct nctablet* t)
  __attribute__ ((nonnull (1)));

// Redraw the ncreel 'nr' in its entirety. The reel will be cleared, and
// tablets will be lain out, using the focused tablet as a fulcrum. Tablet
// drawing callbacks will be invoked for each visible tablet.
API int ncreel_redraw(struct ncreel* nr)
  __attribute__ ((nonnull (1)));

// Offer input 'ni' to the ncreel 'nr'. If it's relevant, this function returns
// true, and the input ought not be processed further. If it's irrelevant to
// the reel, false is returned. Relevant inputs include:
//  * a mouse click on a tablet (focuses tablet)
//  * a mouse scrollwheel event (rolls reel)
//  * up, down, pgup, or pgdown (navigates among items)
API bool ncreel_offer_input(struct ncreel* nr, const struct ncinput* ni)
  __attribute__ ((nonnull (1)));

// Return the focused tablet, if any tablets are present. This is not a copy;
// be careful to use it only for the duration of a critical section.
API struct nctablet* ncreel_focused(struct ncreel* nr)
  __attribute__ ((nonnull (1)));

// Change focus to the next tablet, if one exists
API struct nctablet* ncreel_next(struct ncreel* nr)
  __attribute__ ((nonnull (1)));

// Change focus to the previous tablet, if one exists
API struct nctablet* ncreel_prev(struct ncreel* nr)
  __attribute__ ((nonnull (1)));

// Destroy an ncreel allocated with ncreel_create().
API void ncreel_destroy(struct ncreel* nr);

// Returns a pointer to a user pointer associated with this nctablet.
API void* nctablet_userptr(struct nctablet* t);

// Access the ncplane associated with nctablet 't', if one exists.
API struct ncplane* nctablet_plane(struct nctablet* t);

// Deprecated form of nctablet_plane().
API struct ncplane* nctablet_ncplane(struct nctablet* t)
  __attribute__ ((deprecated));

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
//
// You are encouraged to consult notcurses_metric(3).
API const char* ncmetric(uintmax_t val, uintmax_t decimal, char* buf,
                         int omitdec, uintmax_t mult, int uprefix)
  __attribute__ ((nonnull (3)));

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
#define NCMETRICFWIDTH(x, cols) ((int)(strlen(x) - ncstrwidth(x) + (cols)))
#define PREFIXFMT(x) NCMETRICFWIDTH((x), PREFIXCOLUMNS), (x)
#define IPREFIXFMT(x) NCMETRIXFWIDTH((x), IPREFIXCOLUMNS), (x)
#define BPREFIXFMT(x) NCMETRICFWIDTH((x), BPREFIXCOLUMNS), (x)

// Mega, kilo, gigafoo. Use PREFIXSTRLEN + 1 and PREFIXCOLUMNS.
static inline const char*
qprefix(uintmax_t val, uintmax_t decimal, char* buf, int omitdec){
  return ncmetric(val, decimal, buf, omitdec, 1000, '\0');
}

// Mibi, kebi, gibibytes sans 'i' suffix. Use IPREFIXSTRLEN + 1.
static inline const char*
iprefix(uintmax_t val, uintmax_t decimal, char* buf, int omitdec){
  return ncmetric(val, decimal, buf, omitdec, 1024, '\0');
}

// Mibi, kebi, gibibytes. Use BPREFIXSTRLEN + 1 and BPREFIXCOLUMNS.
static inline const char*
bprefix(uintmax_t val, uintmax_t decimal, char* buf, int omitdec){
  return ncmetric(val, decimal, buf, omitdec, 1024, 'i');
}

// Enable or disable the terminal's cursor, if supported, placing it at
// 'y', 'x'. Immediate effect (no need for a call to notcurses_render()).
// It is an error if 'y', 'x' lies outside the standard plane.
API int notcurses_cursor_enable(struct notcurses* nc, int y, int x);
API int notcurses_cursor_disable(struct notcurses* nc);

// Palette API. Some terminals only support 256 colors, but allow the full
// palette to be specified with arbitrary RGB colors. In all cases, it's more
// performant to use indexed colors, since it's much less data to write to the
// terminal. If you can limit yourself to 256 colors, that's probably best.

typedef struct palette256 {
  uint32_t chans[NCPALETTESIZE]; // RGB values as regular ol' channels
} palette256;

// Create a new palette store. It will be initialized with notcurses' best
// knowledge of the currently configured palette. The palette upon startup
// cannot be reliably detected, sadly.
API ALLOC palette256* palette256_new(struct notcurses* nc);

// Attempt to configure the terminal with the provided palette 'p'. Does not
// transfer ownership of 'p'; palette256_free() can (ought) still be called.
API int palette256_use(struct notcurses* nc, const palette256* p);

// Manipulate entries in the palette store 'p'. These are *not* locked.
static inline int
palette256_set_rgb8(palette256* p, int idx, int r, int g, int b){
  if(idx < 0 || (size_t)idx > sizeof(p->chans) / sizeof(*p->chans)){
    return -1;
  }
  return channel_set_rgb8(&p->chans[idx], r, g, b);
}

static inline int
palette256_set(palette256* p, int idx, unsigned rgb){
  if(idx < 0 || (size_t)idx > sizeof(p->chans) / sizeof(*p->chans)){
    return -1;
  }
  return channel_set(&p->chans[idx], rgb);
}

static inline int
palette256_get_rgb8(const palette256* p, int idx, unsigned* RESTRICT r, unsigned* RESTRICT g, unsigned* RESTRICT b){
  if(idx < 0 || (size_t)idx > sizeof(p->chans) / sizeof(*p->chans)){
    return -1;
  }
  return channel_rgb8(p->chans[idx], r, g, b);
}

// Free the palette store 'p'.
API void palette256_free(palette256* p);

// Convert the plane's content to greyscale.
API void ncplane_greyscale(struct ncplane* n);

//                                 ╭──────────────────────────╮
//                                 │This is the primary header│
//   ╭──────────────────────this is the secondary header──────╮
//   │        ↑                                               │
//   │ option1 Long text #1                                   │
//   │ option2 Long text #2                                   │
//   │ option3 Long text #3                                   │
//   │ option4 Long text #4                                   │
//   │ option5 Long text #5                                   │
//   │ option6 Long text #6                                   │
//   │        ↓                                               │
//   ╰────────────────────────────────────here's the footer───╯

// selection widget -- an ncplane with a title header and a body section. the
// body section supports infinite scrolling up and down.
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
  // default item (selected at start), must be < itemcount unless itemcount is
  // 0, in which case 'defidx' must also be 0
  unsigned defidx;
  // maximum number of options to display at once, 0 to use all available space
  unsigned maxdisplay;
  // exhaustive styling options
  uint64_t opchannels;   // option channels
  uint64_t descchannels; // description channels
  uint64_t titlechannels;// title channels
  uint64_t footchannels; // secondary and footer channels
  uint64_t boxchannels;  // border channels
  uint64_t flags;        // bitfield of NCSELECTOR_OPTION_*
} ncselector_options;

API ALLOC struct ncselector* ncselector_create(struct ncplane* n, const ncselector_options* opts)
  __attribute__ ((nonnull (1)));

// Dynamically add or delete items. It is usually sufficient to supply a static
// list of items via ncselector_options->items.
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

//                                                   ╭───────────────────╮
//                                                   │ short round title │
//╭now this secondary is also very, very, very outlandishly long, you see┤
//│  ↑                                                                   │
//│ ☐ Pa231 Protactinium-231 (162kg)                                     │
//│ ☐ U233 Uranium-233 (15kg)                                            │
//│ ☐ U235 Uranium-235 (50kg)                                            │
//│ ☐ Np236 Neptunium-236 (7kg)                                          │
//│ ☐ Np237 Neptunium-237 (60kg)                                         │
//│ ☐ Pu238 Plutonium-238 (10kg)                                         │
//│ ☐ Pu239 Plutonium-239 (10kg)                                         │
//│ ☐ Pu240 Plutonium-240 (40kg)                                         │
//│ ☐ Pu241 Plutonium-241 (13kg)                                         │
//│ ☐ Am241 Americium-241 (100kg)                                        │
//│  ↓                                                                   │
//╰────────────────────────press q to exit (there is sartrev("no exit"))─╯

// multiselection widget -- a selector supporting multiple selections.
//
// Unlike the selector widget, zero to all of the items can be selected, but
// also the widget does not support adding or removing items at runtime.
typedef struct ncmultiselector_options {
  char* title; // title may be NULL, inhibiting riser, saving two rows.
  char* secondary; // secondary may be NULL
  char* footer; // footer may be NULL
  struct ncmselector_item* items; // initial items, descriptions, and statuses
  // maximum number of options to display at once, 0 to use all available space
  unsigned maxdisplay;
  // exhaustive styling options
  uint64_t opchannels;   // option channels
  uint64_t descchannels; // description channels
  uint64_t titlechannels;// title channels
  uint64_t footchannels; // secondary and footer channels
  uint64_t boxchannels;  // border channels
  uint64_t flags;        // bitfield of NCMULTISELECTOR_OPTION_*
} ncmultiselector_options;

API ALLOC struct ncmultiselector* ncmultiselector_create(struct ncplane* n, const ncmultiselector_options* opts)
  __attribute__ ((nonnull (1)));

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
API void ncmultiselector_destroy(struct ncmultiselector* n);

// nctree widget -- a vertical browser supporting hierarchical objects.
//
// groups can be collapsed or expanded
typedef struct nctree_options {
  char* title; // title may be NULL, inhibiting riser, saving two rows.
  char* secondary; // secondary may be NULL
  char* footer; // footer may be NULL
  struct ncmselector_item* items; // initial items, descriptions, and statuses
  // maximum number of options to display at once, 0 to use all available space
  unsigned maxdisplay;
  // exhaustive styling options
  uint64_t opchannels;   // option channels
  uint64_t descchannels; // description channels
  uint64_t titlechannels;// title channels
  uint64_t footchannels; // secondary and footer channels
  uint64_t boxchannels;  // border channels
  uint64_t flags;        // bitfield of NCtree_OPTION_*
} nctree_options;

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
#define NCMENU_OPTION_HIDING 0x0002ull // hide the menu when not unrolled

typedef struct ncmenu_options {
  struct ncmenu_section* sections; // array of 'sectioncount' menu_sections
  int sectioncount;                // must be positive
  uint64_t headerchannels;         // styling for header
  uint64_t sectionchannels;        // styling for sections
  uint64_t flags;                  // flag word of NCMENU_OPTION_*
} ncmenu_options;

// Create a menu with the specified options. Menus are currently bound to an
// overall Notcurses object (as opposed to a particular plane), and are
// implemented as ncplanes kept atop other ncplanes.
API ALLOC struct ncmenu* ncmenu_create(struct ncplane* n, const ncmenu_options* opts)
  __attribute__ ((nonnull (1)));

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

// Disable or enable a menu item. Returns 0 if the item was found.
API int ncmenu_item_set_status(struct ncmenu* n, const char* section,
                               const char* item, bool enabled);

// Return the selected item description, or NULL if no section is unrolled. If
// 'ni' is not NULL, and the selected item has a shortcut, 'ni' will be filled
// in with that shortcut--this can allow faster matching.
API const char* ncmenu_selected(const struct ncmenu* n, struct ncinput* ni);

// Return the item description corresponding to the mouse click 'click'. The
// item must be on an actively unrolled section, and the click must be in the
// area of a valid item. If 'ni' is not NULL, and the selected item has a
// shortcut, 'ni' will be filled in with the shortcut.
API const char* ncmenu_mouse_selected(const struct ncmenu* n,
                                      const struct ncinput* click,
                                      struct ncinput* ni);

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

// Progress bars. They proceed linearly in any of four directions. The entirety
// of the plane will be used -- any border should be provided by the caller on
// another plane. The plane will not be erased; text preloaded into the plane
// will be consumed by the progress indicator. The bar is redrawn for each
// provided progress report (a double between 0 and 1), and can regress with
// lower values. The procession will take place along the longer dimension (at
// the time of each redraw), with the horizontal length scaled by 2 for
// purposes of comparison. I.e. for a plane of 20 rows and 50 columns, the
// progress will be to the right (50 > 40) or left with OPTION_RETROGRADE.

#define NCPROGBAR_OPTION_RETROGRADE        0x0001u // proceed left/down

typedef struct ncprogbar_options {
  uint32_t ulchannel; // upper-left channel. in the context of a progress bar,
  uint32_t urchannel; // "up" is the direction we are progressing towards, and
  uint32_t blchannel; // "bottom" is the direction of origin. for monochromatic
  uint32_t brchannel; // bar, all four channels ought be the same.
  uint64_t flags;
} ncprogbar_options;

// Takes ownership of the ncplane 'n', which will be destroyed by
// ncprogbar_destroy(). The progress bar is initially at 0%.
API ALLOC struct ncprogbar* ncprogbar_create(struct ncplane* n, const ncprogbar_options* opts)
  __attribute__ ((nonnull (1)));

// Return a reference to the ncprogbar's underlying ncplane.
API struct ncplane* ncprogbar_plane(struct ncprogbar* n)
  __attribute__ ((nonnull (1)));

// Set the progress bar's completion, a double 0 <= 'p' <= 1.
API int ncprogbar_set_progress(struct ncprogbar* n, double p)
  __attribute__ ((nonnull (1)));

// Get the progress bar's completion, a double on [0, 1].
API double ncprogbar_progress(const struct ncprogbar* n)
  __attribute__ ((nonnull (1)));

// Destroy the progress bar and its underlying ncplane.
API void ncprogbar_destroy(struct ncprogbar* n);

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
#define NCPLOT_OPTION_LABELTICKSD   0x0001u // show labels for dependent axis
#define NCPLOT_OPTION_EXPONENTIALD  0x0002u // exponential dependent axis
#define NCPLOT_OPTION_VERTICALI     0x0004u // independent axis is vertical
#define NCPLOT_OPTION_NODEGRADE     0x0008u // fail rather than degrade blitter
#define NCPLOT_OPTION_DETECTMAXONLY 0x0010u // use domain detection only for max
#define NCPLOT_OPTION_PRINTSAMPLE   0x0020u // print the most recent sample

typedef struct ncplot_options {
  // channels for the maximum and minimum levels. linear or exponential
  // interpolation will be applied across the domain between these two.
  uint64_t maxchannels;
  uint64_t minchannels;
  // styling used for the legend, if NCPLOT_OPTION_LABELTICKSD is set
  uint16_t legendstyle;
  // if you don't care, pass NCBLIT_DEFAULT and get NCBLIT_8x1 (assuming
  // UTF8) or NCBLIT_1x1 (in an ASCII environment)
  ncblitter_e gridtype; // number of "pixels" per row x column
  // independent variable can either be a contiguous range, or a finite set
  // of keys. for a time range, say the previous hour sampled with second
  // resolution, the independent variable would be the range [0..3600): 3600.
  // if rangex is 0, it is dynamically set to the number of columns.
  int rangex;
  const char* title;   // optional, printed by the labels
  uint64_t flags;      // bitfield over NCPLOT_OPTION_*
} ncplot_options;

// Use the provided plane 'n' for plotting according to the options 'opts'. The
// plot will make free use of the entirety of the plane. For domain
// autodiscovery, set miny == maxy == 0. ncuplot holds uint64_ts, while
// ncdplot holds doubles.
API ALLOC struct ncuplot* ncuplot_create(struct ncplane* n, const ncplot_options* opts,
                                         uint64_t miny, uint64_t maxy)
  __attribute__ ((nonnull (1)));

API ALLOC struct ncdplot* ncdplot_create(struct ncplane* n, const ncplot_options* opts,
                                         double miny, double maxy)
  __attribute__ ((nonnull (1)));

// Return a reference to the ncplot's underlying ncplane.
API struct ncplane* ncuplot_plane(struct ncuplot* n)
  __attribute__ ((nonnull (1)));

API struct ncplane* ncdplot_plane(struct ncdplot* n)
  __attribute__ ((nonnull (1)));

// Add to or set the value corresponding to this x. If x is beyond the current
// x window, the x window is advanced to include x, and values passing beyond
// the window are lost. The first call will place the initial window. The plot
// will be redrawn, but notcurses_render() is not called.
API int ncuplot_add_sample(struct ncuplot* n, uint64_t x, uint64_t y)
  __attribute__ ((nonnull (1)));
API int ncdplot_add_sample(struct ncdplot* n, uint64_t x, double y)
  __attribute__ ((nonnull (1)));
API int ncuplot_set_sample(struct ncuplot* n, uint64_t x, uint64_t y)
  __attribute__ ((nonnull (1)));
API int ncdplot_set_sample(struct ncdplot* n, uint64_t x, double y)
  __attribute__ ((nonnull (1)));

API int ncuplot_sample(const struct ncuplot* n, uint64_t x, uint64_t* y)
  __attribute__ ((nonnull (1)));
API int ncdplot_sample(const struct ncdplot* n, uint64_t x, double* y)
  __attribute__ ((nonnull (1)));

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
API ALLOC struct ncfdplane* ncfdplane_create(struct ncplane* n, const ncfdplane_options* opts,
                                             int fd, ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn)
  __attribute__ ((nonnull (1)));

API struct ncplane* ncfdplane_plane(struct ncfdplane* n)
  __attribute__ ((nonnull (1)));

API int ncfdplane_destroy(struct ncfdplane* n);

typedef struct ncsubproc_options {
  void* curry;
  uint64_t restart_period; // restart this many seconds after an exit (watch)
  uint64_t flags;          // bitfield over NCOPTION_SUBPROC_*
} ncsubproc_options;

// see exec(2). p-types use $PATH. e-type passes environment vars.
API ALLOC struct ncsubproc* ncsubproc_createv(struct ncplane* n, const ncsubproc_options* opts,
                                              const char* bin,  char* const arg[],
                                              ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn)
  __attribute__ ((nonnull (1)));

API ALLOC struct ncsubproc* ncsubproc_createvp(struct ncplane* n, const ncsubproc_options* opts,
                                               const char* bin,  char* const arg[],
                                               ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn)
  __attribute__ ((nonnull (1)));

API ALLOC struct ncsubproc* ncsubproc_createvpe(struct ncplane* n, const ncsubproc_options* opts,
                                                const char* bin,  char* const arg[], char* const env[],
                                                ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn)
  __attribute__ ((nonnull (1)));

API struct ncplane* ncsubproc_plane(struct ncsubproc* n)
  __attribute__ ((nonnull (1)));

API int ncsubproc_destroy(struct ncsubproc* n);

// Draw a QR code at the current position on the plane. If there is insufficient
// room to draw the code here, or there is any other error, non-zero will be
// returned. Otherwise, the QR code "version" (size) is returned. The QR code
// is (version * 4 + 17) columns wide, and ⌈version * 4 + 17⌉ rows tall (the
// properly-scaled values are written back to '*ymax' and '*xmax').
API int ncplane_qrcode(struct ncplane* n, ncblitter_e blitter, int* ymax,
                       int* xmax, const void* data, size_t len);

// Enable horizontal scrolling. Virtual lines can then grow arbitrarily long.
#define NCREADER_OPTION_HORSCROLL 0x0001ull
// Enable vertical scrolling. You can then use arbitrarily many virtual lines.
#define NCREADER_OPTION_VERSCROLL 0x0002ull
// Disable all editing shortcuts. By default, emacs-style keys are available.
#define NCREADER_OPTION_NOCMDKEYS 0x0004ull
// Make the terminal cursor visible across the lifetime of the ncreader, and
// have the ncreader manage the cursor's placement.
#define NCREADER_OPTION_CURSOR    0x0008ull

typedef struct ncreader_options {
  uint64_t tchannels; // channels used for input
  uint32_t tattrword; // attributes used for input
  uint64_t flags;     // bitfield of NCREADER_OPTION_*
} ncreader_options;

// ncreaders provide freeform input in a (possibly multiline) region, supporting
// optional readline keybindings. takes ownership of 'n', destroying it on any
// error (ncreader_destroy() otherwise destroys the ncplane).
API ALLOC struct ncreader* ncreader_create(struct ncplane* n, const ncreader_options* opts)
  __attribute__ ((nonnull (1)));

// empty the ncreader of any user input, and home the cursor.
API int ncreader_clear(struct ncreader* n)
  __attribute__ ((nonnull (1)));

API struct ncplane* ncreader_plane(struct ncreader* n)
  __attribute__ ((nonnull (1)));

// Offer the input to the ncreader. If it's relevant, this function returns
// true, and the input ought not be processed further. Almost all inputs
// are relevant to an ncreader, save synthesized ones.
API bool ncreader_offer_input(struct ncreader* n, const struct ncinput* ni)
  __attribute__ ((nonnull (1, 2)));

// Atttempt to move in the specified direction. Returns 0 if a move was
// successfully executed, -1 otherwise. Scrolling is taken into account.
API int ncreader_move_left(struct ncreader* n)
  __attribute__ ((nonnull (1)));
API int ncreader_move_right(struct ncreader* n)
  __attribute__ ((nonnull (1)));
API int ncreader_move_up(struct ncreader* n)
  __attribute__ ((nonnull (1)));
API int ncreader_move_down(struct ncreader* n)
  __attribute__ ((nonnull (1)));

// Destructively write the provided EGC to the current cursor location. Move
// the cursor as necessary, scrolling if applicable.
API int ncreader_write_egc(struct ncreader* n, const char* egc)
  __attribute__ ((nonnull (1, 2)));

// return a heap-allocated copy of the current (UTF-8) contents.
API char* ncreader_contents(const struct ncreader* n)
  __attribute__ ((nonnull (1)));

// destroy the reader and its bound plane. if 'contents' is not NULL, the
// UTF-8 input will be heap-duplicated and written to 'contents'.
API void ncreader_destroy(struct ncreader* n, char** contents);

// Dump selected Notcurses state to the supplied 'debugfp'. Output is freeform,
// and subject to change. It includes geometry of all planes, from all piles.
API void notcurses_debug(struct notcurses* nc, FILE* debugfp)
  __attribute__ ((nonnull (1, 2)));

// a system for rendering RGBA pixels as text glyphs
struct blitset {
  ncblitter_e geom;
  int width;
  int height;
  // the EGCs which form the various levels of a given plotset. if the geometry
  // is wide, things are arranged with the rightmost side increasing most
  // quickly, i.e. it can be indexed as height arrays of 1 + height glyphs. i.e.
  // the first five braille EGCs are all 0 on the left, [0..4] on the right.
  const wchar_t* egcs;
  int (*blit)(struct ncplane* n, int placey, int placex, int linesize,
              const void* data, int begy, int begx, int leny, int lenx,
              bool blendcolors);
  const char* name;
  bool fill;
};

API extern const struct blitset notcurses_blitters[];

// replaced by ncvisual_media_defblitter(). this original version never returns
// NCBLIT_3x2.
static inline ncblitter_e
ncvisual_default_blitter(bool utf8, ncscale_e scale)
  __attribute__ ((deprecated));

static inline ncblitter_e
ncvisual_default_blitter(bool utf8, ncscale_e scale){
  if(utf8){
    // NCBLIT_3x2/NCBLIT_2x2 are better image quality, especially for large
    // images, but it's not the general default because it doesn't preserve
    // aspect ratio (as does NCBLIT_2x1). NCSCALE_STRETCH throws away aspect
    // ratio, and can safely use NCBLIT_3x2/2x2.
    if(scale == NCSCALE_STRETCH){
      return NCBLIT_3x2;
    }
    return NCBLIT_2x1;
  }
  return NCBLIT_1x1;
}

// Get the default *media* (not plot) blitter for this environment when using
// the specified scaling method. Currently, this means:
//  - if lacking UTF-8, NCBLIT_1x1
//  - otherwise, if not NCSCALE_STRETCH, NCBLIT_2x1
//  - otherwise, if sextants are not known to be good, NCBLIT_2x2
//  - otherwise NCBLIT_3x2
// NCBLIT_2x2 and NCBLIT_3x2 both distort the original aspect ratio, thus
// NCBLIT_2x1 is used outside of NCSCALE_STRETCH.
API ncblitter_e ncvisual_media_defblitter(const struct notcurses* nc, ncscale_e scale);

#undef API

#ifdef __cplusplus
} // extern "C"
#endif

#endif
