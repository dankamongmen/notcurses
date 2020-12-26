//! `NcChannel*`

// -----------------------------------------------------------------------------
// - The channel components are u8 instead of u32.
//   Because of type enforcing, some runtime checks are now unnecessary.
//
// - None of the functions can't fail now. The original checks for dirty bits
//   have been substitued by mask cleaning (bitwise and)
//
// - These functions were deemed unnecessary to implement:
//   - `channel_set_rgb_clipped()`
//   - `channels_set_fg_rgb8_clipped()`
//   - `channels_set_bg_rgb8_clipped()`
// -----------------------------------------------------------------------------
//
// functions manually reimplemented: 39
// ------------------------------------------
// (X) wont:  3
// (+) done: 36 / 0
// (#) test: 19
// (W) wrap: 13
// ------------------------------------------
//W# channel_alpha
//W# channel_b
//W# channel_default_p
//W# channel_g
// # channel_palindex_p
//W# channel_r
//W# channel_rgb8
// + channel_set
//W# channel_set_alpha
//W# channel_set_default
//W# channel_set_rgb8
// X channel_set_rgb_clipped ---
// # channels_bchannel
//W+ channels_bg_alpha
// + channels_bg_default_p
// # channels_bg_palindex_p
// + channels_bg_rgb
// + channels_bg_rgb8
// # channels_combine
// # channels_fchannel
//W+ channels_fg_alpha
// + channels_fg_default_p
// # channels_fg_palindex_p
// + channels_fg_rgb
// + channels_fg_rgb8
// # channels_set_bchannel
//W+ channels_set_bg_alpha
// + channels_set_bg_default
// # channels_set_bg_palindex
//W+ channels_set_bg_rgb
// + channels_set_bg_rgb8
// X channels_set_bg_rgb8_clipped
// # channels_set_fchannel
//W+ channels_set_fg_alpha
// + channels_set_fg_default
// # channels_set_fg_palindex
//W+ channels_set_fg_rgb
// + channels_set_fg_rgb8
// X channels_set_fg_rgb8_clipped

#[cfg(test)]
mod test;

mod methods;
mod reimplemented;
pub use methods::{NcChannelMethods, NcChannelPairMethods};
pub use reimplemented::*;

// NcChannel
//
/// 32 bits of context-dependent info
/// containing RGB + 2 bits of alpha + extra
///
/// It is:
/// - a 24-bit [`NcRgb`] value
/// - plus 8 bits divided in:
///   - 2 bits of [`NcAlphaBits`]
///   - 6 bits of context-dependent info
///
/// The context details are documented in [`NcChannelPair`]
///
/// ## Diagram
///
/// ```txt
/// ~~AA~~~~ RRRRRRRR GGGGGGGG BBBBBBBB
/// ```
/// `type in C: channel (uint32_t)`
///
pub type NcChannel = u32;

/// Extract these bits to get a channel's alpha value
pub const NCCHANNEL_ALPHA_MASK: u32 = crate::bindings::ffi::CHANNEL_ALPHA_MASK;

// NcAlphaBits
//
/// 2 bits of alpha (surrounded by context dependent bits).
/// It is part of an [`NcChannel`].
///
/// ## Diagram
///
/// ```txt
/// ~~AA~~~~ -------- -------- --------
/// ```
///
/// `type in C: no data type`
///
pub type NcAlphaBits = u32;

// NcChannelPair
//
/// 64 bits containing a foreground and background [`NcChannel`]
///
/// At render time, both 24-bit [`NcRgb`] values are quantized down to terminal
/// capabilities, if necessary. There's a clear path to 10-bit support should
/// we one day need it.
///
/// ## Default Color
///
/// The "default color" is best explained by
/// [color(3NCURSES)](https://manpages.debian.org/stretch/ncurses-doc/color.3ncurses.en.html)
/// and [default_colors(3NCURSES)](https://manpages.debian.org/stretch/ncurses-doc/default_colors.3ncurses.en.html).
/// Ours is the same concept.
///
/// **Until the "not default color" bit is set, any color you load will be ignored.**
///
/// ## Diagram
///
/// ```txt
/// ~~AA~~~~|RRRRRRRR|GGGGGGGG|BBBBBBBB|~~AA~~~~|RRRRRRRR|GGGGGGGG|BBBBBBBB
/// ↑↑↑↑↑↑↑↑↑↑↑↑ foreground ↑↑↑↑↑↑↑↑↑↑↑|↑↑↑↑↑↑↑↑↑↑↑↑ background ↑↑↑↑↑↑↑↑↑↑↑
/// ```
///
/// Detailed info (specially on the context-dependent bits on each
/// [`NcChannel`]'s 4th byte):
///
/// ```txt
///                             ~foreground channel~
/// NCCELL_WIDEASIAN_MASK: part of a wide glyph          ↓bits view↓               ↓hex mask↓
/// 1······· ········ ········ ········ ········ ········ ········ ········  =  8······· ········
///
/// NCCELL_FGDEFAULT_MASK: foreground is NOT "default color"
/// ·1······ ········ ········ ········ ········ ········ ········ ········  =  4······· ········
///
/// NCCELL_FG_ALPHA_MASK: foreground alpha (2bits)
/// ··11···· ········ ········ ········ ········ ········ ········ ········  =  3······· ········
///
/// NCCELL_FG_PALETTE: foreground uses palette index
/// ····1··· ········ ········ ········ ········ ········ ········ ········  =  ·8······ ········
///
/// NCCELL_NOBACKGROUND_MASK: glyph is entirely foreground
/// ·····1·· ········ ········ ········ ········ ········ ········ ········  =  ·4······ ········
///
/// reserved, must be 0
/// ······00 ········ ········ ········ ········ ········ ········ ········  =  ·3······ ········
///
/// NCCELL_FG_RGB_MASK: foreground in 3x8 RGB (rrggbb)
/// ········ 11111111 11111111 11111111 ········ ········ ········ ········  =  ··FFFFFF ········
/// ```

/// ```txt
///                             ~background channel~
/// reserved, must be 0                                  ↓bits view↓               ↓hex mask↓
/// ········ ········ ········ ········ 0······· ········ ········ ········  =  ········ 8·······
///
/// NCCELL_BGDEFAULT_MASK: background is NOT "default color"
/// ········ ········ ········ ········ ·1······ ········ ········ ········  =  ········ 4·······
///
/// NCCELL_BG_ALPHA_MASK: background alpha (2 bits)
/// ········ ········ ········ ········ ··11···· ········ ········ ········  =  ········ 3·······
///
/// NCCELL_BG_PALETTE: background uses palette index
/// ········ ········ ········ ········ ····1··· ········ ········ ········  =  ········ ·8······
///
/// reserved, must be 0
/// ········ ········ ········ ········ ·····000 ········ ········ ········  =  ········ ·7······
///
/// NCCELL_BG_RGB_MASK: background in 3x8 RGB (rrggbb)
/// 0········ ········ ········ ········ ········11111111 11111111 11111111  =  ········ ··FFFFFF
/// ```
/// `type in C: channels (uint64_t)`
///
/// ## `NcCell` Mask Flags
///
/// - [`NCCELL_BGDEFAULT_MASK`][crate::NCCELL_BGDEFAULT_MASK]
/// - [`NCCELL_BG_ALPHA_MASK`][crate::NCCELL_BG_ALPHA_MASK]
/// - [`NCCELL_BG_PALETTE`][crate::NCCELL_BG_PALETTE]
/// - [`NCCELL_BG_RGB_MASK`][crate::NCCELL_BG_RGB_MASK]
/// - [`NCCELL_FGDEFAULT_MASK`][crate::NCCELL_FGDEFAULT_MASK]
/// - [`NCCELL_FG_ALPHA_MASK`][crate::NCCELL_FG_ALPHA_MASK]
/// - [`NCCELL_FG_PALETTE`][crate::NCCELL_FG_PALETTE]
/// - [`NCCELL_FG_RGB_MASK`][crate::NCCELL_FG_RGB_MASK]
/// - [`NCCELL_NOBACKGROUND_MASK`][crate::NCCELL_NOBACKGROUND_MASK]
/// - [`NCCELL_WIDEASIAN_MASK`][crate::NCCELL_WIDEASIAN_MASK]
///
pub type NcChannelPair = u64;

// NcRgb
//
/// 24 bits broken into 3x 8bpp channels.
///
/// Unlike with [`NcChannel`], operations involving `NcRgb` ignores the last 4th byte
///
/// ## Diagram
///
/// ```txt
/// -------- RRRRRRRR GGGGGGGG BBBBBBBB
/// ```
///
/// `type in C: no data type`
///
pub type NcRgb = u32;

// NcColor
//
/// 8 bits representing a R/G/B color or alpha channel
///
/// ## Diagram
///
/// ```txt
/// CCCCCCCC (1 Byte)
/// ```
///
/// `type in C: no data type`
///
pub type NcColor = u8;

/// the [NcEgc][crate::NcEgc] which form the various levels of a given geometry.
///
/// If the geometry is wide, things are arranged with the rightmost side
/// increasing most quickly, i.e. it can be indexed as height arrays of
/// 1 + height glyphs.
/// i.e. The first five braille EGCs are all 0 on the left,
/// [0..4] on the right.
///
/// `type in C: blitset (struct)`
///
pub type NcBlitSet = crate::bindings::ffi::blitset;
