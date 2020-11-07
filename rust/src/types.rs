//! Idiomatic Rust Type Aliases
//!
//! These types wrap the ones used in the C library, including structs,
//! constants, and primitives when used as parameters or return values.
//!
//! This is in order to enforce type checking when possible and also to follow
//! the [Rust API Guidelines](https://rust-lang.github.io/api-guidelines/naming.html)
//!

// Rgb
//
/// 24 bits broken into 3x 8bpp channels.
///
/// ```txt
/// -------- RRRRRRRR GGGGGGGG BBBBBBBB
/// ```
///
/// `type in C: no data type`
///
pub type Rgb = u32;

// Color
//
/// 8 bpp channel
///
/// ```txt
/// CCCCCCCC (1 Byte)
/// ```
///
/// Used both for R/G/B color and 8 bit alpha
///
/// `type in C: no data type`
///
pub type Color = u8;

// Channel
//
/// 32 bits of context-dependent info
/// containing RGB + 2 bits of alpha + crap
///
/// ```txt
/// ~~AA~~~~ RRRRRRRR GGGGGGGG BBBBBBBB
/// ```
///
/// It is:
/// - an RGB value
/// - plus 2 bits of alpha
/// - plus context-dependent info
///
/// The context details are documented in [`Channels`](type.Channels.html)
///
/// `type in C: channel (uint32_t)`
///
pub type Channel = u32;

// AlphaBits
//
/// 2 bits of alpha (surrounded by context dependent bits).
/// It is part of a Channel.
///
/// ```txt
/// ~~AA~~~~ -------- -------- --------
/// ```
///
/// `type in C: no data type`
///
pub type AlphaBits = u32;

// Channels
//
/// 64 bits containing a foreground and background channel
///
/// ```txt
/// ~~AA~~~~|RRRRRRRR|GGGGGGGG|BBBBBBBB|~~AA~~~~|RRRRRRRR|GGGGGGGG|BBBBBBBB
/// ↑↑↑↑↑↑↑↑↑↑↑↑ foreground ↑↑↑↑↑↑↑↑↑↑↑|↑↑↑↑↑↑↑↑↑↑↑↑ background ↑↑↑↑↑↑↑↑↑↑↑
///
/// Detailed info (specially on the context-dependent bits on the 4th byte;
///
///                             ~foreground channel~
/// part of a wide glyph:                                ↓bits view↓               ↓hex view↓
/// 1······· ········ ········ ········ ········ ········ ········ ········  =  8······· ········
///
/// foreground is *not* "default color":
/// ·1······ ········ ········ ········ ········ ········ ········ ········  =  4······· ········
///
/// foreground alpha (2bits):
/// ··11···· ········ ········ ········ ········ ········ ········ ········  =  3······· ········
///
/// foreground uses palette index:
/// ····1··· ········ ········ ········ ········ ········ ········ ········  =  ·8······ ········
///
/// glyph is entirely foreground:
/// ·····1·· ········ ········ ········ ········ ········ ········ ········  =  ·4······ ········
///
/// reserved, must be 0:
/// ······00 ········ ········ ········ ········ ········ ········ ········  =  ·3······ ········
///
/// foreground in 3x8 RGB (rrggbb):
/// ········ 11111111 11111111 11111111 ········ ········ ········ ········  =  ··FFFFFF ········
///
///                             ~background channel~
/// reserved, must be 0:                                 ↓bits view↓               ↓hex view↓
/// ········ ········ ········ ········ 0······· ········ ········ ········  =  ········ 8·······
///
/// background is *not* "default color":
/// ········ ········ ········ ········ ·1······ ········ ········ ········  =  ········ 4·······
///
/// background alpha (2 bits):
/// ········ ········ ········ ········ ··11···· ········ ········ ········  =  ········ 3·······
///
/// background uses palette index:
/// ········ ········ ········ ········ ····1··· ········ ········ ········  =  ········ ·8······
///
/// reserved, must be 0:
/// ········ ········ ········ ········ ·····000 ········ ········ ········  =  ········ ·7······
///
/// background in 3x8 RGB (rrggbb):
/// 0········ ········ ········ ········ ········11111111 11111111 11111111  =  ········ ··FFFFFF
/// ```
///
/// At render time, these 24-bit values are quantized down to terminal
/// capabilities, if necessary. There's a clear path to 10-bit support should
/// we one day need it, but keep things cagey for now. "default color" is
/// best explained by color(3NCURSES). ours is the same concept. until the
/// "not default color" bit is set, any color you load will be ignored.
///
/// `type in C: channels (uint64_t)`
///
pub type Channels = u64;

// Pixel (RGBA)
/// 32 bits broken into RGB + 8-bit alpha
///
/// ```txt
/// AAAAAAAA GGGGGGGG BBBBBBBB RRRRRRRR
/// ```
///
/// ncpixel has 8 bits of alpha,  more or less linear, contributing
/// directly to the usual alpha blending equation
///
/// we map the 8 bits of alpha to 2 bits of alpha via a level function:
/// https://nick-black.com/dankwiki/index.php?title=Notcurses#Transparency.2FContrasting
///
/// `type in C: ncpixel (uint32_t)`
///
// NOTE: the order of the colors is different than channel. Why.
pub type Pixel = u32;

// Cell
/// A coordinate on an [`NcPlane`](type.NcPlane.html)
///
/// ```txt
/// Cell: 128 bits tying together a:
///
/// 1. GCluster, 32b, either or:
/// UUUUUUUU UUUUUUUU UUUUUUUU UUUUUUUU
/// 00000001 IIIIIIII IIIIIIII IIIIIIII
///
/// 2. GCluster backstop, 8b, (zero)
/// 00000000
///
/// 3. reserved, 8b (ought to be zero)
/// ~~~~~~~~
///
/// 4. Stylemask, 16b
/// 11111111 11111111
///
/// 5. Channels (64b)
/// ~~AA~~~~|RRRRRRRR|GGGGGGGG|BBBBBBBB|~~AA~~~~|RRRRRRRR|GGGGGGGG|BBBBBBBB
/// ```
///
/// A Cell corresponds to a single character cell on some plane, which can be occupied by a single grapheme cluster (some root spacing glyph, along with possible combining characters, which might span multiple columns). At any cell, we can have a theoretically arbitrarily long UTF-8 string, a foreground color, a background color, and an attribute set. Valid grapheme cluster contents include:
///
/// - A NUL terminator,
/// - A single control character, followed by a NUL terminator,
/// - At most one spacing character, followed by zero or more nonspacing
///   characters, followed by a NUL terminator.
///
/// Multi-column characters can only have a single style/color throughout.
/// wcwidth() is not reliable. It's just quoting whether or not the Egc
/// contains a "Wide Asian" double-width character.
/// This is set for some things, like most emoji, and not set for
/// other things, like cuneiform.
///
/// Each cell occupies 16 static bytes (128 bits). The surface is thus ~1.6MB
/// for a (pretty large) 500x200 terminal. At 80x43, it's less than 64KB.
/// Dynamic requirements (the egcpool) can add up to 16MB to an ncplane, but
/// such large pools are unlikely in common use.
///
/// We implement some small alpha compositing. Foreground and background both
/// have two bits of inverted alpha. The actual grapheme written to a cell is
/// the topmost non-zero grapheme. If its alpha is 00, its foreground color is
/// used unchanged. If its alpha is 10, its foreground color is derived entirely
/// from cells underneath it. Otherwise, the result will be a composite.
/// Likewise for the background. If the bottom of a coordinate's zbuffer is
/// reached with a cumulative alpha of zero, the default is used. In this way,
/// a terminal configured with transparent background can be supported through
/// multiple occluding ncplanes. A foreground alpha of 11 requests high-contrast
/// text (relative to the computed background). A background alpha of 11 is
/// currently forbidden.
///
/// Default color takes precedence over palette or RGB, and cannot be used with
/// transparency. Indexed palette takes precedence over RGB. It cannot
/// meaningfully set transparency, but it can be mixed into a cascading color.
/// RGB is used if neither default terminal colors nor palette indexing are in
/// play, and fully supports all transparency options.
///
/// `type in C: cell (struct)`
///
pub type Cell = crate::cell;

// Egc
//
/// 32-bit Char, Extended Grapheme Cluster
///
/// This 32 bit char, together with the associated plane's associated egcpool,
/// completely define this cell's `Egc`. Unless the `Egc` requires more than
/// four bytes to encode as UTF-8, it will be inlined here:
///
/// ```txt
/// UUUUUUUU UUUUUUUU UUUUUUUU UUUUUUUU
/// extended grapheme cluster <= 4bytes
/// ```
///
/// If more than four bytes are required, it will be spilled into the egcpool.
/// In either case, there's a NUL-terminated string available without copying,
/// because (1) the egcpool is all NUL-terminated sequences and (2) the fifth
/// byte of this struct (the GClusterBackStop field, see below) is
/// guaranteed to be zero, as are any unused bytes in gcluster.
///
/// A spilled Egc is indicated by the value `0x01iiiiii`. This cannot alias a
/// true supra-ASCII Egc, because UTF-8 only encodes bytes <= 0x80 when they
/// are single-byte ASCII-derived values. The `iiiiii` is interpreted as a 24-bit
/// index into the egcpool (which may thus be up to 16MB):
///
/// ```txt
/// 00000001 iiiiiiii iiiiiiii iiiiiiii
///   sign     24bit index to egcpool
/// ```
///
/// The cost of this scheme is that the character 0x01 (SOH) cannot be encoded
/// in a cell, and therefore it must not be allowed through the API.
///
/// -----
/// NOTE that even if the `Egc` is <= 4 bytes and inlined, is still interpreted
/// a NUL-terminated char * (technically, &cell->gcluster is treated as a char*).
/// If it is more than 4 bytes, cell->gcluster has a first byte of 0x01,
/// and the remaining 24 bits are an index into the plane's egcpool,
/// which is carved into NUL-terminated chunks of arbitrary length.
///
/// `type in C: gcluster (uint32_t)`
///
// WIP towards a safe abstraction for Cell & functions receiving
pub type Egc = char;
// pub type EgcPool<'a> = &'a[u8];

pub type CellGcluster = u32; // the type cell.gcluster expects the EGB to be

/// Egc BackStop
///
/// `type in C: cell.gcluster_backstop`
///
pub type EgcBackstop = u8;

// StyleMask
///
/// 16 bits `NCSTYLE_*` of boolean styling attributes:
///
/// ```txt
/// 11111111 11111111
/// ```
///
/// `type in C:  stylemask (uint16_t)`
///
pub type StyleMask = u16;

// NcPlane
/// Fundamental drawing surface.
///
/// Unites a:
///
/// - CellMatrix
/// - EgcPool
///
/// `type in C: ncplane (struct)`
pub type NcPlane = crate::ncplane;

/// I/O wrapper to dump file descriptor to [`NcPlane`](type.NcPlane.html)
pub type NcFdPlane = crate::ncfdplane;

/// Options struct for [`NcFdPlane`](type.NcFdPlane.html)
///
/// `type in C: ncplane_options (struct)`
pub type NcFdPlaneOptions = crate::ncfdplane_options;

// EGCPool:
//
// Contiguous region chopped up into NUL-terminated UTF8 EGCs, one per plane
//
// `type in C: egcpool (struct)`
// TODO

// CellMatrix: rectilinear array of Cells
// one -- fb per plane, and transients show up ?

/// Palette structure consisting of an array of 256 [`Channel`](type.Channel.html)s
///
/// Some terminals only support 256 colors, but allow the full
/// palette to be specified with arbitrary RGB colors. In all cases, it's more
/// performant to use indexed colors, since it's much less data to write to the
/// terminal. If you can limit yourself to 256 colors, that's probably best.
///
/// `type in C: ncpalette256 (struct)`
pub type Palette = crate::palette256;

/// 8-bit value used for indexing into a palette
///
pub type PaletteIndex = u8;

/// 32-bit signed value used to return errors, when value < 0, (usually -1)
///
pub type IntResult = i32;

/// the [`Egc`](type.Egc.html)s which form the various levels
/// of a given geometry.
///
/// If the geometry is wide, things are arranged with the rightmost side
/// increasing most quickly, i.e. it can be indexed as height arrays of
/// 1 + height glyphs.
/// i.e. The first five braille EGCs are all 0 on the left,
/// [0..4] on the right.
///
/// `type in C: blitset (struct)`
pub type BlitSet = crate::blitset;

/// Alignment within a plane or terminal.
/// Left/right-justified, or centered.
pub type NcAlign = crate::ncalign_e;
/// Align an NcPlane or NcTerm
pub const NCALIGN_LEFT: NcAlign = crate::ncalign_e_NCALIGN_LEFT;
///
pub const NCALIGN_RIGHT: NcAlign = crate::ncalign_e_NCALIGN_RIGHT;
///
pub const NCALIGN_CENTER: NcAlign = crate::ncalign_e_NCALIGN_CENTER;
///
pub const NCALIGN_UNALIGNED: NcAlign = crate::ncalign_e_NCALIGN_UNALIGNED;

/// Blitter Mode (`NCBLIT_*`)
///
/// We never blit full blocks, but instead spaces (more efficient) with the
/// background set to the desired foreground.
pub type NcBlitter = crate::ncblitter_e;
/// space, compatible with ASCII
pub const NCBLIT_1x1: NcBlitter = crate::ncblitter_e_NCBLIT_1x1;
/// halves + 1x1 (space)
/// ▄▀
pub const NCBLIT_2x1: NcBlitter = crate::ncblitter_e_NCBLIT_2x1;
/// quadrants + 2x1
/// ▗▐ ▖▀▟▌▙
pub const NCBLIT_2x2: NcBlitter = crate::ncblitter_e_NCBLIT_2x2;
/// sextants (NOT 2x2)
/// 🬀🬁🬂🬃🬄🬅🬆🬇🬈🬉🬊🬋🬌🬍🬎🬏🬐🬑🬒🬓🬔🬕🬖🬗🬘🬙🬚🬛🬜🬝🬞🬟🬠🬡🬢🬣🬤🬥🬦🬧🬨🬩🬪🬫🬬🬭🬮🬯🬰🬱🬲🬳🬴🬵🬶🬷🬸🬹🬺🬻
pub const NCBLIT_3x2: NcBlitter = crate::ncblitter_e_NCBLIT_3x2;
/// four vertical levels
/// █▆▄▂
pub const NCBLIT_4x1: NcBlitter = crate::ncblitter_e_NCBLIT_4x1;
/// eight vertical levels
/// █▇▆▅▄▃▂▁
pub const NCBLIT_8x1: NcBlitter = crate::ncblitter_e_NCBLIT_8x1;
/// 4 rows, 2 cols (braille)
/// ⡀⡄⡆⡇⢀⣀⣄⣆⣇⢠⣠⣤⣦⣧⢰⣰⣴⣶⣷⢸⣸⣼⣾⣿
pub const NCBLIT_BRAILLE: NcBlitter = crate::ncblitter_e_NCBLIT_BRAILLE;
/// the blitter is automatically chosen
pub const NCBLIT_DEFAULT: NcBlitter = crate::ncblitter_e_NCBLIT_DEFAULT;
/// 6 rows, 1 col (RGB), spotty support among terminals
pub const NCBLIT_SIXEL: NcBlitter = crate::ncblitter_e_NCBLIT_SIXEL;

/// Log level for [`NotcursesOptions`](type.NotcursesOptions.html)
pub type NcLogLevel = crate::ncloglevel_e;
///
pub const NCLOGLEVEL_DEBUG: NcLogLevel = crate::ncloglevel_e_NCLOGLEVEL_DEBUG;
pub const NCLOGLEVEL_ERROR: NcLogLevel = crate::ncloglevel_e_NCLOGLEVEL_ERROR;
pub const NCLOGLEVEL_FATAL: NcLogLevel = crate::ncloglevel_e_NCLOGLEVEL_FATAL;
pub const NCLOGLEVEL_INFO: NcLogLevel = crate::ncloglevel_e_NCLOGLEVEL_INFO;
pub const NCLOGLEVEL_PANIC: NcLogLevel = crate::ncloglevel_e_NCLOGLEVEL_PANIC;
pub const NCLOGLEVEL_SILENT: NcLogLevel = crate::ncloglevel_e_NCLOGLEVEL_SILENT;
pub const NCLOGLEVEL_TRACE: NcLogLevel = crate::ncloglevel_e_NCLOGLEVEL_TRACE;
pub const NCLOGLEVEL_VERBOSE: NcLogLevel = crate::ncloglevel_e_NCLOGLEVEL_VERBOSE;
pub const NCLOGLEVEL_WARNING: NcLogLevel = crate::ncloglevel_e_NCLOGLEVEL_WARNING;

/// How to scale an [`NcVisual`](type.NcVisual.html) during rendering
///
/// - NCSCALE_NONE will apply no scaling.
/// - NCSCALE_SCALE scales a visual to the plane's size,
///   maintaining aspect ratio.
/// - NCSCALE_STRETCH stretches and scales the image in an
///   attempt to fill the entirety of the plane.
pub type NcScale = crate::ncscale_e;
/// Maintain original size
pub const NCSCALE_NONE: NcScale = crate::ncscale_e_NCSCALE_NONE;
/// Maintain aspect ratio
pub const NCSCALE_SCALE: NcScale = crate::ncscale_e_NCSCALE_SCALE;
/// Throw away aspect ratio
pub const NCSCALE_STRETCH: NcScale = crate::ncscale_e_NCSCALE_STRETCH;

/// Reads and decodes input events
///
/// Reads from stdin and decodes the input to stdout,
/// including synthesized events and mouse events.
///
/// To exit, generate EOF (usually Ctrl+‘d’).
pub type NcInput = crate::ncinput;

/// A visual bit of multimedia opened with LibAV|OIIO
pub type NcVisual = crate::ncvisual;
/// Options struct for [`NcVisual`](type.NcVisual.html)
pub type NcVisualOptions = crate::ncvisual_options;

// Terminal --------------------------------------------------------------------

/// Minimal notcurses instances for styling text
pub type NcDirect = crate::ncdirect;

/// Flags for NcDirect
pub type NcDirectFlags = u64;
/// Flag that avoids placing the terminal into cbreak mode
/// (disabling echo and line buffering)
pub const NCDIRECT_OPTION_INHIBIT_CBREAK: NcDirectFlags =
    crate::bindings::NCDIRECT_OPTION_INHIBIT_CBREAK as NcDirectFlags;
/// Flag that avoids calling setlocale(LC_ALL, NULL)
///
/// If the result is either "C" or "POSIX", it will print a
/// diagnostic to stderr, and then call setlocale(LC_ALL, "").
/// This will attempt to set the locale based off the LANG
/// environment variable. Your program should call setlocale(3)
/// itself, usually as one of the first lines.
pub const NCDIRECT_OPTION_INHIBIT_SETLOCALE: NcDirectFlags =
    crate::bindings::NCDIRECT_OPTION_INHIBIT_SETLOCALE as NcDirectFlags;

/// TUI library for modern terminal emulators
///
/// Notcurses builds atop the terminfo abstraction layer to
/// provide reasonably portable vivid character displays.
pub type Notcurses = crate::bindings::notcurses;
/// Options struct for [`Notcurses`](type.Notcurses.html)
pub type NotcursesOptions = crate::bindings::notcurses_options;

/// Context for a palette fade operation
pub type NcFadeCtx = crate::ncfadectx;


// Widgets

pub type NcMenu = crate::ncmenu;
pub type NcMenuItem = crate::ncmenu_item;
pub type NcMenuOptions = crate::ncmenu_options;
pub type NcMenuSection = crate::ncmenu_section;

pub type NcReader = crate::ncreader;
pub type NcReaderOptions = crate::ncreader_options;

pub type NcReel = crate::ncreel;
pub type NcReelOptions = crate::ncreel_options;

pub type NcPlotF64 = crate::ncdplot;
pub type NcPlotU64 = crate::ncuplot;
pub type NcPlotOptions = crate::ncplot_options;

pub type NcSelector = crate::ncselector;
pub type NcSelectorItem = crate::ncselector_item;
pub type NcSelectorOptions = crate::ncselector_options;

pub type NcStats = crate::ncstats;

pub type NcTablet = crate::nctablet;

pub type NcMultiSelector = crate::ncmultiselector;
pub type NcMultiSelectorItem = crate::ncmselector_item;
pub type NcMultiSelectorOptions = crate::ncmultiselector_options;
