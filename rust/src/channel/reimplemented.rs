//! `channel*_*` reimplemented functions.

use crate::{
    NcAlphaBits, NcChannel, NcChannelPair, NcComponent, NcPaletteIndex, NcRgb,
    NCALPHA_BGDEFAULT_MASK, NCALPHA_BG_PALETTE, NCALPHA_BG_RGB_MASK, NCALPHA_FGDEFAULT_MASK,
    NCALPHA_FG_PALETTE, NCALPHA_HIGHCONTRAST, NCALPHA_OPAQUE, NCCHANNEL_ALPHA_MASK,
};

// Alpha -----------------------------------------------------------------------

/// Gets the [`NcAlphaBits`] from an [`NcChannel`].
///
/// *Method: NcChannel.[alpha()][NcChannel#method.alpha]*
#[inline]
pub const fn ncchannel_alpha(channel: NcChannel) -> NcAlphaBits {
    channel & NCCHANNEL_ALPHA_MASK
}

/// Sets the [`NcAlphaBits`] of an [`NcChannel`].
///
/// *Method: NcChannel.[set_alpha()][NcChannel#method.set_alpha]*
#[inline]
pub fn ncchannel_set_alpha(channel: &mut NcChannel, alpha: NcAlphaBits) {
    let alpha_clean = alpha & NCCHANNEL_ALPHA_MASK;
    *channel = alpha_clean | (*channel & !NCCHANNEL_ALPHA_MASK);

    if alpha != NCALPHA_OPAQUE {
        // indicate that we are *not* using the default background color
        *channel |= NCALPHA_BGDEFAULT_MASK;
    }
}

/// Gets the foreground [`NcAlphaBits`] from an [`NcChannelPair`], shifted to LSBs.
///
/// *Method: NcChannelPair.[fg_alpha()][NcChannelPair#method.fg_alpha]*
#[inline]
pub const fn ncchannels_fg_alpha(channels: NcChannelPair) -> NcAlphaBits {
    ncchannel_alpha(ncchannels_fchannel(channels))
}

/// Gets the background [`NcAlphaBits`] from an [`NcChannelPair`], shifted to LSBs.
///
/// *Method: NcChannelPair.[bg_alpha()][NcChannelPair#method.bg_alpha]*
#[inline]
pub const fn ncchannels_bg_alpha(channels: NcChannelPair) -> NcAlphaBits {
    ncchannel_alpha(ncchannels_bchannel(channels))
}

/// Sets the [`NcAlphaBits`] of the foreground [`NcChannel`] of an [`NcChannelPair`].
///
/// *Method: NcChannelPair.[set_fg_alpha()][NcChannelPair#method.set_fg_alpha]*
#[inline]
pub fn ncchannels_set_fg_alpha(channels: &mut NcChannelPair, alpha: NcAlphaBits) {
    let mut channel = ncchannels_fchannel(*channels);
    ncchannel_set_alpha(&mut channel, alpha);
    *channels = (channel as NcChannelPair) << 32 | *channels & 0xffffffff_u64;
}

/// Sets the [`NcAlphaBits`] of the background [`NcChannel`] of an [`NcChannelPair`].
///
/// *Method: NcChannelPair.[set_bg_alpha()][NcChannelPair#method.set_bg_alpha]*
#[inline]
pub fn ncchannels_set_bg_alpha(channels: &mut NcChannelPair, alpha: NcAlphaBits) {
    let mut alpha_clean = alpha;
    if alpha == NCALPHA_HIGHCONTRAST {
        // forbidden for background alpha, so makes it opaque
        alpha_clean = NCALPHA_OPAQUE;
    }
    let mut channel = ncchannels_bchannel(*channels);
    ncchannel_set_alpha(&mut channel, alpha_clean);
    ncchannels_set_bchannel(channels, channel);
}

// Channels --------------------------------------------------------------------

/// Extracts the background [`NcChannel`] from a [`NcChannelPair`].
///
/// *Method: NcChannelPair.[bchannel()][NcChannelPair#method.bchannel]*
#[inline]
pub const fn ncchannels_bchannel(channels: NcChannelPair) -> NcChannel {
    (channels & 0xffffffff_u64) as NcChannel
}

/// Extracts the foreground [`NcChannel`] from an [`NcChannelPair`].
///
/// *Method: NcChannelPair.[fchannel()][NcChannelPair#method.fchannel]*
#[inline]
pub const fn ncchannels_fchannel(channels: NcChannelPair) -> NcChannel {
    ncchannels_bchannel(channels >> 32)
}

/// Sets the background [`NcChannel`] of an [`NcChannelPair`].
///
/// *Method: NcChannelPair.[set_bchannel()][NcChannelPair#method.set_bchannel]*
#[inline]
pub fn ncchannels_set_bchannel(channels: &mut NcChannelPair, bchannel: NcChannel) -> NcChannelPair {
    *channels = (*channels & 0xffffffff00000000_u64) | bchannel as u64;
    *channels
}

/// Sets the foreground [`NcChannel`] of an [`NcChannelPair`].
///
/// *Method: NcChannelPair.[set_fchannel()][NcChannelPair#method.set_fchannel]*
#[inline]
pub fn ncchannels_set_fchannel(channels: &mut NcChannelPair, fchannel: NcChannel) -> NcChannelPair {
    *channels = (*channels & 0xffffffff_u64) | (fchannel as u64) << 32;
    *channels
}

/// Combines two [`NcChannel`]s into an [`NcChannelPair`].
///
/// *Method: NcChannelPair.[combine()][NcChannelPair#method.combine]*
#[inline]
pub fn ncchannels_combine(fchannel: NcChannel, bchannel: NcChannel) -> NcChannelPair {
    let mut channels: NcChannelPair = 0;
    ncchannels_set_fchannel(&mut channels, fchannel);
    ncchannels_set_bchannel(&mut channels, bchannel);
    channels
}

// NcComponent ---------------------------------------------------------------------

/// Gets the red [`NcComponent`] from an [`NcChannel`].
///
/// *Method: NcChannel.[r()][NcChannel#method.r]*
#[inline]
pub const fn ncchannel_r(channel: NcChannel) -> NcComponent {
    ((channel & 0xff0000) >> 16) as NcComponent
}

/// Gets the green [`NcComponent`] from an [`NcChannel`].
///
/// *Method: NcChannel.[g()][NcChannel#method.g]*
#[inline]
pub const fn ncchannel_g(channel: NcChannel) -> NcComponent {
    ((channel & 0x00ff00) >> 8) as NcComponent
}

/// Gets the blue [`NcComponent`] from an [`NcChannel`].
///
/// *Method: NcChannel.[b()][NcChannel#method.b]*
#[inline]
pub const fn ncchannel_b(channel: NcChannel) -> NcComponent {
    (channel & 0x0000ff) as NcComponent
}

/// Sets the red [`NcComponent`] of an [`NcChannel`], and returns it.
///
/// *Method: NcChannel.[set_r()][NcChannel#method.set_r]*
//
// Not in the C API.
#[inline]
pub fn ncchannel_set_r(channel: &mut NcChannel, r: NcComponent) -> NcChannel {
    *channel = (r as NcChannel) << 16 | (*channel & 0xff00) | (*channel & 0xff);
    *channel
}

/// Sets the green [`NcComponent`] of an [`NcChannel`], and returns it.
///
/// *Method: NcChannel.[set_g()][NcChannel#method.set_g]*
//
// Not in the C API.
#[inline]
pub fn ncchannel_set_g(channel: &mut NcChannel, g: NcComponent) -> NcChannel {
    *channel = (*channel & 0xff0000) | (g as NcChannel) << 8 | (*channel & 0xff);
    *channel
}

/// Sets the blue [`NcComponent`] of an [`NcChannel`], and returns it.
///
/// *Method: NcChannel.[set_b()][NcChannel#method.set_b]*
//
// Not in the C API.
#[inline]
pub fn ncchannel_set_b(channel: &mut NcChannel, b: NcComponent) -> NcChannel {
    *channel = (*channel & 0xff0000) | (*channel & 0xff00) | (b as NcChannel);
    *channel
}

/// Gets the three RGB [`NcComponent`]s from an [`NcChannel`], and returns it.
///
/// *Method: NcChannel.[rgb8()][NcChannel#method.rgb8]*
#[inline]
pub fn ncchannel_rgb8(
    channel: NcChannel,
    r: &mut NcComponent,
    g: &mut NcComponent,
    b: &mut NcComponent,
) -> NcChannel {
    *r = ncchannel_r(channel);
    *g = ncchannel_g(channel);
    *b = ncchannel_b(channel);
    channel
}

/// Sets the three RGB [`NcComponent`]s an [`NcChannel`], and marks it as NOT using the
/// "default color", retaining the other bits unchanged.
///
/// *Method: NcChannel.[set_rgb8()][NcChannel#method.set_rgb8]*
#[inline]
pub fn ncchannel_set_rgb8(channel: &mut NcChannel, r: NcComponent, g: NcComponent, b: NcComponent) {
    let rgb: NcRgb = (r as NcChannel) << 16 | (g as NcChannel) << 8 | (b as NcChannel);
    *channel = (*channel & !NCALPHA_BG_RGB_MASK) | NCALPHA_BGDEFAULT_MASK | rgb;
}

/// Gets the three foreground RGB [`NcComponent`]s from an [`NcChannelPair`], and
/// returns the foreground [`NcChannel`] (which can have some extra bits set).
///
/// *Method: NcChannelPair.[fg_rgb8()][NcChannelPair#method.fg_rgb8]*
#[inline]
pub fn ncchannels_fg_rgb8(
    channels: NcChannelPair,
    r: &mut NcComponent,
    g: &mut NcComponent,
    b: &mut NcComponent,
) -> NcChannel {
    ncchannel_rgb8(ncchannels_fchannel(channels), r, g, b)
}

/// Gets the three background RGB [`NcComponent`]s from an [`NcChannelPair`], and
/// returns the background [`NcChannel`] (which can have some extra bits set).
///
/// *Method: NcChannelPair.[bg_rgb8()][NcChannelPair#method.bg_rgb8]*
#[inline]
pub fn ncchannels_bg_rgb8(
    channels: NcChannelPair,
    r: &mut NcComponent,
    g: &mut NcComponent,
    b: &mut NcComponent,
) -> NcChannel {
    ncchannel_rgb8(ncchannels_bchannel(channels), r, g, b)
}

/// Sets the three foreground RGB [`NcComponent`]s of an [`NcChannelPair`], and
/// marks it as NOT using the "default color", retaining the other bits unchanged.
///
/// Unlike the original C API, it also returns the new NcChannelPair.
///
/// *Method: NcChannelPair.[set_fg_rgb8()][NcChannelPair#method.set_fg_rgb8]*
#[inline]
pub fn ncchannels_set_fg_rgb8(
    channels: &mut NcChannelPair,
    r: NcComponent,
    g: NcComponent,
    b: NcComponent,
) -> NcChannelPair {
    let mut channel = ncchannels_fchannel(*channels);
    ncchannel_set_rgb8(&mut channel, r, g, b);
    *channels = (channel as u64) << 32 | *channels & 0xffffffff_u64;
    *channels
}

/// Sets the three background RGB [`NcComponent`]s of an [`NcChannelPair`], and
/// marks it as NOT using the "default color", retaining the other bits unchanged.
///
/// Unlike the original C API, it also returns the new NcChannelPair.
///
/// *Method: NcChannelPair.[set_bg_rgb8()][NcChannelPair#method.set_bg_rgb8]*
#[inline]
pub fn ncchannels_set_bg_rgb8(
    channels: &mut NcChannelPair,
    r: NcComponent,
    g: NcComponent,
    b: NcComponent,
) -> NcChannelPair {
    let mut channel = ncchannels_bchannel(*channels);
    ncchannel_set_rgb8(&mut channel, r, g, b);
    ncchannels_set_bchannel(channels, channel);
    *channels
}

// NcRgb -----------------------------------------------------------------------

/// Gets the foreground [`NcRgb`] from an [`NcChannelPair`], shifted to LSBs.
///
/// *Method: NcChannelPair.[fg_rgb()][NcChannelPair#method.fg_rgb]*
#[inline]
pub fn ncchannels_fg_rgb(channels: NcChannelPair) -> NcRgb {
    ncchannels_fchannel(channels) & NCALPHA_BG_RGB_MASK
}

/// Gets the background [`NcRgb`] from an [`NcChannelPair`], shifted to LSBs.
///
/// *Method: NcChannelPair.[bg_rgb()][NcChannelPair#method.bg_rgb]*
#[inline]
pub fn ncchannels_bg_rgb(channels: NcChannelPair) -> NcRgb {
    ncchannels_bchannel(channels) & NCALPHA_BG_RGB_MASK
}

/// Gets the [`NcRgb`] of an [`NcChannel`].
///
/// This function basically removes the 4th byte of the NcChannel.
///
/// *Method: NcChannel.[rgb()][NcChannel#method.rgb]*
//
// Not in the C API
#[inline]
pub const fn ncchannel_rgb(channel: NcChannel) -> NcRgb {
    channel & NCALPHA_BG_RGB_MASK
}

/// Sets the [`NcRgb`] of an [`NcChannel`], and marks it as NOT using the
/// "default color", retaining the other bits unchanged.
///
/// *Method: NcChannel.[set()][NcChannel#method.set]*
#[inline]
pub fn ncchannel_set(channel: &mut NcChannel, rgb: NcRgb) {
    *channel = (*channel & !NCALPHA_BG_RGB_MASK) | NCALPHA_BGDEFAULT_MASK | (rgb & 0x00ffffff);
}

/// Sets the foreground [`NcRgb`] of an [`NcChannelPair`], and marks it as NOT using
/// the "default color", retaining the other bits unchanged.
///
/// *Method: NcChannelPair.[set_fg_rgb()][NcChannelPair#method.set_fg_rgb]*
#[inline]
pub fn ncchannels_set_fg_rgb(channels: &mut NcChannelPair, rgb: NcRgb) {
    let mut channel = ncchannels_fchannel(*channels);
    ncchannel_set(&mut channel, rgb);
    *channels = (channel as u64) << 32 | *channels & 0xffffffff_u64;
}

/// Sets the foreground [`NcRgb`] of an [`NcChannelPair`], and marks it as NOT using
/// the "default color", retaining the other bits unchanged.
///
/// *Method: NcChannelPair.[set_bg_rgb()][NcChannelPair#method.set_bg_rgb]*
#[inline]
pub fn ncchannels_set_bg_rgb(channels: &mut NcChannelPair, rgb: NcRgb) {
    let mut channel = ncchannels_bchannel(*channels);
    ncchannel_set(&mut channel, rgb);
    ncchannels_set_bchannel(channels, channel);
}

// Default ---------------------------------------------------------------------

/// Is this [`NcChannel`] using the "default color" rather than RGB/palette-indexed?
///
/// *Method: NcChannel.[default_p()][NcChannel#method.default_p]*
#[inline]
pub const fn ncchannel_default_p(channel: NcChannel) -> bool {
    (channel & NCALPHA_BGDEFAULT_MASK) == 0
}

/// Marks an [`NcChannel`] as using its "default color", which also marks it opaque.
///
/// *Method: NcChannel.[set_default()][NcChannel#method.set_default]*
#[inline]
pub fn ncchannel_set_default(channel: &mut NcChannel) -> NcChannel {
    *channel &= !(NCALPHA_BGDEFAULT_MASK | NCALPHA_HIGHCONTRAST);
    *channel
}

/// Marks an [`NcChannel`] as NOT using its "default color",
/// retaining the other bits unchanged.
///
/// *Method: NcChannel.[set_not_default()][NcChannel#method.set_not_default]*
//
// Not in the C API
#[inline]
pub fn ncchannel_set_not_default(channel: &mut NcChannel) -> NcChannel {
    *channel |= NCALPHA_BGDEFAULT_MASK;
    *channel
}

/// Is the foreground of an [`NcChannelPair`] using the "default foreground color"?
///
/// *Method: NcChannelPair.[fg_default_p()][NcChannelPair#method.fg_default_p]*
#[inline]
pub fn ncchannels_fg_default_p(channels: NcChannelPair) -> bool {
    ncchannel_default_p(ncchannels_fchannel(channels))
}

/// Is the background using the "default background color"?
///
/// The "default background color" must generally be used to take advantage of
/// terminal-effected transparency.
///
/// *Method: NcChannelPair.[bg_default_p()][NcChannelPair#method.bg_default_p]*
#[inline]
pub fn ncchannels_bg_default_p(channels: NcChannelPair) -> bool {
    ncchannel_default_p(ncchannels_bchannel(channels))
}

/// Marks the foreground of an [`NcChannelPair`] as using its "default color",
/// which also marks it opaque, and returns the new [`NcChannelPair`].
///
/// *Method: NcChannelPair.[set_fg_default()][NcChannelPair#method.set_fg_default]*
#[inline]
pub fn ncchannels_set_fg_default(channels: &mut NcChannelPair) -> NcChannelPair {
    let mut channel = ncchannels_fchannel(*channels);
    ncchannel_set_default(&mut channel);
    *channels = (channel as u64) << 32 | *channels & 0xffffffff_u64;
    *channels
}

/// Marks the foreground of an [`NcChannelPair`] as NOT using its "default color",
/// retaining the other bits unchanged, and returns the new [`NcChannelPair`].
///
/// *Method: NcChannelPair.[set_fg_not_default()][NcChannelPair#method.set_fg_not_default]*
//
// Not in the C API
#[inline]
pub fn ncchannels_set_fg_not_default(channels: &mut NcChannelPair) -> NcChannelPair {
    let mut channel = ncchannels_fchannel(*channels);
    ncchannel_set_not_default(&mut channel);
    *channels = (channel as u64) << 32 | *channels & 0xffffffff_u64;
    *channels
}

/// Marks the background of an [`NcChannelPair`] as using its "default color",
/// which also marks it opaque, and returns the new [`NcChannelPair`].
///
/// *Method: NcChannelPair.[set_bg_default()][NcChannelPair#method.set_bg_default]*
#[inline]
pub fn ncchannels_set_bg_default(channels: &mut NcChannelPair) -> NcChannelPair {
    let mut channel = ncchannels_bchannel(*channels);
    ncchannel_set_default(&mut channel);
    ncchannels_set_bchannel(channels, channel);
    *channels
}

/// Marks the background of an [`NcChannelPair`] as NOT using its "default color",
/// retaining the other bits unchanged, and returns the new [`NcChannelPair`].
///
/// *Method: NcChannelPair.[set_bg_not_default()][NcChannelPair#method.set_bg_not_default]*
//
// Not in the C API
#[inline]
pub fn ncchannels_set_bg_not_default(channels: &mut NcChannelPair) -> NcChannelPair {
    let mut channel = ncchannels_bchannel(*channels);
    ncchannel_set_not_default(&mut channel);
    ncchannels_set_bchannel(channels, channel);
    *channels
}

/// Marks both the foreground and background of an [`NcChannelPair`] as using their
/// "default color", which also marks them opaque, and returns the new [`NcChannelPair`].
///
/// *Method: NcChannelPair.[set_default()][NcChannelPair#method.set_default]*
//
// Not in the C API
#[inline]
pub fn ncchannels_set_default(channels: &mut NcChannelPair) -> NcChannelPair {
    ncchannels_set_bg_default(&mut ncchannels_set_fg_default(channels))
}

/// Marks both the foreground and background of an [`NcChannelPair`] as NOT using their
/// "default color", retaining the other bits unchanged, and returns the new [`NcChannelPair`].
///
/// *Method: NcChannelPair.[set_not_default()][NcChannelPair#method.set_not_default]*
//
// Not in the C API
#[inline]
pub fn ncchannels_set_not_default(channels: &mut NcChannelPair) -> NcChannelPair {
    ncchannels_set_bg_not_default(&mut ncchannels_set_fg_not_default(channels))
}

// Palette ---------------------------------------------------------------------

/// Is this [`NcChannel`] using palette-indexed color rather than RGB?
///
/// *Method: NcChannel.[palindex_p()][NcChannel#method.palindex_p]*
#[inline]
pub fn ncchannel_palindex_p(channel: NcChannel) -> bool {
    !(ncchannel_default_p(channel) && (channel & NCALPHA_BG_PALETTE) == 0)
}

/// Is the foreground of an [`NcChannelPair`] using an [indexed][`NcPaletteIndex`]
/// [`NcPalette`][crate::NcPalette] color?
///
/// *Method: NcChannelPair.[fg_palindex_p()][NcChannelPair#method.fg_palindex_p]*
#[inline]
pub fn ncchannels_fg_palindex_p(channels: NcChannelPair) -> bool {
    ncchannel_palindex_p(ncchannels_fchannel(channels))
}

/// Is the background of an [`NcChannelPair`] using an [indexed][`NcPaletteIndex`]
/// [`NcPalette`][crate::NcPalette] color?
///
/// *Method: NcChannelPair.[bg_palindex_p()][NcChannelPair#method.bg_palindex_p]*
#[inline]
pub fn ncchannels_bg_palindex_p(channels: NcChannelPair) -> bool {
    ncchannel_palindex_p(ncchannels_bchannel(channels))
}

/// Sets the foreground of an [`NcChannelPair`] as using an
/// [indexed][`NcPaletteIndex`] [`NcPalette`][crate::NcPalette] color.
///
/// *Method: NcChannelPair.[set_fg_palindex()][NcChannelPair#method.set_fg_palindex]*
#[inline]
#[allow(clippy::unnecessary_cast)]
pub fn ncchannels_set_fg_palindex(channels: &mut NcChannelPair, index: NcPaletteIndex) {
    *channels |= NCALPHA_FGDEFAULT_MASK;
    *channels |= NCALPHA_FG_PALETTE as NcChannelPair;
    ncchannels_set_fg_alpha(channels, NCALPHA_OPAQUE);
    *channels &= 0xff000000ffffffff as NcChannelPair;
    *channels |= (index as NcChannelPair) << 32;
}

/// Sets the background of an [`NcChannelPair`] as using an
/// [indexed][`NcPaletteIndex`] [`NcPalette`][crate::NcPalette] color.
///
/// *Method: NcChannelPair.[set_bg_palindex()][NcChannelPair#method.set_bg_palindex]*
#[inline]
pub fn ncchannels_set_bg_palindex(channels: &mut NcChannelPair, index: NcPaletteIndex) {
    *channels |= NCALPHA_BGDEFAULT_MASK as NcChannelPair;
    *channels |= NCALPHA_BG_PALETTE as NcChannelPair;
    ncchannels_set_bg_alpha(channels, NCALPHA_OPAQUE);
    *channels &= 0xffffffffff000000;
    *channels |= index as NcChannelPair;
}
