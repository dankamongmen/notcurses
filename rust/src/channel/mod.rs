//! [`NcChannel`] & [`NcChannelPair`] `channel*_*` static fn reimplementations

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
// functions already exported by bindgen : 0
// ------------------------------------------
//
// static inline functions total: 39
// ------------------------------------------ (implement / remaining)
// (X) wont:  3
// (+) done: 34 /  2
// (#) test: 19 / 17
// ------------------------------------------
//# channel_alpha
//# channel_b
//# channel_default_p
//# channel_g
//# channel_palindex_p
//# channel_r
//# channel_rgb8
//# channels_bchannel
//+ channels_bg_alpha
//+ channels_bg_default_p
//# channels_bg_palindex_p
//+ channels_bg_rgb
//+ channels_bg_rgb8
//# channels_combine
//+ channel_set
//# channel_set_alpha
//# channel_set_default
//# channel_set_rgb8
//X channel_set_rgb_clipped
//# channels_fchannel
//+ channels_fg_alpha
//+ channels_fg_default_p
//# channels_fg_palindex_p
//+ channels_fg_rgb
//+ channels_fg_rgb8
//# channels_set_bchannel
//+ channels_set_bg_alpha
//+ channels_set_bg_default
//# channels_set_bg_palindex
//+ channels_set_bg_rgb
//+ channels_set_bg_rgb8
//X channels_set_bg_rgb8_clipped
//# channels_set_fchannel
//+ channels_set_fg_alpha
//+ channels_set_fg_default
//# channels_set_fg_palindex
//+ channels_set_fg_rgb
//+ channels_set_fg_rgb8
//X channels_set_fg_rgb8_clipped

#[cfg(test)]
mod tests;

use crate::{
    NcAlphaBits, NcChannel, NcChannelPair, NcColor, NcPaletteIndex, NcRgb,
    NCCELL_ALPHA_HIGHCONTRAST, NCCELL_ALPHA_OPAQUE, NCCELL_BGDEFAULT_MASK, NCCELL_BG_PALETTE,
    NCCELL_BG_RGB_MASK, NCCELL_FGDEFAULT_MASK, NCCELL_FG_PALETTE, NCCHANNEL_ALPHA_MASK,
};

/// Extracts the [NcColor] 8-bit red component from a 32-bit [NcChannel].
#[inline]
pub const fn channel_r(channel: NcChannel) -> NcColor {
    ((channel & 0xff0000) >> 16) as NcColor
}

/// Extracts the [NcColor] 8-bit green component from a 32-bit [NcChannel].
#[inline]
pub const fn channel_g(channel: NcChannel) -> NcColor {
    ((channel & 0x00ff00) >> 8) as NcColor
}

/// Extracts the [NcColor] 8-bit blue component from a 32-bit [NcChannel].
#[inline]
pub const fn channel_b(channel: NcChannel) -> NcColor {
    (channel & 0x0000ff) as NcColor
}

/// Extracts the three [NcColor] 8-bit RGB components from a 32-bit [NcChannel].
#[inline]
pub fn channel_rgb8(
    channel: NcChannel,
    r: &mut NcColor,
    g: &mut NcColor,
    b: &mut NcColor,
) -> NcChannel {
    *r = channel_r(channel);
    *g = channel_g(channel);
    *b = channel_b(channel);
    channel
}

/// Sets the three [NcColor] 8-bit components of a 32-bit [NcChannel], and marks
/// it as not using the "default color". Retain the other bits unchanged.
#[inline]
pub fn channel_set_rgb8(channel: &mut NcChannel, r: NcColor, g: NcColor, b: NcColor) {
    let rgb: NcRgb = (r as NcChannel) << 16 | (g as NcChannel) << 8 | (b as NcChannel);
    *channel = (*channel & !NCCELL_BG_RGB_MASK) | NCCELL_BGDEFAULT_MASK | rgb;
}

/// Sets the [NcRgb] 24-bit RGB value of a 32-bit [NcChannel], and marks it as
/// not using the "default color". Retain the other bits unchanged.
#[inline]
pub fn channel_set(channel: &mut NcChannel, rgb: NcRgb) {
    *channel = (*channel & !NCCELL_BG_RGB_MASK) | NCCELL_BGDEFAULT_MASK | (rgb & 0x00ffffff);
}

/// Extracts the [NcAlphaBits] 2-bit component from a 32-bit [NcChannel].
#[inline]
pub fn channel_alpha(channel: NcChannel) -> NcAlphaBits {
    channel & NCCHANNEL_ALPHA_MASK
}

/// Sets the [NcAlphaBits] 2-bit component of a 32-bit [NcChannel].
#[inline]
pub fn channel_set_alpha(channel: &mut NcChannel, alpha: NcAlphaBits) {
    let alpha_clean = alpha & NCCHANNEL_ALPHA_MASK;
    *channel = alpha_clean | (*channel & !NCCHANNEL_ALPHA_MASK);

    if alpha != NCCELL_ALPHA_OPAQUE {
        // indicate that we are *not* using the default background color
        *channel |= NCCELL_BGDEFAULT_MASK;
    }
}

/// Is this [NcChannel] using the "default color" rather than RGB/palette-indexed?
#[inline]
pub fn channel_default_p(channel: NcChannel) -> bool {
    (channel & NCCELL_BGDEFAULT_MASK) == 0
}

/// Is this [NcChannel] using palette-indexed color rather than RGB?
#[inline]
pub fn channel_palindex_p(channel: NcChannel) -> bool {
    !(channel_default_p(channel) && (channel & NCCELL_BG_PALETTE) == 0)
}

/// Marks an [NcChannel] as using its "default color", which also marks it opaque.
#[inline]
pub fn channel_set_default(channel: &mut NcChannel) -> NcChannel {
    *channel &= !(NCCELL_BGDEFAULT_MASK | NCCELL_ALPHA_HIGHCONTRAST);
    *channel
}

/// Extracts the 32-bit background [NcChannel] from a [NcChannelPair].
#[inline]
pub fn channels_bchannel(channels: NcChannelPair) -> NcChannel {
    (channels & 0xffffffff_u64) as NcChannel
}

/// Extracts the 32-bit foreground [NcChannel] from an [NcChannelPair].
#[inline]
pub fn channels_fchannel(channels: NcChannelPair) -> NcChannel {
    channels_bchannel(channels >> 32)
}

/// Sets the 32-bit background [NcChannel] of an [NcChannelPair].
#[inline]
pub fn channels_set_bchannel(channels: &mut NcChannelPair, bchannel: NcChannel) -> NcChannelPair {
    *channels = (*channels & 0xffffffff00000000_u64) | bchannel as u64;
    *channels
}

/// Sets the 32-bit foreground [NcChannel] of an [NcChannelPair].
#[inline]
pub fn channels_set_fchannel(channels: &mut NcChannelPair, fchannel: NcChannel) -> NcChannelPair {
    *channels = (*channels & 0xffffffff_u64) | (fchannel as u64) << 32;
    *channels
}

/// Combines two [NcChannel]s into a [NcChannelPair].
#[inline]
pub fn channels_combine(fchannel: NcChannel, bchannel: NcChannel) -> NcChannelPair {
    let mut channels: NcChannelPair = 0;
    channels_set_fchannel(&mut channels, fchannel);
    channels_set_bchannel(&mut channels, bchannel);
    channels
}

/// Extracts the foreground [NcRgb] 24-bit value from an [NcChannelPair],
/// shifted to LSBs.
#[inline]
pub fn channels_fg_rgb(channels: NcChannelPair) -> NcChannel {
    channels_fchannel(channels) & NCCELL_BG_RGB_MASK
}

/// Extracts the background [NcRgb] 24-bit value from an [NcChannelPair],
/// shifted to LSBs.
#[inline]
pub fn channels_bg_rgb(channels: NcChannelPair) -> NcChannel {
    channels_bchannel(channels) & NCCELL_BG_RGB_MASK
}

/// Extracts the foreground [NcAlphabits] from an [NcChannelPair], shifted to LSBs.
#[inline]
pub fn channels_fg_alpha(channels: NcChannelPair) -> NcAlphaBits {
    channel_alpha(channels_fchannel(channels))
}

/// Extracts the background [NcAlphabits] from an [NcChannelPair], shifted to LSBs.
#[inline]
pub fn channels_bg_alpha(channels: NcChannelPair) -> NcAlphaBits {
    channel_alpha(channels_bchannel(channels))
}

/// Extracts the foreground [NcRgb] 24-bit value from an [NcChannelPair], and
/// saves it split into three [NcColor] 8-bit components. Also returns the
/// corresponding [NcChannel] (which can have some extra bits set).
#[inline]
pub fn channels_fg_rgb8(
    channels: NcChannelPair,
    r: &mut NcColor,
    g: &mut NcColor,
    b: &mut NcColor,
) -> NcChannel {
    channel_rgb8(channels_fchannel(channels), r, g, b)
}

/// Extracts the background [NcRgb] 24-bit value from an [NcChannelPair], and
/// saves it split into three [NcColor] 8-bit components. Also returns the
/// corresponding [NcChannel] (which can have some extra bits set).
#[inline]
pub fn channels_bg_rgb8(
    channels: NcChannelPair,
    r: &mut NcColor,
    g: &mut NcColor,
    b: &mut NcColor,
) -> NcChannel {
    channel_rgb8(channels_bchannel(channels), r, g, b)
}

/// Sets the RGB [NcColor] components for the foreground [NcChannel] of an
/// [NcChannelPair] 64-bit variable, and marks it as not using the "default color".
#[inline]
pub fn channels_set_fg_rgb8(channels: &mut NcChannelPair, r: NcColor, g: NcColor, b: NcColor) {
    let mut channel = channels_fchannel(*channels);
    channel_set_rgb8(&mut channel, r, g, b);
    *channels = (channel as u64) << 32 | *channels & 0xffffffff_u64;
}

/// Sets the [NcRgb] 24-bit value for the foreground [NcChannel] of an
/// [NcChannelPair] 64-bit variable, and marks it as not using the "default color".
#[inline]
pub fn channels_set_fg_rgb(channels: &mut NcChannelPair, rgb: NcRgb) {
    let mut channel = channels_fchannel(*channels);
    channel_set(&mut channel, rgb);
    *channels = (channel as u64) << 32 | *channels & 0xffffffff_u64;
}

/// Sets the RGB [NcColor] components for the background [NcChannel] of an
/// [NcChannelPair] 64-bit variable, and marks it as not using the "default color".
#[inline]
pub fn channels_set_bg_rgb8(channels: &mut NcChannelPair, r: NcColor, g: NcColor, b: NcColor) {
    let mut channel = channels_bchannel(*channels);
    channel_set_rgb8(&mut channel, r, g, b);
    channels_set_bchannel(channels, channel);
}

/// Sets the [NcRgb] 24-bit value for the background [NcChannel] of an
/// [NcChannelPair] 64-bit variable, and marks it as not using the "default color".
#[inline]
pub fn channels_set_bg_rgb(channels: &mut NcChannelPair, rgb: NcRgb) {
    let mut channel = channels_bchannel(*channels);
    channel_set(&mut channel, rgb);
    channels_set_bchannel(channels, channel);
}

/// Sets the [NcAlphaBits] of the foreground [NcChannel] of an [NcChannelPair].
#[inline]
pub fn channels_set_fg_alpha(channels: &mut NcChannelPair, alpha: NcAlphaBits) {
    let mut channel = channels_fchannel(*channels);
    channel_set_alpha(&mut channel, alpha);
    *channels = (channel as NcChannelPair) << 32 | *channels & 0xffffffff_u64;
}

/// Sets the [NcAlphaBits] of the background [NcChannel] of an [NcChannelPair].
#[inline]
pub fn channels_set_bg_alpha(channels: &mut NcChannelPair, alpha: NcAlphaBits) {
    let mut alpha_clean = alpha;
    if alpha == NCCELL_ALPHA_HIGHCONTRAST {
        // forbidden for background alpha, so makes it opaque
        alpha_clean = NCCELL_ALPHA_OPAQUE;
    }
    let mut channel = channels_bchannel(*channels);
    channel_set_alpha(&mut channel, alpha_clean);
    channels_set_bchannel(channels, channel);
}

/// Is the foreground of an [NcChannelPair] using the "default foreground color"?
#[inline]
pub fn channels_fg_default_p(channels: NcChannelPair) -> bool {
    channel_default_p(channels_fchannel(channels))
}

/// Is the foreground of an [NcChannelPair] using an [indexed][NcPaletteIndex]
/// [NcPalette][crate::NcPalette] color?
#[inline]
pub fn channels_fg_palindex_p(channels: NcChannelPair) -> bool {
    channel_palindex_p(channels_fchannel(channels))
}

/// Is the background using the "default background color"? The "default
/// background color" must generally be used to take advantage of
/// terminal-effected transparency.
#[inline]
pub fn channels_bg_default_p(channels: NcChannelPair) -> bool {
    channel_default_p(channels_bchannel(channels))
}

/// Is the background of an [NcChannelPair] using an [indexed][NcPaletteIndex]
/// [NcPalette][crate::NcPalette] color?
#[inline]
pub fn channels_bg_palindex_p(channels: NcChannelPair) -> bool {
    channel_palindex_p(channels_bchannel(channels))
}

/// Sets an [NcCell]'s background [NcPaletteIndex].
///
/// Also sets [NCCELL_BG_PALETTE] and [NCCELL_ALPHA_OPAQUE],
/// and clears out [NCCELL_BGDEFAULT_MASK].
#[inline]
pub fn channels_set_bg_palindex(channels: &mut NcChannelPair, index: NcPaletteIndex) {
    *channels |= NCCELL_BGDEFAULT_MASK as NcChannelPair;
    *channels |= NCCELL_BG_PALETTE as NcChannelPair;
    channels_set_bg_alpha(channels, NCCELL_ALPHA_OPAQUE);
    *channels &= 0xffffffffff000000;
    *channels |= index as NcChannelPair;
}

/// Sets an [NcCell]'s foreground [NcPaletteIndex].
///
/// Also sets [NCCELL_FG_PALETTE] and [NCCELL_ALPHA_OPAQUE],
/// and clears out [NCCELL_FGDEFAULT_MASK].
#[inline]
pub fn channels_set_fg_palindex(channels: &mut NcChannelPair, index: NcPaletteIndex) {
    *channels |= NCCELL_FGDEFAULT_MASK;
    *channels |= NCCELL_FG_PALETTE as NcChannelPair;
    channels_set_fg_alpha(channels, NCCELL_ALPHA_OPAQUE);
    *channels &= 0xff000000ffffffff as NcChannelPair;
    *channels |= (index as NcChannelPair) << 32;
}

/// Marks the foreground of an [NcChannelPair] as using its "default color",
/// and returns the new [NcChannelPair].
#[inline]
pub fn channels_set_fg_default(channels: &mut NcChannelPair) -> NcChannelPair {
    let mut channel = channels_fchannel(*channels);
    channel_set_default(&mut channel);
    *channels = (channel as u64) << 32 | *channels & 0xffffffff_u64;
    *channels
}

/// Marks the background of an [NcChannelPair] as using its "default color",
/// and returns the new [NcChannelPair].
#[inline]
pub fn channels_set_bg_default(channels: &mut NcChannelPair) -> NcChannelPair {
    let mut channel = channels_bchannel(*channels);
    channel_set_default(&mut channel);
    channels_set_bchannel(channels, channel);
    *channels
}
