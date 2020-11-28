#[allow(unused_imports)] // for docblocks
use crate::NcPlane;

// NcCell
/// A coordinate on an [`NcPlane`] storing 128 bits of data
///
/// ```txt
/// NcCell: 128 bits structure comprised of the following 5 elements:
///
/// GCLUSTER GCLUSTER GCLUSTER GCLUSTER  1. NcChar
/// 00000000 ~~~~~~~~ 11111111 11111111  2. NcCharBackstop + 3. reserved + 4. NcStyleMask
/// ~~AA~~~~ RRRRRRRR GGGGGGGG BBBBBBBB  5. NcChannels
/// ~~AA~~~~ RRRRRRRR GGGGGGGG BBBBBBBB  |
///
/// 1. (32b) Extended Grapheme Cluster, presented either as:
///
///     1.1. An NcChar of up to 4 bytes:
///     UUUUUUUU UUUUUUUU UUUUUUUU UUUUUUUU
///
///     1.2. A `0x01` in the first byte, plus 3 bytes with a 24b address to an egcpool:
///     00000001 IIIIIIII IIIIIIII IIIIIIII
///
/// 2. (8b) Backstop (zero)
/// 00000000
///
/// 3. (8b) reserved (ought to be zero)
/// ~~~~~~~~
///
/// 4. (16b) NcStyleMask
/// 11111111 11111111
///
/// 5. (64b) NcChannels
/// ~~AA~~~~ RRRRRRRR GGGGGGGG BBBBBBBB|~~AA~~~~ RRRRRRRR GGGGGGGG BBBBBBBB
/// ```
///
/// An NcCell corresponds to a single character cell on some plane, which can be
/// occupied by a single grapheme cluster (some root spacing glyph, along with
/// possible combining characters, which might span multiple columns). At any
/// cell, we can have a theoretically arbitrarily long UTF-8 string, a
/// foreground color, a background color, and an attribute set.
///
/// Valid grapheme cluster contents include:
///
/// - A NUL terminator,
/// - A single control character, followed by a NUL terminator,
/// - At most one spacing character, followed by zero or more nonspacing
///   characters, followed by a NUL terminator.
///
/// Multi-column characters can only have a single style/color throughout.
/// wcwidth() is not reliable. It's just quoting whether or not the NcChar
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
/// the topmost non-zero grapheme.
///
/// - If its alpha is 00 (CELL_ALPHA_OPAQUE) its foreground color is used unchanged.
///
/// - If its alpha is 10 (CELL_ALPHA_TRANSPARENT) its foreground color is derived
///   entirely from cells underneath it.
///
/// - Otherwise, the result will be a composite (CELL_ALPHA_BLEND).
///
/// Likewise for the background. If the bottom of a coordinate's zbuffer is
/// reached with a cumulative alpha of zero, the default is used. In this way,
/// a terminal configured with transparent background can be supported through
/// multiple occluding ncplanes.
///
/// A foreground alpha of 11 (CELL_ALPHA_HIGHCONTRAST) requests high-contrast
/// text (relative to the computed background).
/// A background alpha of 11 is currently forbidden.
///
/// Default color takes precedence over palette or RGB, and cannot be used with
/// transparency. Indexed palette takes precedence over RGB. It cannot
/// meaningfully set transparency, but it can be mixed into a cascading color.
/// RGB is used if neither default terminal colors nor palette indexing are in
/// play, and fully supports all transparency options.
///
/// `type in C: cell (struct)`
///
pub type NcCell = crate::bindings::bindgen::cell;

///
pub const NCCELL_ALPHA_BLEND: u32 = crate::bindings::bindgen::CELL_ALPHA_BLEND;

/// Background cannot be highcontrast, only foreground
pub const NCCELL_ALPHA_HIGHCONTRAST: u32 = crate::bindings::bindgen::CELL_ALPHA_HIGHCONTRAST;

///
pub const NCCELL_ALPHA_OPAQUE: u32 = crate::bindings::bindgen::CELL_ALPHA_OPAQUE;

///
pub const NCCELL_ALPHA_TRANSPARENT: u32 = crate::bindings::bindgen::CELL_ALPHA_TRANSPARENT;

/// If this bit is set, we are *not* using the default background color
pub const NCCELL_BGDEFAULT_MASK: u32 = crate::bindings::bindgen::CELL_BGDEFAULT_MASK;

/// Extract these bits to get the background alpha mask
pub const NCCELL_BG_ALPHA_MASK: u32 = crate::bindings::bindgen::CELL_BG_ALPHA_MASK;

/// If this bit *and* [`CELL_BGDEFAULT_MASK`] are set, we're using a
/// palette-indexed background color
pub const NCCELL_BG_PALETTE: u32 = crate::bindings::bindgen::CELL_BG_PALETTE;

/// Extract these bits to get the background RGB value
pub const NCCELL_BG_RGB_MASK: u32 = crate::bindings::bindgen::CELL_BG_RGB_MASK;

/// If this bit is set, we are *not* using the default foreground color
pub const NCCELL_FGDEFAULT_MASK: u64 = crate::bindings::bindgen::CELL_FGDEFAULT_MASK;

/// Extract these bits to get the foreground alpha mask
pub const NCCELL_FG_ALPHA_MASK: u64 = crate::bindings::bindgen::CELL_FG_ALPHA_MASK;

/// If this bit *and* [`CELL_BGDEFAULT_MASK`] are set, we're using a
/// palette-indexed background color
pub const NCCELL_FG_PALETTE: u64 = crate::bindings::bindgen::CELL_FG_PALETTE;

/// Extract these bits to get the foreground RGB value
pub const NCCELL_FG_RGB_MASK: u64 = crate::bindings::bindgen::CELL_FG_RGB_MASK;

///
pub const NCCELL_NOBACKGROUND_MASK: u64 = crate::bindings::bindgen::CELL_NOBACKGROUND_MASK;

/// If this bit is set, the cell is part of a multicolumn glyph.
///
/// Whether a cell is the left or right side of the glyph can be determined
/// by checking whether ->gcluster is zero.
pub const NCCELL_WIDEASIAN_MASK: u64 = crate::bindings::bindgen::CELL_WIDEASIAN_MASK as u64;

// NcChar
//
/// Extended Grapheme Cluster. A 32-bit `Char` type
///
/// - https://unicode.org/reports/tr29/#Grapheme_Cluster_Boundaries
///
/// This 32 bit char, together with the associated plane's associated egcpool,
/// completely define this cell's `NcChar`. Unless the `NcChar` requires more than
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
/// A spilled NcChar is indicated by the value `0x01iiiiii`. This cannot alias a
/// true supra-ASCII NcChar, because UTF-8 only encodes bytes <= 0x80 when they
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
/// NOTE that even if the `NcChar` is <= 4 bytes and inlined, is still interpreted as
/// a NUL-terminated char * (technically, &cell->gcluster is treated as a char*).
/// If it is more than 4 bytes, cell->gcluster has a first byte of 0x01,
/// and the remaining 24 bits are an index into the plane's egcpool,
/// which is carved into NUL-terminated chunks of arbitrary length.
///
/// `type in C: uint32_t`
///
pub type NcChar = char;

// NcCharBackStop
/// An `u8` always at zero, part of the [`NcCell`] struct
///
/// ```txt
/// 00000000
/// ```
///
/// `type in C: uint_8t`
///
pub type NcCharBackstop = u8;

// NcStyleMask
///
/// An `u16` of `NCSTYLE_*` boolean styling attributes
///
/// ```txt
/// 11111111 11111111
/// ```
///
/// `type in C:  uint16_t`
///
pub type NcStyleMask = u16;
