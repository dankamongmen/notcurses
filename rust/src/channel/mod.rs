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
// functions manually reimplemented: 44
// ------------------------------------------
// (X) wont:  3
// (+) done: 36 / 0
// (#) test: 21
// (W) wrap: 41
// ------------------------------------------
//W# channel_alpha
//W# channel_b
//W# channel_default_p
//W# channel_g
//W# channel_palindex_p
//W# channel_r
//W# channel_rgb8
//W# channel_set
//W# channel_set_alpha
//W# channel_set_default
//W# channel_set_not_default         // not in the original C API
//W# channel_set_rgb8
// X channel_set_rgb_clipped         // not needed
//W# channels_bchannel
//W+ channels_bg_alpha
//W+ channels_bg_default_p
//W# channels_bg_palindex_p
//W+ channels_bg_rgb
//W+ channels_bg_rgb8
//W# channels_combine
//W# channels_fchannel
//W+ channels_fg_alpha
//W+ channels_fg_default_p
//W# channels_fg_palindex_p
//W+ channels_fg_rgb
//W+ channels_fg_rgb8
//W# channels_set_bchannel
//W+ channels_set_bg_alpha
//W+ channels_set_bg_default
//W  channels_set_bg_not_default     // not in the original C API
//W# channels_set_bg_palindex
//W+ channels_set_bg_rgb
//W+ channels_set_bg_rgb8
// X channels_set_bg_rgb8_clipped    // not needed
//W  channels_set_default            // not in the original C API
//W# channels_set_fchannel
//W+ channels_set_fg_alpha
//W+ channels_set_fg_default
//W  channels_set_fg_not_default     // not in the original C API
//W# channels_set_fg_palindex
//W+ channels_set_fg_rgb
//W+ channels_set_fg_rgb8
// X channels_set_fg_rgb8_clipped    // not needed
//W  channels_set_not_default        // not in the original C API

#[allow(unused_imports)] // for the doc comments
use crate::{NcCell, NcRgba};

#[cfg(test)]
mod test;

mod methods;
mod reimplemented;
pub use methods::{NcChannelMethods, NcChannelsMethods};
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
/// The context details are documented in [`NcChannels`]
///
/// ## Diagram
///
/// ```txt
/// ~~AA~~~~ RRRRRRRR GGGGGGGG BBBBBBBB
/// ```
/// `type in C: channel (uint32_t)`
///
/// See also: [NcRgb] and [NcRgba] types.
pub type NcChannel = u32;

/// Extract these bits to get a channel's alpha value
pub const NCCHANNEL_ALPHA_MASK: u32 = crate::bindings::ffi::NC_BG_ALPHA_MASK;

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
/// `type in C: no data type`
///
pub type NcAlphaBits = u32;

/// [`NcAlphaBits`] bits indicating
/// [`NcCell`]'s foreground or background color will be a composite between
/// its color and the `NcCell`s' corresponding colors underneath it
pub const NCALPHA_BLEND: u32 = crate::bindings::ffi::NCALPHA_BLEND;

/// [`NcAlphaBits`] bits indicating
/// [`NcCell`]'s foreground color will be high-contrast (relative to the
/// computed background). Background cannot be highcontrast
pub const NCALPHA_HIGHCONTRAST: u32 = crate::bindings::ffi::NCALPHA_HIGHCONTRAST;

/// [`NcAlphaBits`] bits indicating
/// [`NcCell`]'s foreground or background color is used unchanged
pub const NCALPHA_OPAQUE: u32 = crate::bindings::ffi::NCALPHA_OPAQUE;

/// [`NcAlphaBits`] bits indicating
/// [`NcCell`]'s foreground or background color is derived entirely from the
/// `NcCell`s underneath it
pub const NCALPHA_TRANSPARENT: u32 = crate::bindings::ffi::NCALPHA_TRANSPARENT;

/// If this bit is set, we are *not* using the default background color
///
/// See the detailed diagram at [`NcChannels`][crate::NcChannels]
///
/// Note: This can also be used against a single [`NcChannel`]
pub const NCALPHA_BGDEFAULT_MASK: u32 = crate::bindings::ffi::NC_BGDEFAULT_MASK;

/// Extract these bits to get the background alpha mask
/// ([`NcAlphaBits`])
///
/// See the detailed diagram at [`NcChannels`][crate::NcChannels]
///
/// Note: This can also be used against a single [`NcChannel`]
pub const NCALPHA_BG_ALPHA_MASK: u32 = crate::bindings::ffi::NC_BG_ALPHA_MASK;

/// If this bit *and* [`NCALPHA_BGDEFAULT_MASK`] are set, we're using a
/// palette-indexed background color
///
/// See the detailed diagram at [`NcChannels`][crate::NcChannels]
///
/// Note: This can also be used against a single [`NcChannel`]
pub const NCALPHA_BG_PALETTE: u32 = crate::bindings::ffi::NC_BG_PALETTE;

/// Extract these bits to get the background [`NcRgb`][crate::NcRgb] value
///
/// See the detailed diagram at [`NcChannels`][crate::NcChannels]
///
/// Note: This can also be used against a single [`NcChannel`]
pub const NCALPHA_BG_RGB_MASK: u32 = crate::bindings::ffi::NC_BG_RGB_MASK;

/// If this bit is set, we are *not* using the default foreground color
///
/// See the detailed diagram at [`NcChannels`][crate::NcChannels]
///
/// Note: When working with a single [`NcChannel`] use [`NCALPHA_BGDEFAULT_MASK`];
pub const NCALPHA_FGDEFAULT_MASK: u64 = crate::bindings::ffi::NC_FGDEFAULT_MASK;

/// Extract these bits to get the foreground alpha mask
/// ([`NcAlphaBits`])
///
/// See the detailed diagram at [`NcChannels`][crate::NcChannels]
///
/// Note: When working with a single [`NcChannel`] use [`NCALPHA_BG_ALPHA_MASK`];
pub const NCALPHA_FG_ALPHA_MASK: u64 = crate::bindings::ffi::NC_FG_ALPHA_MASK;

/// If this bit *and* [`NCALPHA_FGDEFAULT_MASK`] are set, we're using a
/// palette-indexed background color
///
/// See the detailed diagram at [`NcChannels`][crate::NcChannels]
///
/// Note: When working with a single [`NcChannel`] use [`NCALPHA_BG_PALETTE`];
pub const NCALPHA_FG_PALETTE: u64 = crate::bindings::ffi::NC_FG_PALETTE;

/// Extract these bits to get the foreground [`NcRgb`][crate::NcRgb] value
///
/// See the detailed diagram at [`NcChannels`][crate::NcChannels]
///
/// Note: When working with a single [`NcChannel`] use [`NCALPHA_BG_RGB_MASK`];
pub const NCALPHA_FG_RGB_MASK: u64 = crate::bindings::ffi::NC_FG_RGB_MASK;

// NcChannels
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
/// [color(3NCURSES)](https://manpages.debian.org/stretch/ncurses-doc/color.3ncurses.en.html) and
/// [default_colors(3NCURSES)](https://manpages.debian.org/stretch/ncurses-doc/default_colors.3ncurses.en.html).
/// Ours is the same concept.
///
/// **Until the "not default color" bit is set, any color you load will be ignored.**
///
/// ## Diagram
///
/// ```txt
/// ~~AA~~~~|RRRRRRRR|GGGGGGGG|BBBBBBBB║~~AA~~~~|RRRRRRRR|GGGGGGGG|BBBBBBBB
/// ↑↑↑↑↑↑↑↑↑↑↑↑ foreground ↑↑↑↑↑↑↑↑↑↑↑║↑↑↑↑↑↑↑↑↑↑↑↑ background ↑↑↑↑↑↑↑↑↑↑↑
/// ```
///
/// Detailed info (specially on the context-dependent bits on each
/// [`NcChannel`]'s 4th byte):
///
/// ```txt
///                             ~foreground channel~
/// NCALPHA_WIDEASIAN_MASK: part of a wide glyph          ↓bits view↓               ↓hex mask↓
/// 1·······|········|········|········║········|········|········|········  =  8·······|········
///
/// NCALPHA_FGDEFAULT_MASK: foreground is NOT "default color"
/// ·1······|········|········|········║········|········|········|········  =  4·······|········
///
/// NCALPHA_FG_ALPHA_MASK: foreground alpha (2bits)
/// ··11····|········|········|········║········|········|········|········  =  3·······|········
///
/// NCALPHA_FG_PALETTE: foreground uses palette index
/// ····1···|········|········|········║········|········|········|········  =  ·8······|········
///
/// NCALPHA_NOBACKGROUND_MASK: glyph is entirely foreground
/// ·····1··|········|········|········║········|········|········|········  =  ·4······|········
///
/// reserved, must be 0
/// ······00|········|········|········║········|········|········|········  =  ·3······|········
///
/// NCALPHA_FG_RGB_MASK: foreground in 3x8 RGB (rrggbb)
/// ········|11111111|11111111|11111111║········|········|········|········  =  ··FFFFFF|········
/// ```

/// ```txt
///                             ~background channel~
/// reserved, must be 0                                  ↓bits view↓               ↓hex mask↓
/// ········|········|········|········║0·······|········|········|········  =  ········|8·······
///
/// NCALPHA_BGDEFAULT_MASK: background is NOT "default color"
/// ········|········|········|········║·1······|········|········|········  =  ········|4·······
///
/// NCALPHA_BG_ALPHA_MASK: background alpha (2 bits)
/// ········|········|········|········║··11····|········|········|········  =  ········|3·······
///
/// NCALPHA_BG_PALETTE: background uses palette index
/// ········|········|········|········║····1···|········|········|········  =  ········|·8······
///
/// reserved, must be 0
/// ········|········|········|········║·····000|········|········|········  =  ········|·7······
///
/// NCALPHA_BG_RGB_MASK: background in 3x8 RGB (rrggbb)
/// ········|········|········|········║········|11111111|11111111|11111111  =  ········|··FFFFFF
/// ```
/// `type in C: channels (uint64_t)`
///
/// ## `NcCell` Mask Flags
///
/// - [`NCALPHA_BGDEFAULT_MASK`][crate::NCALPHA_BGDEFAULT_MASK]
/// - [`NCALPHA_BG_ALPHA_MASK`][crate::NCALPHA_BG_ALPHA_MASK]
/// - [`NCALPHA_BG_PALETTE`][crate::NCALPHA_BG_PALETTE]
/// - [`NCALPHA_BG_RGB_MASK`][crate::NCALPHA_BG_RGB_MASK]
/// - [`NCALPHA_FGDEFAULT_MASK`][crate::NCALPHA_FGDEFAULT_MASK]
/// - [`NCALPHA_FG_ALPHA_MASK`][crate::NCALPHA_FG_ALPHA_MASK]
/// - [`NCALPHA_FG_PALETTE`][crate::NCALPHA_FG_PALETTE]
/// - [`NCALPHA_FG_RGB_MASK`][crate::NCALPHA_FG_RGB_MASK]
///
pub type NcChannels = u64;

#[deprecated = "use NcChannels instead"]
#[doc(hidden)]
pub type NcChannelPair = NcChannels;

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
/// `type in C: no data type`
///
/// See also: [NcRgba] and [NcChannel] types.
pub type NcRgb = u32;

// NcComponent
//
/// 8 bits representing an R/G/B color component or an alpha channel component.
///
/// ## Diagram
///
/// ```txt
/// CCCCCCCC (1 Byte)
/// ```
/// `type in C: no data type`
pub type NcComponent = u8;
