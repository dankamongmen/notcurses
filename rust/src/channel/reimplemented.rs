//! `channel*_*` reimplemented functions.

use crate::{
    NcAlphaBits, NcChannel, NcChannelPair, NcColor, NcPaletteIndex, NcRgb,
    NCCELL_ALPHA_HIGHCONTRAST, NCCELL_ALPHA_OPAQUE, NCCELL_BGDEFAULT_MASK, NCCELL_BG_PALETTE,
    NCCELL_BG_RGB_MASK, NCCELL_FGDEFAULT_MASK, NCCELL_FG_PALETTE, NCCHANNEL_ALPHA_MASK,
};

// Alpha -----------------------------------------------------------------------

/// Gets the [NcAlphaBits] from an [NcChannel].
///
/// *Method: NcChannel.[alpha()][NcChannel#method.alpha]*
#[inline]
pub const fn channel_alpha(channel: NcChannel) -> NcAlphaBits {
    channel & NCCHANNEL_ALPHA_MASK
}

/// Sets the [NcAlphaBits] of an [NcChannel].
///
/// *Method: NcChannel.[set_alpha()][NcChannel#method.set_alpha]*
#[inline]
pub fn channel_set_alpha(channel: &mut NcChannel, alpha: NcAlphaBits) {
    let alpha_clean = alpha & NCCHANNEL_ALPHA_MASK;
    *channel = alpha_clean | (*channel & !NCCHANNEL_ALPHA_MASK);

    if alpha != NCCELL_ALPHA_OPAQUE {
        // indicate that we are *not* using the default background color
        *channel |= NCCELL_BGDEFAULT_MASK;
    }
}

/// Gets the foreground [NcAlphaBits] from an [NcChannelPair], shifted to LSBs.
///
/// *Method: NcChannelPair.[fg_alpha()][NcChannelPair#method.fg_alpha]*
#[inline]
pub const fn channels_fg_alpha(channels: NcChannelPair) -> NcAlphaBits {
    channel_alpha(channels_fchannel(channels))
}

/// Gets the background [NcAlphaBits] from an [NcChannelPair], shifted to LSBs.
///
/// *Method: NcChannelPair.[bg_alpha()][NcChannelPair#method.bg_alpha]*
#[inline]
pub const fn channels_bg_alpha(channels: NcChannelPair) -> NcAlphaBits {
    channel_alpha(channels_bchannel(channels))
}

/// Sets the [NcAlphaBits] of the foreground [NcChannel] of an [NcChannelPair].
///
/// *Method: NcChannelPair.[set_fg_alpha()][NcChannelPair#method.set_fg_alpha]*
#[inline]
pub fn channels_set_fg_alpha(channels: &mut NcChannelPair, alpha: NcAlphaBits) {
    let mut channel = channels_fchannel(*channels);
    channel_set_alpha(&mut channel, alpha);
    *channels = (channel as NcChannelPair) << 32 | *channels & 0xffffffff_u64;
}

/// Sets the [NcAlphaBits] of the background [NcChannel] of an [NcChannelPair].
///
/// *Method: NcChannelPair.[set_bg_alpha()][NcChannelPair#method.set_bg_alpha]*
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
///
/// *Method: NcChannelPair.[bchannel()][NcChannelPair#method.bchannel]*
#[inline]
pub const fn channels_bchannel(channels: NcChannelPair) -> NcChannel {
    (channels & 0xffffffff_u64) as NcChannel
}

/// Extracts the foreground [NcChannel] from an [NcChannelPair].
///
/// *Method: NcChannelPair.[fchannel()][NcChannelPair#method.fchannel]*
#[inline]
pub const fn channels_fchannel(channels: NcChannelPair) -> NcChannel {
    channels_bchannel(channels >> 32)
}

/// Sets the background [NcChannel] of an [NcChannelPair].
///
/// *Method: NcChannelPair.[set_bchannel()][NcChannelPair#method.set_bchannel]*
#[inline]
pub fn channels_set_bchannel(channels: &mut NcChannelPair, bchannel: NcChannel) -> NcChannelPair {
    *channels = (*channels & 0xffffffff00000000_u64) | bchannel as u64;
    *channels
}

/// Sets the foreground [NcChannel] of an [NcChannelPair].
///
/// *Method: NcChannelPair.[set_fchannel()][NcChannelPair#method.set_fchannel]*
#[inline]
pub fn channels_set_fchannel(channels: &mut NcChannelPair, fchannel: NcChannel) -> NcChannelPair {
    *channels = (*channels & 0xffffffff_u64) | (fchannel as u64) << 32;
    *channels
}

/// Combines two [NcChannel]s into an [NcChannelPair].
///
/// *Method: NcChannelPair.[combine()][NcChannelPair#method.combine]*
#[inline]
pub fn channels_combine(fchannel: NcChannel, bchannel: NcChannel) -> NcChannelPair {
    let mut channels: NcChannelPair = 0;
    channels_set_fchannel(&mut channels, fchannel);
    channels_set_bchannel(&mut channels, bchannel);
    channels
}

// NcColor ---------------------------------------------------------------------

/// Gets the red [NcColor] from an [NcChannel].
///
/// *Method: NcChannel.[r()][NcChannel#method.r]*
#[inline]
pub const fn channel_r(channel: NcChannel) -> NcColor {
    ((channel & 0xff0000) >> 16) as NcColor
}

/// Gets the green [NcColor] from an [NcChannel].
///
/// *Method: NcChannel.[g()][NcChannel#method.g]*
#[inline]
pub const fn channel_g(channel: NcChannel) -> NcColor {
    ((channel & 0x00ff00) >> 8) as NcColor
}

/// Gets the blue [NcColor] from an [NcChannel].
///
/// *Method: NcChannel.[b()][NcChannel#method.b]*
#[inline]
pub const fn channel_b(channel: NcChannel) -> NcColor {
    (channel & 0x0000ff) as NcColor
}

/// Sets the red [NcColor] of an [NcChannel], and returns it.
///
/// *Method: NcChannel.[set_r()][NcChannel#method.set_r]*
//
// Not in the C API.
#[inline]
pub fn channel_set_r(channel: &mut NcChannel, r: NcColor) -> NcChannel {
    *channel = (r as NcChannel) << 16 | (*channel & 0xff00) | (*channel & 0xff);
    *channel
}

/// Sets the green [NcColor] of an [NcChannel], and returns it.
///
/// *Method: NcChannel.[set_g()][NcChannel#method.set_g]*
//
// Not in the C API.
#[inline]
pub fn channel_set_g(channel: &mut NcChannel, g: NcColor) -> NcChannel {
    *channel = (*channel & 0xff0000) | (g as NcChannel) << 8 | (*channel & 0xff);
    *channel
}

/// Sets the blue [NcColor] of an [NcChannel], and returns it.
///
/// *Method: NcChannel.[set_b()][NcChannel#method.set_b]*
//
// Not in the C API.
#[inline]
pub fn channel_set_b(channel: &mut NcChannel, b: NcColor) -> NcChannel {
    *channel = (*channel & 0xff0000) | (*channel & 0xff00) | (b as NcChannel);
    *channel
}

/// Gets the three RGB [NcColor]s from an [NcChannel], and returns it.
///
/// *Method: NcChannel.[rgb8()][NcChannel#method.rgb8]*
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

/// Sets the three RGB [NcColor]s an [NcChannel], and marks it as NOT using the
/// "default color", retaining the other bits unchanged.
///
/// *Method: NcChannel.[set_rgb8()][NcChannel#method.set_rgb8]*
#[inline]
pub fn channel_set_rgb8(channel: &mut NcChannel, r: NcColor, g: NcColor, b: NcColor) {
    let rgb: NcRgb = (r as NcChannel) << 16 | (g as NcChannel) << 8 | (b as NcChannel);
    *channel = (*channel & !NCCELL_BG_RGB_MASK) | NCCELL_BGDEFAULT_MASK | rgb;
}

/// Gets the three foreground RGB [NcColor]s from an [NcChannelPair], and
/// returns the foreground [NcChannel] (which can have some extra bits set).
///
/// *Method: NcChannelPair.[fg_rgb8()][NcChannelPair#method.fg_rgb8]*
#[inline]
pub fn channels_fg_rgb8(
    channels: NcChannelPair,
    r: &mut NcColor,
    g: &mut NcColor,
    b: &mut NcColor,
) -> NcChannel {
    channel_rgb8(channels_fchannel(channels), r, g, b)
}

/// Gets the three background RGB [NcColor]s from an [NcChannelPair], and
/// returns the background [NcChannel] (which can have some extra bits set).
///
/// *Method: NcChannelPair.[bg_rgb8()][NcChannelPair#method.bg_rgb8]*
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
/// marks it as NOT using the "default color", retaining the other bits unchanged.
///
/// Unlike the original C API, it also returns the new NcChannelPair.
///
/// *Method: NcChannelPair.[set_fg_rgb8()][NcChannelPair#method.set_fg_rgb8]*
#[inline]
pub fn channels_set_fg_rgb8(
    channels: &mut NcChannelPair,
    r: NcColor,
    g: NcColor,
    b: NcColor,
) -> NcChannelPair {
    let mut channel = channels_fchannel(*channels);
    channel_set_rgb8(&mut channel, r, g, b);
    *channels = (channel as u64) << 32 | *channels & 0xffffffff_u64;
    *channels
}

/// Sets the three background RGB [NcColor]s of an [NcChannelPair], and
/// marks it as NOT using the "default color", retaining the other bits unchanged.
///
/// Unlike the original C API, it also returns the new NcChannelPair.
///
/// *Method: NcChannelPair.[set_bg_rgb8()][NcChannelPair#method.set_bg_rgb8]*
#[inline]
pub fn channels_set_bg_rgb8(
    channels: &mut NcChannelPair,
    r: NcColor,
    g: NcColor,
    b: NcColor,
) -> NcChannelPair {
    let mut channel = channels_bchannel(*channels);
    channel_set_rgb8(&mut channel, r, g, b);
    channels_set_bchannel(channels, channel);
    *channels
}

// NcRgb -----------------------------------------------------------------------

/// Gets the foreground [NcRgb] from an [NcChannelPair], shifted to LSBs.
///
/// *Method: NcChannelPair.[fg_rgb()][NcChannelPair#method.fg_rgb]*
#[inline]
pub fn channels_fg_rgb(channels: NcChannelPair) -> NcRgb {
    channels_fchannel(channels) & NCCELL_BG_RGB_MASK
}

/// Gets the background [NcRgb] from an [NcChannelPair], shifted to LSBs.
///
/// *Method: NcChannelPair.[bg_rgb()][NcChannelPair#method.bg_rgb]*
#[inline]
pub fn channels_bg_rgb(channels: NcChannelPair) -> NcRgb {
    channels_bchannel(channels) & NCCELL_BG_RGB_MASK
}

/// Gets the [NcRgb] of an [NcChannel].
///
/// This function basically removes the 4th byte of the NcChannel.
///
/// *Method: NcChannel.[rgb()][NcChannel#method.rgb]*
//
// Not in the C API
#[inline]
pub const fn channel_rgb(channel: NcChannel) -> NcRgb {
    channel & NCCELL_BG_RGB_MASK
}

/// Sets the [NcRgb] of an [NcChannel], and marks it as NOT using the
/// "default color", retaining the other bits unchanged.
///
/// *Method: NcChannel.[set()][NcChannel#method.set]*
#[inline]
pub fn channel_set(channel: &mut NcChannel, rgb: NcRgb) {
    *channel = (*channel & !NCCELL_BG_RGB_MASK) | NCCELL_BGDEFAULT_MASK | (rgb & 0x00ffffff);
}

/// Sets the foreground [NcRgb] of an [NcChannelPair], and marks it as NOT using
/// the "default color", retaining the other bits unchanged.
///
/// *Method: NcChannelPair.[set_fg_rgb()][NcChannelPair#method.set_fg_rgb]*
#[inline]
pub fn channels_set_fg_rgb(channels: &mut NcChannelPair, rgb: NcRgb) {
    let mut channel = channels_fchannel(*channels);
    channel_set(&mut channel, rgb);
    *channels = (channel as u64) << 32 | *channels & 0xffffffff_u64;
}

/// Sets the foreground [NcRgb] of an [NcChannelPair], and marks it as NOT using
/// the "default color", retaining the other bits unchanged.
///
/// *Method: NcChannelPair.[set_bg_rgb()][NcChannelPair#method.set_bg_rgb]*
#[inline]
pub fn channels_set_bg_rgb(channels: &mut NcChannelPair, rgb: NcRgb) {
    let mut channel = channels_bchannel(*channels);
    channel_set(&mut channel, rgb);
    channels_set_bchannel(channels, channel);
}

// Default ---------------------------------------------------------------------

/// Is this [NcChannel] using the "default color" rather than RGB/palette-indexed?
///
/// *Method: NcChannel.[default_p()][NcChannel#method.default_p]*
#[inline]
pub const fn channel_default_p(channel: NcChannel) -> bool {
    (channel & NCCELL_BGDEFAULT_MASK) == 0
}

/// Marks an [NcChannel] as using its "default color", which also marks it opaque.
///
/// *Method: NcChannel.[set_default()][NcChannel#method.set_default]*
#[inline]
pub fn channel_set_default(channel: &mut NcChannel) -> NcChannel {
    *channel &= !(NCCELL_BGDEFAULT_MASK | NCCELL_ALPHA_HIGHCONTRAST);
    *channel
}

/// Marks an [NcChannel] as NOT using its "default color",
/// retaining the other bits unchanged.
///
/// *Method: NcChannel.[set_not_default()][NcChannel#method.set_not_default]*
//
// Not in the C API
#[inline]
pub fn channel_set_not_default(channel: &mut NcChannel) -> NcChannel {
    *channel |= NCCELL_BGDEFAULT_MASK;
    *channel
}

/// Is the foreground of an [NcChannelPair] using the "default foreground color"?
///
/// *Method: NcChannelPair.[fg_default_p()][NcChannelPair#method.fg_default_p]*
#[inline]
pub fn channels_fg_default_p(channels: NcChannelPair) -> bool {
    channel_default_p(channels_fchannel(channels))
}

/// Is the background using the "default background color"?
///
/// The "default background color" must generally be used to take advantage of
/// terminal-effected transparency.
///
/// *Method: NcChannelPair.[bg_default_p()][NcChannelPair#method.bg_default_p]*
#[inline]
pub fn channels_bg_default_p(channels: NcChannelPair) -> bool {
    channel_default_p(channels_bchannel(channels))
}

/// Marks the foreground of an [NcChannelPair] as using its "default color",
/// which also marks it opaque, and returns the new [NcChannelPair].
///
/// *Method: NcChannelPair.[set_fg_default()][NcChannelPair#method.set_fg_default]*
#[inline]
pub fn channels_set_fg_default(channels: &mut NcChannelPair) -> NcChannelPair {
    let mut channel = channels_fchannel(*channels);
    channel_set_default(&mut channel);
    *channels = (channel as u64) << 32 | *channels & 0xffffffff_u64;
    *channels
}

/// Marks the foreground of an [NcChannelPair] as NOT using its "default color",
/// retaining the other bits unchanged, and returns the new [NcChannelPair].
///
/// *Method: NcChannelPair.[set_fg_not_default()][NcChannelPair#method.set_fg_not_default]*
//
// Not in the C API
#[inline]
pub fn channels_set_fg_not_default(channels: &mut NcChannelPair) -> NcChannelPair {
    let mut channel = channels_fchannel(*channels);
    channel_set_not_default(&mut channel);
    *channels = (channel as u64) << 32 | *channels & 0xffffffff_u64;
    *channels
}

/// Marks the background of an [NcChannelPair] as using its "default color",
/// which also marks it opaque, and returns the new [NcChannelPair].
///
/// *Method: NcChannelPair.[set_bg_default()][NcChannelPair#method.set_bg_default]*
#[inline]
pub fn channels_set_bg_default(channels: &mut NcChannelPair) -> NcChannelPair {
    let mut channel = channels_bchannel(*channels);
    channel_set_default(&mut channel);
    channels_set_bchannel(channels, channel);
    *channels
}

/// Marks the background of an [NcChannelPair] as NOT using its "default color",
/// retaining the other bits unchanged, and returns the new [NcChannelPair].
///
/// *Method: NcChannelPair.[set_bg_not_default()][NcChannelPair#method.set_bg_not_default]*
//
// Not in the C API
#[inline]
pub fn channels_set_bg_not_default(channels: &mut NcChannelPair) -> NcChannelPair {
    let mut channel = channels_bchannel(*channels);
    channel_set_not_default(&mut channel);
    channels_set_bchannel(channels, channel);
    *channels
}

/// Marks both the foreground and background of an [NcChannelPair] as using their
/// "default color", which also marks them opaque, and returns the new [NcChannelPair].
///
/// *Method: NcChannelPair.[set_default()][NcChannelPair#method.set_default]*
//
// Not in the C API
#[inline]
pub fn channels_set_default(channels: &mut NcChannelPair) -> NcChannelPair {
    let mut channels = channels_set_fg_default(channels);
    let channels = channels_set_bg_default(&mut channels);
    channels
}

/// Marks both the foreground and background of an [NcChannelPair] as NOT using their
/// "default color", retaining the other bits unchanged, and returns the new [NcChannelPair].
///
/// *Method: NcChannelPair.[set_not_default()][NcChannelPair#method.set_not_default]*
//
// Not in the C API
#[inline]
pub fn channels_set_not_default(channels: &mut NcChannelPair) -> NcChannelPair {
    let mut channels = channels_set_fg_not_default(channels);
    let channels = channels_set_bg_not_default(&mut channels);
    channels
}

// Palette ---------------------------------------------------------------------

/// Is this [NcChannel] using palette-indexed color rather than RGB?
///
/// *Method: NcChannel.[palindex_p()][NcChannel#method.palindex_p]*
#[inline]
pub fn channel_palindex_p(channel: NcChannel) -> bool {
    !(channel_default_p(channel) && (channel & NCCELL_BG_PALETTE) == 0)
}

/// Is the foreground of an [NcChannelPair] using an [indexed][NcPaletteIndex]
/// [NcPalette][crate::NcPalette] color?
///
/// *Method: NcChannelPair.[fg_palindex_p()][NcChannelPair#method.fg_palindex_p]*
#[inline]
pub fn channels_fg_palindex_p(channels: NcChannelPair) -> bool {
    channel_palindex_p(channels_fchannel(channels))
}

/// Is the background of an [NcChannelPair] using an [indexed][NcPaletteIndex]
/// [NcPalette][crate::NcPalette] color?
///
/// *Method: NcChannelPair.[bg_palindex_p()][NcChannelPair#method.bg_palindex_p]*
#[inline]
pub fn channels_bg_palindex_p(channels: NcChannelPair) -> bool {
    channel_palindex_p(channels_bchannel(channels))
}

/// Sets the foreground of an [NcChannelPair] as using an
/// [indexed][NcPaletteIndex] [NcPalette][crate::NcPalette] color.
///
/// *Method: NcChannelPair.[set_fg_palindex()][NcChannelPair#method.set_fg_palindex]*
#[inline]
pub fn channels_set_fg_palindex(channels: &mut NcChannelPair, index: NcPaletteIndex) {
    *channels |= NCCELL_FGDEFAULT_MASK;
    *channels |= NCCELL_FG_PALETTE as NcChannelPair;
    channels_set_fg_alpha(channels, NCCELL_ALPHA_OPAQUE);
    *channels &= 0xff000000ffffffff as NcChannelPair;
    *channels |= (index as NcChannelPair) << 32;
}

/// Sets the background of an [NcChannelPair] as using an
/// [indexed][NcPaletteIndex] [NcPalette][crate::NcPalette] color.
///
/// *Method: NcChannelPair.[set_bg_palindex()][NcChannelPair#method.set_bg_palindex]*
#[inline]
pub fn channels_set_bg_palindex(channels: &mut NcChannelPair, index: NcPaletteIndex) {
    *channels |= NCCELL_BGDEFAULT_MASK as NcChannelPair;
    *channels |= NCCELL_BG_PALETTE as NcChannelPair;
    channels_set_bg_alpha(channels, NCCELL_ALPHA_OPAQUE);
    *channels &= 0xffffffffff000000;
    *channels |= index as NcChannelPair;
}
