//! `channel*_*` reimplemented functions.

use crate::{
    NcAlphaBits, NcChannel, NcChannelPair, NcColor, NcPaletteIndex, NcRgb,
    NCCELL_ALPHA_HIGHCONTRAST, NCCELL_ALPHA_OPAQUE, NCCELL_BGDEFAULT_MASK, NCCELL_BG_PALETTE,
    NCCELL_BG_RGB_MASK, NCCELL_FGDEFAULT_MASK, NCCELL_FG_PALETTE, NCCHANNEL_ALPHA_MASK,
};

// Alpha -----------------------------------------------------------------------

/// Gets the [NcAlphaBits] from an [NcChannel].
#[inline]
pub const fn channel_alpha(channel: NcChannel) -> NcAlphaBits {
    channel & NCCHANNEL_ALPHA_MASK
}

/// Sets the [NcAlphaBits] of an [NcChannel].
#[inline]
pub fn channel_set_alpha(channel: &mut NcChannel, alpha: NcAlphaBits) {
    let alpha_clean = alpha & NCCHANNEL_ALPHA_MASK;
    *channel = alpha_clean | (*channel & !NCCHANNEL_ALPHA_MASK);

    if alpha != NCCELL_ALPHA_OPAQUE {
        // indicate that we are *not* using the default background color
        *channel |= NCCELL_BGDEFAULT_MASK;
    }
}

/// Gets the foreground [NcAlphabits] from an [NcChannelPair], shifted to LSBs.
#[inline]
pub const fn channels_fg_alpha(channels: NcChannelPair) -> NcAlphaBits {
    channel_alpha(channels_fchannel(channels))
}

/// Gets the background [NcAlphabits] from an [NcChannelPair], shifted to LSBs.
#[inline]
pub const fn channels_bg_alpha(channels: NcChannelPair) -> NcAlphaBits {
    channel_alpha(channels_bchannel(channels))
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

// Channels --------------------------------------------------------------------

/// Extracts the background [NcChannel] from a [NcChannelPair].
#[inline]
pub const fn channels_bchannel(channels: NcChannelPair) -> NcChannel {
    (channels & 0xffffffff_u64) as NcChannel
}

/// Extracts the foreground [NcChannel] from an [NcChannelPair].
#[inline]
pub const fn channels_fchannel(channels: NcChannelPair) -> NcChannel {
    channels_bchannel(channels >> 32)
}

/// Sets the background [NcChannel] of an [NcChannelPair].
#[inline]
pub fn channels_set_bchannel(channels: &mut NcChannelPair, bchannel: NcChannel) -> NcChannelPair {
    *channels = (*channels & 0xffffffff00000000_u64) | bchannel as u64;
    *channels
}

/// Sets the foreground [NcChannel] of an [NcChannelPair].
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

// NcColor ---------------------------------------------------------------------

/// Gets the red [NcColor] from an [NcChannel].
#[inline]
pub const fn channel_r(channel: NcChannel) -> NcColor {
    ((channel & 0xff0000) >> 16) as NcColor
}

/// Gets the green [NcColor] from an [NcChannel].
#[inline]
pub const fn channel_g(channel: NcChannel) -> NcColor {
    ((channel & 0x00ff00) >> 8) as NcColor
}

/// Gets the blue [NcColor] from an [NcChannel].
#[inline]
pub const fn channel_b(channel: NcChannel) -> NcColor {
    (channel & 0x0000ff) as NcColor
}

/// Sets the red [NcColor] of an [NcChannel], and returns it.
// Not in the C API.
#[inline]
pub fn channel_set_r(channel: &mut NcChannel, r: NcColor) -> NcChannel {
    *channel = (r as NcChannel) << 16 | (*channel & 0xff00) | (*channel & 0xff);
    *channel
}

/// Sets the green [NcColor] of an [NcChannel], and returns it.
// Not in the C API.
#[inline]
pub fn channel_set_g(channel: &mut NcChannel, g: NcColor) -> NcChannel {
    *channel = (*channel & 0xff0000) | (g as NcChannel) << 8 | (*channel & 0xff);
    *channel
}

/// Sets the blue [NcColor] of an [NcChannel], and returns it.
// Not in the C API.
#[inline]
pub fn channel_set_b(channel: &mut NcChannel, b: NcColor) -> NcChannel {
    *channel = (*channel & 0xff0000) | (*channel & 0xff00) | (b as NcChannel);
    *channel
}

/// Gets the three RGB [NcColor]s from an [NcChannel], and returns it.
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

/// Sets the three RGB [NcColor]s an [NcChannel], and marks it as not using the
/// "default color", retaining the other bits unchanged.
#[inline]
pub fn channel_set_rgb8(channel: &mut NcChannel, r: NcColor, g: NcColor, b: NcColor) {
    let rgb: NcRgb = (r as NcChannel) << 16 | (g as NcChannel) << 8 | (b as NcChannel);
    *channel = (*channel & !NCCELL_BG_RGB_MASK) | NCCELL_BGDEFAULT_MASK | rgb;
}

/// Gets the three foreground RGB [NcColors] from an [NcChannelPair], and
/// returns the foreground [NcChannel] (which can have some extra bits set).
#[inline]
pub fn channels_fg_rgb8(
    channels: NcChannelPair,
    r: &mut NcColor,
    g: &mut NcColor,
    b: &mut NcColor,
) -> NcChannel {
    channel_rgb8(channels_fchannel(channels), r, g, b)
}

/// Gets the three background RGB [NcColors] from an [NcChannelPair], and
/// returns the background [NcChannel] (which can have some extra bits set).
#[inline]
pub fn channels_bg_rgb8(
    channels: NcChannelPair,
    r: &mut NcColor,
    g: &mut NcColor,
    b: &mut NcColor,
) -> NcChannel {
    channel_rgb8(channels_bchannel(channels), r, g, b)
}

/// Sets the three foreground RGB [NcColor]s of an [NcChannelPair], and
/// marks it as not using the "default color".
#[inline]
pub fn channels_set_fg_rgb8(channels: &mut NcChannelPair, r: NcColor, g: NcColor, b: NcColor) {
    let mut channel = channels_fchannel(*channels);
    channel_set_rgb8(&mut channel, r, g, b);
    *channels = (channel as u64) << 32 | *channels & 0xffffffff_u64;
}

/// Sets the three background RGB [NcColor]s of an [NcChannelPair], and
/// marks it as not using the "default color".
#[inline]
pub fn channels_set_bg_rgb8(channels: &mut NcChannelPair, r: NcColor, g: NcColor, b: NcColor) {
    let mut channel = channels_bchannel(*channels);
    channel_set_rgb8(&mut channel, r, g, b);
    channels_set_bchannel(channels, channel);
}

// NcRgb -----------------------------------------------------------------------

/// Gets the foreground [NcRgb] from an [NcChannelPair], shifted to LSBs.
#[inline]
pub fn channels_fg_rgb(channels: NcChannelPair) -> NcChannel {
    channels_fchannel(channels) & NCCELL_BG_RGB_MASK
}

/// Gets the background [NcRgb] from an [NcChannelPair], shifted to LSBs.
#[inline]
pub fn channels_bg_rgb(channels: NcChannelPair) -> NcChannel {
    channels_bchannel(channels) & NCCELL_BG_RGB_MASK
}

/// Gets the [NcRgb] of an [NcChannel].
///
/// This function basically removes the 4th byte of the NcChannel.
// Not in the C API
#[inline]
pub const fn channel_rgb(channel: NcChannel) -> NcRgb {
    channel & NCCELL_BG_RGB_MASK
}

/// Sets the [NcRgb] of an [NcChannel], and marks it
/// as not using the "default color", retaining the other bits unchanged.
#[inline]
pub fn channel_set(channel: &mut NcChannel, rgb: NcRgb) {
    *channel = (*channel & !NCCELL_BG_RGB_MASK) | NCCELL_BGDEFAULT_MASK | (rgb & 0x00ffffff);
}

/// Sets the foreground [NcRgb] of an [NcChannelPair],
/// and marks it as not using the the "default color".
#[inline]
pub fn channels_set_fg_rgb(channels: &mut NcChannelPair, rgb: NcRgb) {
    let mut channel = channels_fchannel(*channels);
    channel_set(&mut channel, rgb);
    *channels = (channel as u64) << 32 | *channels & 0xffffffff_u64;
}

/// Sets the foreground [NcRgb] of an [NcChannelPair],
/// and marks it as not using the the "default color".
#[inline]
pub fn channels_set_bg_rgb(channels: &mut NcChannelPair, rgb: NcRgb) {
    let mut channel = channels_bchannel(*channels);
    channel_set(&mut channel, rgb);
    channels_set_bchannel(channels, channel);
}

// Default ---------------------------------------------------------------------

/// Is this [NcChannel] using the "default color" rather than RGB/palette-indexed?
#[inline]
pub const fn channel_default_p(channel: NcChannel) -> bool {
    (channel & NCCELL_BGDEFAULT_MASK) == 0
}

/// Marks an [NcChannel] as using its "default color", which also marks it opaque.
#[inline]
pub fn channel_set_default(channel: &mut NcChannel) -> NcChannel {
    *channel &= !(NCCELL_BGDEFAULT_MASK | NCCELL_ALPHA_HIGHCONTRAST);
    *channel
}

/// Is the foreground of an [NcChannelPair] using the "default foreground color"?
#[inline]
pub fn channels_fg_default_p(channels: NcChannelPair) -> bool {
    channel_default_p(channels_fchannel(channels))
}

/// Is the background using the "default background color"? The "default
/// background color" must generally be used to take advantage of
/// terminal-effected transparency.
#[inline]
pub fn channels_bg_default_p(channels: NcChannelPair) -> bool {
    channel_default_p(channels_bchannel(channels))
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

// Palette ---------------------------------------------------------------------

/// Is this [NcChannel] using palette-indexed color rather than RGB?
#[inline]
pub fn channel_palindex_p(channel: NcChannel) -> bool {
    !(channel_default_p(channel) && (channel & NCCELL_BG_PALETTE) == 0)
}

/// Is the foreground of an [NcChannelPair] using an [indexed][NcPaletteIndex]
/// [NcPalette][crate::NcPalette] color?
#[inline]
pub fn channels_fg_palindex_p(channels: NcChannelPair) -> bool {
    channel_palindex_p(channels_fchannel(channels))
}

/// Is the background of an [NcChannelPair] using an [indexed][NcPaletteIndex]
/// [NcPalette][crate::NcPalette] color?
#[inline]
pub fn channels_bg_palindex_p(channels: NcChannelPair) -> bool {
    channel_palindex_p(channels_bchannel(channels))
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
