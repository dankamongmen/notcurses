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
/// It is:
/// - an RGB value
/// - plus 2 bits of alpha
/// - plus context-dependent info
///
/// The context details are documented in ChannelPair,
///
/// type in C: channel (uint32_t)
///
pub type Channel = u32;

/// 2 bits of alpha (surrounded by context dependent bits).
/// It is part of a Channel.
///
/// ~~AA~~~~ -------- -------- --------
///
/// type in C: no data type
///
pub type AlphaBits = u32;

/// Channels: 64 bits containing a foreground and background channel
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
/// type in C: channels (uint64_t)
///
pub type ChannelPair = u64;

/// Pixel (RGBA): 32 bits broken into RGB + 8-bit alpha
///
/// AAAAAAAA GGGGGGGG BBBBBBBB RRRRRRRR
///
/// ncpixel has 8 bits of alpha,  more or less linear, contributing
/// directly to the usual alpha blending equation
///
/// we map the 8 bits of alpha to 2 bits of alpha via a level function:
/// https://nick-black.com/dankwiki/index.php?title=Notcurses#Transparency.2FContrasting
///
/// type in C: ncpixel (uint32_t)
///
// NOTE: the order of the colors is different than channel. Why.
pub type Pixel = u32;

// Cell: 128 bits tying together a:
//
// 1. GCluster (32b), either or:
// UUUUUUUU UUUUUUUU UUUUUUUU UUUUUUUU
// 00000001 IIIIIIII IIIIIIII IIIIIIII
//
// 2. Attrword (32b)
// 11111111 11111111 ~~~~~~~~ 00000000
//
// 3. Channels (64b)
// ~~AA~~~~|RRRRRRRR|GGGGGGGG|BBBBBBBB|~~AA~~~~|RRRRRRRR|GGGGGGGG|BBBBBBBB
//
// type in C: cell (struct)

/// GCluster
///
/// These 32 bits, together with the associated plane's associated egcpool,
/// completely define this cell's EGC. Unless the EGC requires more than four
/// bytes to encode as UTF-8, it will be inlined here:
///
/// UUUUUUUU UUUUUUUU UUUUUUUU UUUUUUUU
/// extended grapheme cluster <= 4bytes
///
/// If more than four bytes are required, it will be spilled into the egcpool.
/// In either case, there's a NUL-terminated string available without copying,
/// because (1) the egcpool is all NUL-terminated sequences and (2) the fifth
/// byte of this struct (the first byte of the attrword, see below) is
/// guaranteed to be zero, as are any unused bytes in gcluster.
///
/// A spilled EGC is indicated by the value 0x01XXXXXX. This cannot alias a
/// true supra-ASCII EGC, because UTF-8 only encodes bytes <= 0x80 when they
/// are single-byte ASCII-derived values. The XXXXXX is interpreted as a 24-bit
/// index into the egcpool (which may thus be up to 16MB):
///
/// 00000001 IIIIIIII IIIIIIII IIIIIIII
///   sign     24bit index to egpool
///
/// The cost of this scheme is that the character 0x01 (SOH) cannot be encoded
/// in a cell, and therefore it must not be allowed through the API.
///
/// type in C: gcluster (uint32_t)
///
pub type GraphemeCluster = u32;

/// Attrword: 32 bits of styling, including:
///
/// 8 bits of zero + 8 reserved bits + 16 bits NCSTYLE_* boolean attributes:
///
/// 11111111 11111111 ~~~~~~~~ 00000000  =  0xFFFF~~00
///     NCSTYLE_      reserved   zero
///
/// The values of the NCSTYLE_* bits depend on endianness at compile time:
/// we need them in the higher memory addresses, because we rely on the octet
/// adjacent to gcluster being zero, as a backstop to a 4-byte inlined UTF-8
/// value.  (attrword & 0xff000000): egc backstop, *must be zero*
///
/// type in C:  attrword (uint32_t)
///
pub type Attribute = u32;

// Plane: fundamental drawing surface. unites a:
//
// - CellMatrix
// - EGCPool
//
// type in C: ncplane (struct)

// EGCPool: contiguous region chopped up into NUL-terminated UTF8 EGCs, one per plane
//
// type in C: egcpool (struct)


// CellMatrix: rectilinear array of Cells
// one -- fb per plane, and transients show up ?

/// 8-bit value used for indexing into a palette
///
pub type PaletteIndex = u8;

/// 32-bit signed value used to return errors, when value < 0, (usually -1)
///
pub type IntResult = i32;
