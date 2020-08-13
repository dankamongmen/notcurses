//! # Types in notcurses
//!
//! The types are defined and explined here
//!


/// RGB: 24 bits broken into 3x 8bpp channels.
///
/// -------- RRRRRRRR GGGGGGGG BBBBBBBB
///
/// type in C: no data type
///
pub type Rgb = u32;


/// 8 bpp channel
///
/// CCCCCCCC (1 Byte)
///
/// Used both for R/G/B color and 8 bit alpha
///
/// type in C: no data type
///
pub type Color = u8;

/// Channel: 32 bits of context-dependent info
/// containing RGB + 2 bits of alpha + crap
///
/// ~~AA~~~~ RRRRRRRR GGGGGGGG BBBBBBBB
///
/// type in C: channel (uint32_t)
///
/// It is:
/// - an RGB value
/// - plus 2 bits of alpha
/// - plus context-dependent info
///
/// The context details are documented in ChannelPair,
///
pub type Channel = u32;

/// 2 bits of (alpha + crap) which is part of a Channel
///
/// ~~AA~~~~ -------- -------- --------
///
pub type AlphaBits = u32;

/// Channels: 64 bits containing a foreground and background channel
///
/// type in C: channels (uint64_t) // + 8B == 16B
///
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
///
///
/// At render time, these 24-bit values are quantized down to terminal
/// capabilities, if necessary. There's a clear path to 10-bit support should
/// we one day need it, but keep things cagey for now. "default color" is
/// best explained by color(3NCURSES). ours is the same concept. until the
/// "not default color" bit is set, any color you load will be ignored.
///
pub type ChannelPair = u64;

/// Pixel (RGBA): 32 bits broken into RGB + 8-bit alpha
///
/// AAAAAAAA GGGGGGGG BBBBBBBB RRRRRRRR
///
/// type in C: ncpixel (uint32_t)
///
/// ncpixel has 8 bits of alpha,  more or less linear, contributing
/// directly to the usual alpha blending equation
///
/// we map the 8 bits of alpha to 2 bits of alpha via a level function:
/// https://nick-black.com/dankwiki/index.php?title=Notcurses#Transparency.2FContrasting
//
// NOTE: the order of the colors is different than channel. Why.
pub type Pixel = u32;

/// Attrword: 32 bits of styling, including:
///
/// type in C:  attrword (uint32_t)
///
/// -  8 bits of zero
/// -  8 bits reserved
/// - 16 bits NCSTYLE_* boolean attributes
///
/// 00000000 ~~~~~~~~ FFFFFFFF FFFFFFFF
///   zero   reserved      NCSTYLE
///
pub type Attribute = u32;

/// GCluster: 32 bits representing:
///
/// 1. a directly-encoded ASCII-1968 value of 7 bits (values 0--0x7f)
///    (A single-byte single-character grapheme cluster)
///
/// 2. or a 25-bit index into an egcpool, which may be up to 32MB.
///    (An offset into a per-ncplane attached pool of varying-length UTF-8
///    grapheme clusters).
///
/// type in C: gcluster (uint32_t)
///
/// NOTE: WIP unstable
/// https://github.com/dankamongmen/notcurses/issues/830
pub type GraphemeCluster = u32;

// Cell: 128 bits tying together a:
//
// - GCluster (32b)
// - Attrword (32b)
// - Channels (64b)
//
// type in C: cell (struct)

// Plane: fundamental drawing surface. unites a:
//
// - CellMatrix
// - EGCPool
//
// type in C: ncplane (struct)

// EGCPool: contiguous region chopped up into NUL-terminated UTF8 EGCs, one per plane
//
// type in C: egcpool (struct)
//
// NOTE: need more info


// CellMatrix: rectilinear array of Cells
// one -- fb per plane, and transients show up ?
//
// NOTE: need more info


pub type PaletteIndex = u8;

pub type IntResult = i32;     // <0 == err (usually -1)

