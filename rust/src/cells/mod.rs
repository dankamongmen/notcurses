//! `NcCell`

// functions already exported by bindgen : 7
// -----------------------------------------
// (W) wrap: 4
// (#) test: 0
// ------------------------------------------
//…  nccell_extended_gcluster
//…  nccell_load
//   nccell_width
//W  nccells_double_box
//W  nccells_rounded_box
//W  nccell_duplicate
//W  nccell_release
//
// functions manually reimplemented: 46
// ------------------------------------------
// (X) wont:  2
// (+) done: 38
// (W) wrap: 40
// (#) test: 26
// ------------------------------------------
//W# nccell_bg_alpha
//W# nccell_bg_default_p
//W# nccell_bg_palindex
//W# nccell_bg_palindex_p
//W# nccell_bg_rgb
//W# nccell_bg_rgb8
//W+ nccell_double_wide_p
//W+ nccell_extract
//W# nccell_fchannel
//W# nccell_fg_alpha
//W# nccell_fg_default_p
//W# nccell_fg_palindex
//W# nccell_fg_palindex_p
//W# nccell_fg_rgb
//W# nccell_fg_rgb8
//W+ nccell_init
//…… nccell_load_char
//   nccell_load_egc32
//W+ nccell_off_styles
//W+ nccell_on_styles
//W+ nccell_prime
//W# nccell_set_bchannel
//W# nccell_set_bg_alpha
//W# nccell_set_bg_default
//W# nccell_set_bg_palindex
//W# nccell_set_bg_rgb
//W# nccell_set_bg_rgb8
// X nccell_set_bg_rgb8_clipped   // unneeded
//W# nccell_set_fchannel
//W# nccell_set_fg_alpha
//W# nccell_set_fg_default
//W# nccell_set_fg_palindex
//W# nccell_set_fg_rgb
//W# nccell_set_fg_rgb8
// X nccell_set_fg_rgb8_clipped   // unneeded
//W+ nccell_set_styles
//W+ nccell_strdup
//W+ nccell_styles
//W+ nccell_wide_left_p
//W+ nccell_wide_right_p
//W+ nccellcmp
//   nccells_ascii_box
//   nccells_heavy_box
//   nccells_light_box
//W+ nccells_load_box

#[cfg(test)]
mod test;

mod methods;
mod reimplemented;
pub use reimplemented::*;

// NcCell
/// A coordinate on an [`NcPlane`][crate::NcPlane] storing 128 bits of data.
///
/// # Methods & Associated Functions
///
/// - [Constructors & Destructors](#nccell-constructors-and-destructors)
///
/// - [bg|fg `NcChannel`s manipulation](#nccell-methods-bgfg-ncchannels-manipulation)
/// - [Other components](#nccell-methods-other-components)
/// - [Text](#nccell-methods-text)
///
/// # Description
///
/// An `NcCell` corresponds to a single character cell on some NcPlane`,
/// which can be occupied by a single [`NcEgc`] grapheme cluster (some root
/// spacing glyph, along with possible combining characters, which might span
/// multiple columns).
///
/// An NcCell is bounded to an NcPlane, but the cell doesn't store anything
/// about the plane.
///
/// At any `NcCell`, we can have a theoretically arbitrarily long UTF-8 string,
/// a foreground color, a background color, and an [`NcStyle`] attribute set.
///
/// Valid grapheme cluster contents include:
///
/// - A NUL terminator,
/// - A single [control character](https://en.wikipedia.org/wiki/Control_character),
///   followed by a NUL terminator,
/// - At most one [spacing
/// character](https://en.wikipedia.org/wiki/Graphic_character#Spacing_character),
///   followed by zero or more nonspacing characters, followed by a NUL terminator.
///
/// ## Diagram
///
/// ```txt
/// NcCell: 128 bits structure comprised of the following 5 elements:
///
/// GCLUSTER GCLUSTER GCLUSTER GCLUSTER  1. NcEgc
/// 00000000 ~~~~~~~~ 11111111 11111111  2. NcEgcBackstop + 3. width + 4. NcStyle
/// ~~AA~~~~ RRRRRRRR GGGGGGGG BBBBBBBB  5. NcChannelPair
/// ~~AA~~~~ RRRRRRRR GGGGGGGG BBBBBBBB  |
///
/// 1. (32b) Extended Grapheme Cluster, presented either as:
///
///     1.1. An NcEgc of up to 4 bytes:
///     UUUUUUUU UUUUUUUU UUUUUUUU UUUUUUUU
///
///     1.2. A `0x01` in the first byte, plus 3 bytes with a 24b address to an egcpool:
///     00000001 IIIIIIII IIIIIIII IIIIIIII
///
/// 2. (8b) Backstop (zero)
/// 00000000
///
/// 3. (8b) column width
/// WWWWWWWW
///
/// 4. (16b) NcStyle
/// 11111111 11111111
///
/// 5. (64b) NcChannelPair
/// ~~AA~~~~ RRRRRRRR GGGGGGGG BBBBBBBB|~~AA~~~~ RRRRRRRR GGGGGGGG BBBBBBBB
/// ```
///
/// `type in C: cell (struct)`
///
/// # More NcCell Information
///
/// ## Size
///
/// Multi-column characters can only have a single style/color throughout.
/// [`wcwidth()`](https://www.man7.org/linux/man-pages/man3/wcwidth.3.html)
/// is not reliable. It's just quoting whether or not the [`NcEgc`]
/// contains a "Wide Asian" double-width character.
/// This is set for some things, like most emoji, and not set for
/// other things, like cuneiform.
///
/// Each cell occupies 16 static bytes (128 bits). The surface is thus ~1.6MB
/// for a (pretty large) 500x200 terminal. At 80x43, it's less than 64KB.
/// Dynamic requirements (the egcpool) can add up to 16MB to an ncplane, but
/// such large pools are unlikely in common use.
///
/// ## Alpha Compositing
///
/// We implement some small alpha compositing. Foreground and background both
/// have two bits of inverted alpha. The actual grapheme written to a cell is
/// the topmost non-zero grapheme.
///
/// - If its alpha is 00 ([`NCCELL_OPAQUE`]) its foreground color is used unchanged.
///
/// - If its alpha is 10 ([`NCCELL_TRANSPARENT`]) its foreground color is derived
///   entirely from cells underneath it.
///
/// - If its alpha is 01 ([`NCCELL_BLEND`]) the result will be a composite.
///
/// Likewise for the background. If the bottom of a coordinate's zbuffer is
/// reached with a cumulative alpha of zero, the default is used. In this way,
/// a terminal configured with transparent background can be supported through
/// multiple occluding ncplanes.
///
/// A foreground alpha of 11 ([`NCCELL_HIGHCONTRAST`]) requests high-contrast
/// text (relative to the computed background).
/// A background alpha of 11 is currently forbidden.
///
/// ## Precedence
///
/// - Default color takes precedence over palette or RGB, and cannot be used with
///   transparency.
/// - Indexed palette takes precedence over RGB. It cannot meaningfully set
///   transparency, but it can be mixed into a cascading color.
/// - RGB is used if neither default terminal colors nor palette indexing are in
///   play, and fully supports all transparency options.
///
/// ## Column width *(WIP)*
///
/// See [USAGE.md](https://github.com/dankamongmen/notcurses/blob/master/USAGE.md)
///
/// We store the column width in this field. for a multicolumn EGC of N
/// columns, there will be N nccells, and each has a width of N...for now.
/// eventually, such an EGC will set more than one subsequent cell to
/// WIDE_RIGHT, and this won't be necessary. it can then be used as a
/// bytecount. see #1203. FIXME iff width >= 2, the cell is part of a
/// multicolumn glyph. whether a cell is the left or right side of the glyph
/// can be determined by checking whether ->gcluster is zero.
///
pub type NcCell = crate::bindings::ffi::cell;

#[allow(unused_imports)]
use crate::{NcAlphaBits, NcChannel, NcPlane};

/// [`NcAlphaBits`] bits indicating
/// [`NcCell`]'s foreground or background color will be a composite between
/// its color and the `NcCell`s' corresponding colors underneath it
pub const NCCELL_BLEND: u32 = crate::bindings::ffi::NCALPHA_BLEND;

/// [`NcAlphaBits`] bits indicating
/// [`NcCell`]'s foreground color will be high-contrast (relative to the
/// computed background). Background cannot be highcontrast
pub const NCCELL_HIGHCONTRAST: u32 = crate::bindings::ffi::NCALPHA_HIGHCONTRAST;

/// [`NcAlphaBits`] bits indicating
/// [`NcCell`]'s foreground or background color is used unchanged
pub const NCCELL_OPAQUE: u32 = crate::bindings::ffi::NCALPHA_OPAQUE;

/// [`NcAlphaBits`] bits indicating
/// [`NcCell`]'s foreground or background color is derived entirely from the
/// `NcCell`s underneath it
pub const NCCELL_TRANSPARENT: u32 = crate::bindings::ffi::NCALPHA_TRANSPARENT;

/// If this bit is set, we are *not* using the default background color
///
/// See the detailed diagram at [`NcChannelPair`][crate::NcChannelPair]
///
/// NOTE: This can also be used against a single [`NcChannel`]
pub const NCCELL_BGDEFAULT_MASK: u32 = crate::bindings::ffi::CELL_BGDEFAULT_MASK;

/// Extract these bits to get the background alpha mask
/// ([`NcAlphaBits`])
///
/// See the detailed diagram at [`NcChannelPair`][crate::NcChannelPair]
///
/// NOTE: This can also be used against a single [`NcChannel`]
pub const NCCELL_BG_ALPHA_MASK: u32 = crate::bindings::ffi::CELL_BG_ALPHA_MASK;

/// If this bit *and* [`NCCELL_BGDEFAULT_MASK`] are set, we're using a
/// palette-indexed background color
///
/// See the detailed diagram at [`NcChannelPair`][crate::NcChannelPair]
///
/// NOTE: This can also be used against a single [`NcChannel`]
pub const NCCELL_BG_PALETTE: u32 = crate::bindings::ffi::CELL_BG_PALETTE;

/// Extract these bits to get the background [`NcRgb`][crate::NcRgb] value
///
/// See the detailed diagram at [`NcChannelPair`][crate::NcChannelPair]
///
/// NOTE: This can also be used against a single [`NcChannel`]
pub const NCCELL_BG_RGB_MASK: u32 = crate::bindings::ffi::CELL_BG_RGB_MASK;

/// If this bit is set, we are *not* using the default foreground color
///
/// See the detailed diagram at [`NcChannelPair`][crate::NcChannelPair]
///
/// NOTE: When working with a single [`NcChannel`] use [`NCCELL_BGDEFAULT_MASK`];
pub const NCCELL_FGDEFAULT_MASK: u64 = crate::bindings::ffi::CELL_FGDEFAULT_MASK;

/// Extract these bits to get the foreground alpha mask
/// ([`NcAlphaBits`])
///
/// See the detailed diagram at [`NcChannelPair`][crate::NcChannelPair]
///
/// NOTE: When working with a single [`NcChannel`] use [`NCCELL_BG_ALPHA_MASK`];
pub const NCCELL_FG_ALPHA_MASK: u64 = crate::bindings::ffi::CELL_FG_ALPHA_MASK;

/// If this bit *and* [`NCCELL_FGDEFAULT_MASK`] are set, we're using a
/// palette-indexed background color
///
/// See the detailed diagram at [`NcChannelPair`][crate::NcChannelPair]
///
/// NOTE: When working with a single [`NcChannel`] use [`NCCELL_BG_PALETTE`];
pub const NCCELL_FG_PALETTE: u64 = crate::bindings::ffi::CELL_FG_PALETTE;

/// Extract these bits to get the foreground [`NcRgb`][crate::NcRgb] value
///
/// See the detailed diagram at [`NcChannelPair`][crate::NcChannelPair]
///
/// NOTE: When working with a single [`NcChannel`] use [`NCCELL_BG_RGB_MASK`];
pub const NCCELL_FG_RGB_MASK: u64 = crate::bindings::ffi::CELL_FG_RGB_MASK;

// NcEgc
//
/// Extended Grapheme Cluster. A 32-bit [`char`]-like type
///
/// This 32 bit char, together with the associated plane's associated egcpool,
/// completely define this cell's `NcEgc`. Unless the `NcEgc` requires more than
/// four bytes to encode as UTF-8, it will be inlined here:
///
/// ## Diagram 1
///
/// ```txt
/// UUUUUUUU UUUUUUUU UUUUUUUU UUUUUUUU
/// extended grapheme cluster <= 4bytes
/// ```
///
/// `type in C: uint32_t`
///
/// If more than four bytes are required, it will be spilled into the egcpool.
/// In either case, there's a NUL-terminated string available without copying,
/// because (1) the egcpool is all NUL-terminated sequences and (2) the fifth
/// byte of this struct (the GClusterBackStop field, see below) is
/// guaranteed to be zero, as are any unused bytes in gcluster.
///
/// A spilled `NcEgc` is indicated by the value `0x01iiiiii`. This cannot alias a
/// true supra-ASCII NcEgc, because UTF-8 only encodes bytes <= 0x80 when they
/// are single-byte ASCII-derived values. The `iiiiii` is interpreted as a 24-bit
/// index into the egcpool (which may thus be up to 16MB):
///
/// ## Diagram 2
///
/// ```txt
/// 00000001 iiiiiiii iiiiiiii iiiiiiii
///   sign     24bit index to egcpool
/// ```
/// `type in C: uint32_t`
///
/// The cost of this scheme is that the character 0x01 (`SOH`) cannot be encoded
/// in a cell, and therefore it must not be allowed through the API.
///
/// -----
/// NOTE that even if the `NcEgc` is <= 4 bytes and inlined, is still interpreted as
/// a NUL-terminated char * (technically, &cell->gcluster is treated as a char*).
/// If it is more than 4 bytes, cell->gcluster has a first byte of 0x01,
/// and the remaining 24 bits are an index into the plane's egcpool,
/// which is carved into NUL-terminated chunks of arbitrary length.
///
/// ## Links
///
/// - [Grapheme Cluster
/// Boundaries](https://unicode.org/reports/tr29/#Grapheme_Cluster_Boundaries)
///
///
// TODO: there should be two types at least:
// - an utf-8 string len 1 of type &str.
// - a unicode codepoint of type char.
pub type NcEgc = char;

// NcEgcBackStop
/// An `u8` always at zero, part of the [`NcCell`] struct
///
/// ## Diagram
///
/// ```txt
/// 00000000
/// ```
///
/// `type in C: uint_8t`
///
pub type NcEgcBackstop = u8;

// NcStyle
///
/// An `u16` of `NCSTYLE_*` boolean styling attribute flags
///
/// ## Attributes
///
/// - [`NCSTYLE_BLINK`]
/// - [`NCSTYLE_BOLD`]
/// - [`NCSTYLE_DIM`]
/// - [`NCSTYLE_INVIS`]
/// - [`NCSTYLE_ITALIC`]
/// - [`NCSTYLE_MASK`]
/// - [`NCSTYLE_NONE`]
/// - [`NCSTYLE_PROTECT`]
/// - [`NCSTYLE_REVERSE`]
/// - [`NCSTYLE_STANDOUT`]
/// - [`NCSTYLE_STRUCK`]
/// - [`NCSTYLE_UNDERLINE`]
///
///
/// ## Diagram
///
/// ```txt
/// 11111111 11111111
/// ```
///
/// `type in C:  uint16_t`
///
pub type NcStyle = u16;

///
pub const NCSTYLE_BLINK: u16 = crate::bindings::ffi::NCSTYLE_BLINK as u16;

///
pub const NCSTYLE_BOLD: u16 = crate::bindings::ffi::NCSTYLE_BOLD as u16;

///
pub const NCSTYLE_DIM: u16 = crate::bindings::ffi::NCSTYLE_DIM as u16;

///
pub const NCSTYLE_INVIS: u16 = crate::bindings::ffi::NCSTYLE_INVIS as u16;

///
pub const NCSTYLE_ITALIC: u16 = crate::bindings::ffi::NCSTYLE_ITALIC as u16;

///
pub const NCSTYLE_MASK: u16 = crate::bindings::ffi::NCSTYLE_MASK as u16;

///
pub const NCSTYLE_NONE: u16 = crate::bindings::ffi::NCSTYLE_NONE as u16;

///
pub const NCSTYLE_PROTECT: u16 = crate::bindings::ffi::NCSTYLE_PROTECT as u16;

///
pub const NCSTYLE_REVERSE: u16 = crate::bindings::ffi::NCSTYLE_REVERSE as u16;

///
pub const NCSTYLE_STANDOUT: u16 = crate::bindings::ffi::NCSTYLE_STANDOUT as u16;

///
pub const NCSTYLE_STRUCK: u16 = crate::bindings::ffi::NCSTYLE_STRUCK as u16;

///
pub const NCSTYLE_UNDERLINE: u16 = crate::bindings::ffi::NCSTYLE_UNDERLINE as u16;
