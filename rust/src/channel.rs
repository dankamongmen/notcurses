// ---------------------------------------------------------------------------------------
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
// ---------------------------------------------------------------------------------------
//
// functions already exported by bindgen : 0
// ------------------------------------------
//
// static inline functions to reimplement: 37
// ------------------------------------------ (done / (x) wont / remaining)
// (+) implement : 34 / 3 /  0
// (#) unit tests: 14 / 3 / 20
// ------------------------------------------
//#channel_alpha
//#channel_b
//#channel_default_p
//#channel_g
//+channel_palindex_p
//#channel_r
//#channel_rgb
//#channels_bchannel
//+channels_bg_alpha
//+channels_bg_default_p
//+channels_bg_palindex_p
//+channels_bg_rgb
//+channels_bg_rgb8
//#channels_combine
//+channel_set
//#channel_set_alpha
//#channel_set_default
//#channel_set_rgb
//xchannel_set_rgb_clipped
//#channels_fchannel
//+channels_fg_alpha
//+channels_fg_default_p
//+channels_fg_palindex_p
//+channels_fg_rgb
//+channels_fg_rgb8
//#channels_set_bchannel
//+channels_set_bg_alpha
//+channels_set_bg_default
//+channels_set_bg_rgb
//+channels_set_bg_rgb8
//xchannels_set_bg_rgb8_clipped
//#channels_set_fchannel
//+channels_set_fg_alpha
//+channels_set_fg_default
//+channels_set_fg_rgb
//+channels_set_fg_rgb8
//xchannels_set_fg_rgb8_clipped

use crate as nc;

use nc::types::{AlphaBits, Channel, ChannelPair, Color, Rgb};

/// Extract the 8-bit red component from a 32-bit channel.
#[inline]
pub fn channel_r(channel: Channel) -> Color {
    ((channel & 0xff0000) >> 16) as Color
}

/// Extract the 8-bit green component from a 32-bit channel.
#[inline]
pub fn channel_g(channel: Channel) -> Color {
    ((channel & 0x00ff00) >> 8) as Color
}

/// Extract the 8-bit blue component from a 32-bit channel.
#[inline]
pub fn channel_b(channel: Channel) -> Color {
    (channel & 0x0000ff) as Color
}

/// Extract the three 8-bit R/G/B components from a 32-bit channel.
#[inline]
pub fn channel_rgb(channel: Channel, r: &mut Color, g: &mut Color, b: &mut Color) -> Channel {
    *r = channel_r(channel);
    *g = channel_g(channel);
    *b = channel_b(channel);
    channel
}

/// Set the three 8-bit components of a 32-bit channel, and mark it as not using
/// the default color. Retain the other bits unchanged.
#[inline]
pub fn channel_set_rgb(channel: &mut Channel, r: Color, g: Color, b: Color) {
    let rgb: Rgb = (r as Channel) << 16 | (g as Channel) << 8 | (b as Channel);
    *channel = (*channel & !nc::CELL_BG_RGB_MASK) | nc::CELL_BGDEFAULT_MASK | rgb;
}

/// Same as channel_set_rgb(), but provide an assembled, packed 24 bits of rgb.
// TODO: TEST
#[inline]
pub fn channel_set(channel: &mut Channel, rgb: Rgb) {
    *channel = (*channel & !nc::CELL_BG_RGB_MASK) | nc::CELL_BGDEFAULT_MASK | (rgb & 0x00ffffff);
}

/// Extract the 2-bit alpha component from a 32-bit channel.
#[inline]
pub fn channel_alpha(channel: Channel) -> AlphaBits {
    channel & nc::CHANNEL_ALPHA_MASK
}

/// Set the 2-bit alpha component of the 32-bit channel.
#[inline]
pub fn channel_set_alpha(channel: &mut Channel, alpha: AlphaBits) {
    let alpha_clean = alpha & nc::CHANNEL_ALPHA_MASK;
    *channel = alpha_clean | (*channel & !nc::CHANNEL_ALPHA_MASK);

    if alpha != nc::CELL_ALPHA_OPAQUE {
        // indicate that we are *not* using the default background color
        *channel |= nc::CELL_BGDEFAULT_MASK;
    }
}

/// Is this channel using the "default color" rather than RGB/palette-indexed?
// TODO: TEST
#[inline]
pub fn channel_default_p(channel: Channel) -> bool {
    (channel & nc::CELL_BGDEFAULT_MASK) == 0
}

/// Is this channel using palette-indexed color rather than RGB?
// TODO: TEST
#[inline]
pub fn channel_palindex_p(channel: Channel) -> bool {
    !channel_default_p(channel) && (channel & nc::CELL_BG_PALETTE) == 0
}

/// Mark the channel as using its default color, which also marks it opaque.
#[inline]
pub fn channel_set_default(channel: &mut Channel) -> Channel {
    *channel &= !(nc::CELL_BGDEFAULT_MASK | nc::CELL_ALPHA_HIGHCONTRAST); // < NOTE shouldn't be better CHANNEL_ALPHA_MASK?
    *channel
}

/// Extract the 32-bit background channel from a channel pair.
#[inline]
pub fn channels_bchannel(channels: ChannelPair) -> Channel {
    (channels & 0xffffffff_u64) as Channel
}

/// Extract the 32-bit foreground channel from a channel pair.
#[inline]
pub fn channels_fchannel(channels: ChannelPair) -> Channel {
    channels_bchannel(channels >> 32)
}

/// Set the 32-bit background channel of a channel pair.
#[inline]
pub fn channels_set_bchannel(channels: &mut ChannelPair, bchannel: Channel) -> ChannelPair {
    *channels = (*channels & 0xffffffff00000000_u64) | bchannel as u64;
    *channels
}

/// Set the 32-bit foreground channel of a channel pair.
#[inline]
pub fn channels_set_fchannel(channels: &mut ChannelPair, fchannel: Channel) -> ChannelPair {
    *channels = (*channels & 0xffffffff_u64) | (fchannel as u64) << 32;
    *channels
}

/// Combine two channels into a channel pair.
#[inline]
pub fn channels_combine(fchannel: Channel, bchannel: Channel) -> ChannelPair {
    let mut channels: ChannelPair = 0;
    channels_set_fchannel(&mut channels, fchannel);
    channels_set_bchannel(&mut channels, bchannel);
    channels
}

/// Extract 24 bits of foreground RGB from 'channels', shifted to LSBs.
// TODO: TEST
#[inline]
pub fn channels_fg_rgb(channels: ChannelPair) -> Channel {
    channels_fchannel(channels) & nc::CELL_BG_RGB_MASK
}

/// Extract 24 bits of background RGB from 'channels', shifted to LSBs.
// TODO: TEST
#[inline]
pub fn channels_bg_rgb(channels: ChannelPair) -> Channel {
    channels_bchannel(channels) & nc::CELL_BG_RGB_MASK
}

/// Extract 2 bits of foreground alpha from 'channels', shifted to LSBs.
// TODO: TEST
#[inline]
pub fn channels_fg_alpha(channels: ChannelPair) -> AlphaBits {
    channel_alpha(channels_fchannel(channels))
}

/// Extract 2 bits of background alpha from 'channels', shifted to LSBs.
// TODO: TEST
#[inline]
pub fn channels_bg_alpha(channels: ChannelPair) -> AlphaBits {
    channel_alpha(channels_bchannel(channels))
}

/// Extract 24 bits of foreground RGB from 'channels', split into subchannels.
// TODO: TEST
#[inline]
pub fn channels_fg_rgb8(
    channels: ChannelPair,
    r: &mut Color,
    g: &mut Color,
    b: &mut Color,
) -> Channel {
    channel_rgb(channels_fchannel(channels), r, g, b)
}

/// Extract 24 bits of background RGB from 'channels', split into subchannels.
// TODO: TEST
#[inline]
pub fn channels_bg_rgb8(
    channels: ChannelPair,
    r: &mut Color,
    g: &mut Color,
    b: &mut Color,
) -> Channel {
    channel_rgb(channels_bchannel(channels), r, g, b)
}

/// Set the r, g, and b channels for the foreground component of this 64-bit
/// 'channels' variable, and mark it as not using the default color.
// TODO: TEST
#[inline]
pub fn channels_set_fg_rgb8(channels: &mut ChannelPair, r: Color, g: Color, b: Color) {
    let mut channel = channels_fchannel(*channels);
    channel_set_rgb(&mut channel, r, g, b);
    *channels = (channel as u64) << 32 | *channels & 0xffffffff_u64;
}

/// Same as channels_set_fg_rgb8 but set an assembled 24 bit channel at once.
// TODO: TEST
#[inline]
pub fn channels_set_fg_rgb(channels: &mut ChannelPair, rgb: Rgb) {
    let mut channel = channels_fchannel(*channels);
    channel_set(&mut channel, rgb);
    *channels = (channel as u64) << 32 | *channels & 0xffffffff_u64;
}

/// Set the r, g, and b channels for the background component of this 64-bit
/// 'channels' variable, and mark it as not using the default color.
// TODO: TEST
#[inline]
pub fn channels_set_bg_rgb8(channels: &mut ChannelPair, r: Color, g: Color, b: Color) {
    let mut channel = channels_bchannel(*channels);
    channel_set_rgb(&mut channel, r, g, b);
    channels_set_bchannel(channels, channel);
}

/// Same as channels_set_bg_rgb8 but set an assembled 24 bit channel at once.
// TODO: TEST
#[inline]
pub fn channels_set_bg_rgb(channels: &mut ChannelPair, rgb: Rgb) {
    let mut channel = channels_bchannel(*channels);
    channel_set(&mut channel, rgb);
    channels_set_bchannel(channels, channel);
}

/// Set the 2-bit alpha component of the foreground channel.
// TODO: TEST
#[inline]
pub fn channels_set_fg_alpha(channels: &mut ChannelPair, alpha: AlphaBits) {
    let mut channel = channels_fchannel(*channels);
    channel_set_alpha(&mut channel, alpha);
    *channels = (channel as ChannelPair) << 32 | *channels & 0xffffffff_u64;
}

/// Set the 2-bit alpha component of the background channel.
// TODO: TEST
#[inline]
pub fn channels_set_bg_alpha(channels: &mut ChannelPair, alpha: AlphaBits) {
    let mut alpha_clean = alpha;
    if alpha == nc::CELL_ALPHA_HIGHCONTRAST {
        // forbidden for background alpha, so makes it opaque
        alpha_clean = nc::CELL_ALPHA_OPAQUE;
    }
    let mut channel = channels_bchannel(*channels);
    channel_set_alpha(&mut channel, alpha_clean);
    channels_set_bchannel(channels, channel);
}

/// Is the foreground using the "default foreground color"?
// TODO: TEST
#[inline]
pub fn channels_fg_default_p(channels: ChannelPair) -> bool {
    channel_default_p(channels_fchannel(channels))
}

/// Is the foreground using indexed palette color?
// TODO: TEST
#[inline]
pub fn channels_fg_palindex_p(channels: ChannelPair) -> bool {
    channel_palindex_p(channels_fchannel(channels))
}

/// Is the background using the "default background color"? The "default
/// background color" must generally be used to take advantage of
/// terminal-effected transparency.
// TODO: TEST
#[inline]
pub fn channels_bg_default_p(channels: ChannelPair) -> bool {
    channel_default_p(channels_bchannel(channels))
}

/// Is the background using indexed palette color?
// TODO: TEST
#[inline]
pub fn channels_bg_palindex_p(channels: ChannelPair) -> bool {
    channel_palindex_p(channels_bchannel(channels))
}

/// Mark the foreground channel as using its default color.
// TODO: TEST
#[inline]
pub fn channels_set_fg_default(channels: &mut ChannelPair) -> ChannelPair {
    let mut channel = channels_fchannel(*channels);
    channel_set_default(&mut channel);
    *channels = (channel as u64) << 32 | *channels & 0xffffffff_u64;
    *channels
}

/// Mark the background channel as using its default color.
// TODO: TEST
#[inline]
pub fn channels_set_bg_default(channels: &mut ChannelPair) -> ChannelPair {
    let mut channel = channels_bchannel(*channels);
    channel_set_default(&mut channel);
    channels_set_bchannel(channels, channel);
    *channels
}

#[cfg(test)]
mod test {
    use super::{nc, Channel, ChannelPair};
    use serial_test::serial;

    #[test]
    #[serial]
    fn channel_r() {
        let c: Channel = 0x112233;
        assert_eq!(super::channel_r(c), 0x11);
    }
    #[test]
    #[serial]
    fn channel_g() {
        let c: Channel = 0x112233;
        assert_eq!(super::channel_g(c), 0x22);
    }
    #[test]
    #[serial]
    fn channel_b() {
        let c: Channel = 0x112233;
        assert_eq!(super::channel_b(c), 0x33);
    }
    #[test]
    #[serial]
    fn channel_rgb() {
        let c: Channel = 0x112233;
        let mut r = 0;
        let mut g = 0;
        let mut b = 0;
        super::channel_rgb(c, &mut r, &mut g, &mut b);
        assert_eq!(r, 0x11);
        assert_eq!(g, 0x22);
        assert_eq!(b, 0x33);
    }
    #[test]
    #[serial]
    fn channel_set_rgb() {
        let mut c: Channel = 0x000000;
        super::channel_set_rgb(&mut c, 0x11, 0x22, 0x33);
        assert_eq!(super::channel_r(c), 0x11);
        assert_eq!(super::channel_g(c), 0x22);
        assert_eq!(super::channel_b(c), 0x33);
    }
    #[test]
    #[serial]
    fn channel_alpha() {
        let c: Channel = 0x112233 | nc::CELL_ALPHA_TRANSPARENT;
        assert_eq!(super::channel_alpha(c), nc::CELL_ALPHA_TRANSPARENT);
    }
    #[test]
    #[serial]
    fn channel_set_alpha() {
        let mut c: Channel = 0x112233;
        super::channel_set_alpha(&mut c, nc::CELL_ALPHA_HIGHCONTRAST);
        assert_eq!(nc::CELL_ALPHA_HIGHCONTRAST, super::channel_alpha(c));

        super::channel_set_alpha(&mut c, nc::CELL_ALPHA_TRANSPARENT);
        assert_eq!(nc::CELL_ALPHA_TRANSPARENT, super::channel_alpha(c));

        super::channel_set_alpha(&mut c, nc::CELL_ALPHA_BLEND);
        assert_eq!(nc::CELL_ALPHA_BLEND, super::channel_alpha(c));

        super::channel_set_alpha(&mut c, nc::CELL_ALPHA_OPAQUE);
        assert_eq!(nc::CELL_ALPHA_OPAQUE, super::channel_alpha(c));
        // TODO: CHECK for nc::CELL_BGDEFAULT_MASK
    }

    #[test]
    #[serial]
    fn channel_set_default() {
        const DEFAULT: Channel = 0x112233;

        let mut c: Channel = DEFAULT | nc::CELL_ALPHA_TRANSPARENT;
        assert!(c != DEFAULT);

        super::channel_set_default(&mut c);
        assert_eq!(c, DEFAULT);
    }

    #[test]
    #[serial]
    fn channel_default_p() {
        let mut c: Channel = 0x112233;
        assert_eq!(true, super::channel_default_p(c));

        // TODO FIXME: test for the false result
        // let _ = super::channel_set_alpha(&mut c, nc::CELL_ALPHA_TRANSPARENT);
        // assert_eq!(false, super::channel_default_p(c));

        let _ = super::channel_set_alpha(&mut c, nc::CELL_ALPHA_OPAQUE);
        assert_eq!(true, super::channel_default_p(c));
    }
    #[test]
    #[serial]
    #[allow(non_snake_case)]
    fn channels_set_fchannel__channels_fchannel() {
        let fc: Channel = 0x112233;
        let mut cp: ChannelPair = 0;
        super::channels_set_fchannel(&mut cp, fc);
        assert_eq!(super::channels_fchannel(cp), fc);
    }
    #[test]
    #[serial]
    #[allow(non_snake_case)]
    fn channels_set_bchannel__channels_bchannel() {
        let bc: Channel = 0x112233;
        let mut cp: ChannelPair = 0;
        super::channels_set_bchannel(&mut cp, bc);
        assert_eq!(super::channels_bchannel(cp), bc);
    }
    #[test]
    #[serial]
    fn channels_combine() {
        let bc: Channel = 0x112233;
        let fc: Channel = 0x445566;
        let mut cp1: ChannelPair = 0;
        let mut _cp2: ChannelPair = 0;
        super::channels_set_bchannel(&mut cp1, bc);
        super::channels_set_fchannel(&mut cp1, fc);
        _cp2 = super::channels_combine(fc, bc);
        assert_eq!(cp1, _cp2);
    }
}
