//! [`NcChannel`] & [`NcChannels`] `channel*_*` static_function reinmplementations

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

use crate::types::{
    NcAlphaBits, NcChannel, NcChannels, NcColor, NcPaletteIndex, NcRgb, NCCELL_ALPHA_HIGHCONTRAST,
    NCCELL_ALPHA_OPAQUE, NCCELL_BGDEFAULT_MASK, NCCELL_BG_PALETTE, NCCELL_BG_RGB_MASK,
    NCCELL_FGDEFAULT_MASK, NCCELL_FG_PALETTE, NCCHANNEL_ALPHA_MASK,
};

/// Extract the 8-bit red component from a 32-bit channel.
#[inline]
pub fn channel_r(channel: NcChannel) -> NcColor {
    ((channel & 0xff0000) >> 16) as NcColor
}

/// Extract the 8-bit green component from a 32-bit channel.
#[inline]
pub fn channel_g(channel: NcChannel) -> NcColor {
    ((channel & 0x00ff00) >> 8) as NcColor
}

/// Extract the 8-bit blue component from a 32-bit channel.
#[inline]
pub fn channel_b(channel: NcChannel) -> NcColor {
    (channel & 0x0000ff) as NcColor
}

/// Extract the three 8-bit R/G/B components from a 32-bit channel.
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

/// Set the three 8-bit components of a 32-bit channel, and mark it as not using
/// the default color. Retain the other bits unchanged.
#[inline]
pub fn channel_set_rgb8(channel: &mut NcChannel, r: NcColor, g: NcColor, b: NcColor) {
    let rgb: NcRgb = (r as NcChannel) << 16 | (g as NcChannel) << 8 | (b as NcChannel);
    *channel = (*channel & !NCCELL_BG_RGB_MASK) | NCCELL_BGDEFAULT_MASK | rgb;
}

/// Same as channel_set_rgb8(), but provide an assembled, packed 24 bits of rgb.
#[inline]
pub fn channel_set(channel: &mut NcChannel, rgb: NcRgb) {
    *channel = (*channel & !NCCELL_BG_RGB_MASK) | NCCELL_BGDEFAULT_MASK | (rgb & 0x00ffffff);
}

/// Extract the 2-bit alpha component from a 32-bit channel.
#[inline]
pub fn channel_alpha(channel: NcChannel) -> NcAlphaBits {
    channel & NCCHANNEL_ALPHA_MASK
}

/// Set the 2-bit alpha component of the 32-bit channel.
#[inline]
pub fn channel_set_alpha(channel: &mut NcChannel, alpha: NcAlphaBits) {
    let alpha_clean = alpha & NCCHANNEL_ALPHA_MASK;
    *channel = alpha_clean | (*channel & !NCCHANNEL_ALPHA_MASK);

    if alpha != NCCELL_ALPHA_OPAQUE {
        // indicate that we are *not* using the default background color
        *channel |= NCCELL_BGDEFAULT_MASK;
    }
}

/// Is this channel using the "default color" rather than RGB/palette-indexed?
#[inline]
pub fn channel_default_p(channel: NcChannel) -> bool {
    (channel & NCCELL_BGDEFAULT_MASK) == 0
}

/// Is this channel using palette-indexed color rather than RGB?
#[inline]
pub fn channel_palindex_p(channel: NcChannel) -> bool {
    !(channel_default_p(channel) && (channel & NCCELL_BG_PALETTE) == 0)
}

/// Mark the channel as using its default color, which also marks it opaque.
#[inline]
pub fn channel_set_default(channel: &mut NcChannel) -> NcChannel {
    *channel &= !(NCCELL_BGDEFAULT_MASK | NCCELL_ALPHA_HIGHCONTRAST);
    *channel
}

/// Extract the 32-bit background channel from a channel pair.
#[inline]
pub fn channels_bchannel(channels: NcChannels) -> NcChannel {
    (channels & 0xffffffff_u64) as NcChannel
}

/// Extract the 32-bit foreground channel from a channel pair.
#[inline]
pub fn channels_fchannel(channels: NcChannels) -> NcChannel {
    channels_bchannel(channels >> 32)
}

/// Set the 32-bit background channel of a channel pair.
#[inline]
pub fn channels_set_bchannel(channels: &mut NcChannels, bchannel: NcChannel) -> NcChannels {
    *channels = (*channels & 0xffffffff00000000_u64) | bchannel as u64;
    *channels
}

/// Set the 32-bit foreground channel of a channel pair.
#[inline]
pub fn channels_set_fchannel(channels: &mut NcChannels, fchannel: NcChannel) -> NcChannels {
    *channels = (*channels & 0xffffffff_u64) | (fchannel as u64) << 32;
    *channels
}

/// Combine two channels into a channel pair.
#[inline]
pub fn channels_combine(fchannel: NcChannel, bchannel: NcChannel) -> NcChannels {
    let mut channels: NcChannels = 0;
    channels_set_fchannel(&mut channels, fchannel);
    channels_set_bchannel(&mut channels, bchannel);
    channels
}

/// Extract 24 bits of foreground RGB from 'channels', shifted to LSBs.
#[inline]
pub fn channels_fg_rgb(channels: NcChannels) -> NcChannel {
    channels_fchannel(channels) & NCCELL_BG_RGB_MASK
}

/// Extract 24 bits of background RGB from 'channels', shifted to LSBs.
#[inline]
pub fn channels_bg_rgb(channels: NcChannels) -> NcChannel {
    channels_bchannel(channels) & NCCELL_BG_RGB_MASK
}

/// Extract 2 bits of foreground alpha from 'channels', shifted to LSBs.
#[inline]
pub fn channels_fg_alpha(channels: NcChannels) -> NcAlphaBits {
    channel_alpha(channels_fchannel(channels))
}

/// Extract 2 bits of background alpha from 'channels', shifted to LSBs.
#[inline]
pub fn channels_bg_alpha(channels: NcChannels) -> NcAlphaBits {
    channel_alpha(channels_bchannel(channels))
}

/// Extract 24 bits of foreground RGB from 'channels', split into subchannels.
#[inline]
pub fn channels_fg_rgb8(
    channels: NcChannels,
    r: &mut NcColor,
    g: &mut NcColor,
    b: &mut NcColor,
) -> NcChannel {
    channel_rgb8(channels_fchannel(channels), r, g, b)
}

/// Extract 24 bits of background RGB from 'channels', split into subchannels.
#[inline]
pub fn channels_bg_rgb8(
    channels: NcChannels,
    r: &mut NcColor,
    g: &mut NcColor,
    b: &mut NcColor,
) -> NcChannel {
    channel_rgb8(channels_bchannel(channels), r, g, b)
}

/// Set the r, g, and b channels for the foreground component of this 64-bit
/// 'channels' variable, and mark it as not using the default color.
#[inline]
pub fn channels_set_fg_rgb8(channels: &mut NcChannels, r: NcColor, g: NcColor, b: NcColor) {
    let mut channel = channels_fchannel(*channels);
    channel_set_rgb8(&mut channel, r, g, b);
    *channels = (channel as u64) << 32 | *channels & 0xffffffff_u64;
}

/// Same as channels_set_fg_rgb8 but set an assembled 24 bit channel at once.
#[inline]
pub fn channels_set_fg_rgb(channels: &mut NcChannels, rgb: NcRgb) {
    let mut channel = channels_fchannel(*channels);
    channel_set(&mut channel, rgb);
    *channels = (channel as u64) << 32 | *channels & 0xffffffff_u64;
}

/// Set the r, g, and b channels for the background component of this 64-bit
/// 'channels' variable, and mark it as not using the default color.
#[inline]
pub fn channels_set_bg_rgb8(channels: &mut NcChannels, r: NcColor, g: NcColor, b: NcColor) {
    let mut channel = channels_bchannel(*channels);
    channel_set_rgb8(&mut channel, r, g, b);
    channels_set_bchannel(channels, channel);
}

/// Same as channels_set_bg_rgb8 but set an assembled 24 bit channel at once.
#[inline]
pub fn channels_set_bg_rgb(channels: &mut NcChannels, rgb: NcRgb) {
    let mut channel = channels_bchannel(*channels);
    channel_set(&mut channel, rgb);
    channels_set_bchannel(channels, channel);
}

/// Set the 2-bit alpha component of the foreground channel.
#[inline]
pub fn channels_set_fg_alpha(channels: &mut NcChannels, alpha: NcAlphaBits) {
    let mut channel = channels_fchannel(*channels);
    channel_set_alpha(&mut channel, alpha);
    *channels = (channel as NcChannels) << 32 | *channels & 0xffffffff_u64;
}

/// Set the 2-bit alpha component of the background channel.
#[inline]
pub fn channels_set_bg_alpha(channels: &mut NcChannels, alpha: NcAlphaBits) {
    let mut alpha_clean = alpha;
    if alpha == NCCELL_ALPHA_HIGHCONTRAST {
        // forbidden for background alpha, so makes it opaque
        alpha_clean = NCCELL_ALPHA_OPAQUE;
    }
    let mut channel = channels_bchannel(*channels);
    channel_set_alpha(&mut channel, alpha_clean);
    channels_set_bchannel(channels, channel);
}

/// Is the foreground using the "default foreground color"?
#[inline]
pub fn channels_fg_default_p(channels: NcChannels) -> bool {
    channel_default_p(channels_fchannel(channels))
}

/// Is the foreground using indexed palette color?
#[inline]
pub fn channels_fg_palindex_p(channels: NcChannels) -> bool {
    channel_palindex_p(channels_fchannel(channels))
}

/// Is the background using the "default background color"? The "default
/// background color" must generally be used to take advantage of
/// terminal-effected transparency.
#[inline]
pub fn channels_bg_default_p(channels: NcChannels) -> bool {
    channel_default_p(channels_bchannel(channels))
}

/// Is the background using indexed palette color?
#[inline]
pub fn channels_bg_palindex_p(channels: NcChannels) -> bool {
    channel_palindex_p(channels_bchannel(channels))
}

/// Set the cell's background palette index, set the background palette index
/// bit, set it background-opaque, and clear the background default color bit.
#[inline]
pub fn channels_set_bg_palindex(channels: &mut NcChannels, index: NcPaletteIndex) {
    *channels |= NCCELL_BGDEFAULT_MASK as NcChannels;
    *channels |= NCCELL_BG_PALETTE as NcChannels;
    channels_set_bg_alpha(channels, NCCELL_ALPHA_OPAQUE);
    *channels &= 0xffffffffff000000;
    *channels |= index as NcChannels;
}

/// Set the cell's foreground palette index, set the foreground palette index
/// bit, set it foreground-opaque, and clear the foreground default color bit.
#[inline]
pub fn channels_set_fg_palindex(channels: &mut NcChannels, index: NcPaletteIndex) {
    *channels |= NCCELL_FGDEFAULT_MASK;
    *channels |= NCCELL_FG_PALETTE as NcChannels;
    channels_set_fg_alpha(channels, NCCELL_ALPHA_OPAQUE);
    *channels &= 0xff000000ffffffff as NcChannels;
    *channels |= (index as NcChannels) << 32;
}

/// Mark the foreground channel as using its default color.
#[inline]
pub fn channels_set_fg_default(channels: &mut NcChannels) -> NcChannels {
    let mut channel = channels_fchannel(*channels);
    channel_set_default(&mut channel);
    *channels = (channel as u64) << 32 | *channels & 0xffffffff_u64;
    *channels
}

/// Mark the background channel as using its default color.
#[inline]
pub fn channels_set_bg_default(channels: &mut NcChannels) -> NcChannels {
    let mut channel = channels_bchannel(*channels);
    channel_set_default(&mut channel);
    channels_set_bchannel(channels, channel);
    *channels
}
