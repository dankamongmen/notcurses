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
/// ~~AA~~~~|RRRRRRRR|GGGGGGGG|BBBBBBBB|~~AA~~~~|RRRRRRRR|GGGGGGGG|BBBBBBBB
/// ↑↑↑↑↑↑↑↑↑↑↑↑ foreground ↑↑↑↑↑↑↑↑↑↑↑|↑↑↑↑↑↑↑↑↑↑↑↑ background ↑↑↑↑↑↑↑↑↑↑↑
///
/// type in C: channels (uint64_t) // + 8B == 16B
///
/// The hairy details: (source from include/notcurses/notcurses.h)
///
/// part of a wide glyph
/// 10000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 = 80000000 00000000
///
/// foreground is *not* "default color"
/// 01000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 = 40000000 00000000
///
/// foreground alpha (2bits)
/// 00110000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 = 30000000 00000000
///
/// foreground uses palette index
/// 00001000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 = 08000000 00000000
///
/// glyph is entirely foreground
/// 00000100 00000000 00000000 00000000 00000000 00000000 00000000 00000000 = 04000000 00000000
///
/// reserved, must be 0
/// 00000011 00000000 00000000 00000000 00000000 00000000 00000000 00000000 = 03000000 00000000
///
/// foreground in 3x8 RGB (rrggbb)
/// 00000000 11111111 11111111 11111111 00000000 00000000 00000000 00000000 = 00FFFFFF 00000000
///
/// reserved, must be 0
/// 00000000 00000000 00000000 00000000 10000000 00000000 00000000 00000000 = 00000000 80000000
///
/// background is *not* "default color"
/// 00000000 00000000 00000000 00000000 01000000 00000000 00000000 00000000 = 00000000 40000000
///
/// background alpha (2 bits)
/// 00000000 00000000 00000000 00000000 00110000 00000000 00000000 00000000 = 00000000 30000000
///
/// background uses palette index
/// 00000000 00000000 00000000 00000000 00001000 00000000 00000000 00000000 = 00000000 08000000
///
/// reserved, must be 0
/// 00000000 00000000 00000000 00000000 00000111 00000000 00000000 00000000 = 00000000 07000000
///
/// background in 3x8 RGB (rrggbb)
/// 00000000 00000000 00000000 00000000 00000000 11111111 11111111 11111111 = 00000000 00FFFFFF
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
/// ncpixel has 8 bits of alpha, and no other gunk.
///
/// we map the 8 bits of alpha to 2 bits of alpha via a level function,
//
// NOTE: the order of the colors is different than channel. Why.
pub type Pixel = u32;

pub type PaletteIndex = u8;

pub type IntResult = i32;     // -1 == err

/// Attrword: 32 bits of styling, including:
///
/// - 16 "classic" NCURSES bits
/// - 16 bits for palette-indexed color
///
/// type in C:  attrword (uint32_t)
///
/// NOTE: WIP unstable
/// https://github.com/dankamongmen/notcurses/issues/884
pub type Attribute = u32;


/// GCluster: 32 bits representing
/// either a directly-encoded ASCII-1968 value of 7 bits
/// or a 25-bit index into an egcpool
///
///  type in C: gcluster (uint32_t)
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


