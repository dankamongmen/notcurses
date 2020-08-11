// ---------------------------------------------------------------------------------------
// - NOTE: The channel components are u8 instead of u32.
//   Because of type enforcing, some runtime checks are now unnecessary.
//
// - NOTE: These functions now can't fail and don't have to return an error:
//   - `channel_set_rgb()`
//   - `channels_set_fg_rgb()`
//   - `channels_set_bg_rgb()`
//   - `channel_set()`
//   - `channels_set_fg()`
//   - `channels_set_bg()`
//
// - NOTE: These functions were therefore deemed unnecessary to implement:
//   - `channel_set_rgb_clipped()`
//   - `channels_set_fg_rgb_clipped()`
//   - `channels_set_bg_rgb_clipped()`
//
// - These functions still return an integer error result:
//   - `channel_set_alpha()`
//   - `channels_set_fg_alpha()`
//   - `channels_set_bg_alpha()`
// ---------------------------------------------------------------------------------------
//
// functions already exported by bindgen : 0
// ------------------------------------------
//
// static inline functions to reimplement: 38
// ------------------------------------------ (done / wont / remaining)
// - implement : 34 / 3 /  1
// - unit tests: 14 / 0 / 21
// --------------- (+) implemented (#) + unit test (x) wont implement
//#channel_alpha
//#channel_b
//#channel_default_p  // FIXME TEST
//#channel_g
//+channel_palindex_p
//#channel_r
//#channel_rgb
//#channels_bchannel
//+channels_bg
//+channels_bg_alpha
//+channels_bg_default_p
//+channels_bg_palindex_p
//+channels_bg_rgb
// channels_blend   // TODO
//#channels_combine
//+channel_set
//#channel_set_alpha
//#channel_set_default
//#channel_set_rgb
//xchannel_set_rgb_clipped
//#channels_fchannel
//+channels_fg
//+channels_fg_alpha
//+channels_fg_default_p
//+channels_fg_palindex_p
//+channels_fg_rgb
//#channels_set_bchannel
//+channels_set_bg
//+channels_set_bg_alpha
//+channels_set_bg_default
//+channels_set_bg_rgb
//xchannels_set_bg_rgb_clipped
//#channels_set_fchannel
//+channels_set_fg
//+channels_set_fg_alpha
//+channels_set_fg_default
//+channels_set_fg_rgb
//xchannels_set_fg_rgb_clipped
//
#![allow(dead_code)]

use crate as ffi;

use crate::types::{Alpha, Channel, ChannelPair, Color, IntResult, Rgb};

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
    *channel = (*channel & !ffi::CELL_BG_RGB_MASK) | ffi::CELL_BGDEFAULT_MASK | rgb;
}

/// Same as channel_set_rgb(), but provide an assembled, packed 24 bits of rgb.
// TODO: TEST
#[inline]
pub fn channel_set(channel: &mut Channel, rgb: Rgb) {
    *channel = (*channel & !ffi::CELL_BG_RGB_MASK) | ffi::CELL_BGDEFAULT_MASK | (rgb & 0x00ffffff);
}

/// Extract the 2-bit alpha component from a 32-bit channel.
#[inline]
pub fn channel_alpha(channel: Channel) -> Alpha {
    channel & ffi::NCCHANNEL_ALPHA_MASK
}

/// Set the 2-bit alpha component of the 32-bit channel.
#[inline]
pub fn channel_set_alpha(channel: &mut Channel, alpha: Alpha) -> IntResult {
    if (alpha & !ffi::NCCHANNEL_ALPHA_MASK) != 0 {
        return -1;
    }
    *channel = alpha | (*channel & !ffi::NCCHANNEL_ALPHA_MASK);
    if alpha != ffi::CELL_ALPHA_OPAQUE {
        // indicate that we are *not* using the default background color
        *channel |= ffi::CELL_BGDEFAULT_MASK;
    }
    0
}

/// Is this channel using the "default color" rather than RGB/palette-indexed?
// TODO: TEST
#[inline]
pub fn channel_default_p(channel: Channel) -> bool {
    (channel & ffi::CELL_BGDEFAULT_MASK) == 0
}

/// Is this channel using palette-indexed color rather than RGB?
// TODO: TEST
#[inline]
pub fn channel_palindex_p(channel: Channel) -> bool {
    !channel_default_p(channel) && (channel & ffi::CELL_BG_PALETTE) == 0
}

/// Mark the channel as using its default color, which also marks it opaque.
#[inline]
pub fn channel_set_default(channel: &mut Channel) -> Channel {
    *channel &= !(ffi::CELL_BGDEFAULT_MASK | ffi::CELL_ALPHA_HIGHCONTRAST); // < NOTE shouldn't be better NCCHANNEL_ALPHA_MASK?
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
pub fn channels_fg(channels: ChannelPair) -> Channel {
    channels_fchannel(channels) & ffi::CELL_BG_RGB_MASK
}

/// Extract 24 bits of background RGB from 'channels', shifted to LSBs.
// TODO: TEST
#[inline]
pub fn channels_bg(channels: ChannelPair) -> Channel {
    channels_bchannel(channels) & ffi::CELL_BG_RGB_MASK
}

/// Extract 2 bits of foreground alpha from 'channels', shifted to LSBs.
// TODO: TEST
#[inline]
pub fn channels_fg_alpha(channels: ChannelPair) -> Alpha {
    channel_alpha(channels_fchannel(channels))
}

/// Extract 2 bits of background alpha from 'channels', shifted to LSBs.
// TODO: TEST
#[inline]
pub fn channels_bg_alpha(channels: ChannelPair) -> Alpha {
    channel_alpha(channels_bchannel(channels))
}

/// Extract 24 bits of foreground RGB from 'channels', split into subchannels.
// TODO: TEST
#[inline]
pub fn channels_fg_rgb(
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
pub fn channels_bg_rgb(
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
pub fn channels_set_fg_rgb(channels: &mut ChannelPair, r: Color, g: Color, b: Color) {
    let mut channel = channels_fchannel(*channels);
    channel_set_rgb(&mut channel, r, g, b);
    *channels = (channel as u64) << 32 | *channels & 0xffffffff_u64;
}

/// Same as channels_set_fg_rgb but set an assembled 24 bit channel at once.
// TODO: TEST
#[inline]
pub fn channels_set_fg(channels: &mut ChannelPair, rgb: Rgb) {
    let mut channel = channels_fchannel(*channels);
    channel_set(&mut channel, rgb);
    *channels = (channel as u64) << 32 | *channels & 0xffffffff_u64;
}

/// Set the r, g, and b channels for the background component of this 64-bit
/// 'channels' variable, and mark it as not using the default color.
// TODO: TEST
#[inline]
pub fn channels_set_bg_rgb(channels: &mut ChannelPair, r: Color, g: Color, b: Color) {
    let mut channel = channels_bchannel(*channels);
    channel_set_rgb(&mut channel, r, g, b);
    channels_set_bchannel(channels, channel);
}

/// Same as channels_set_bg_rgb but set an assembled 24 bit channel at once.
// TODO: TEST
#[inline]
pub fn channels_set_bg(channels: &mut ChannelPair, rgb: Rgb) {
    let mut channel = channels_bchannel(*channels);
    channel_set(&mut channel, rgb);
    channels_set_bchannel(channels, channel);
}

/// Set the 2-bit alpha component of the foreground channel.
// TODO: TEST
#[inline]
pub fn channels_set_fg_alpha(channels: &mut ChannelPair, alpha: Alpha) -> IntResult {
    let mut channel = channels_fchannel(*channels);
    if channel_set_alpha(&mut channel, alpha) < 0 {
        return -1;
    }
    *channels = (channel as ChannelPair) << 32 | *channels & 0xffffffff_u64;
    0
}

/// Set the 2-bit alpha component of the background channel.
// TODO: TEST
#[inline]
pub fn channels_set_bg_alpha(channels: &mut ChannelPair, alpha: Alpha) -> IntResult {
    if alpha == ffi::CELL_ALPHA_HIGHCONTRAST {
        // forbidden for background alpha
        return -1;
    }
    let mut channel = channels_bchannel(*channels);
    if channel_set_alpha(&mut channel, alpha) < 0 {
        return -1;
    }
    channels_set_bchannel(channels, channel);
    0
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

/// Returns the result of blending two channels. 'blends' indicates how heavily
/// 'c1' ought be weighed. If 'blends' is 0, 'c1' will be entirely replaced by
/// 'c2'. If 'c1' is otherwise the default color, 'c1' will not be touched,
/// since we can't blend default colors. Likewise, if 'c2' is a default color,
/// it will not be used (unless 'blends' is 0).
///
/// Palette-indexed colors do not blend, and since we need the attrword to store
/// them, we just don't fuck wit' 'em here. Do not pass me palette-indexed
/// channels! I will eat them.
// TODO
// static inline unsigned
// channels_blend(unsigned c1, unsigned c2, unsigned* blends){
//   if(channel_alpha(c2) == CELL_ALPHA_TRANSPARENT){
//     return c1; // do *not* increment *blends
//   }
//   unsigned rsum, gsum, bsum;
//   channel_rgb(c2, &rsum, &gsum, &bsum);
//   bool c2default = channel_default_p(c2);
//   if(*blends == 0){
//     // don't just return c2, or you set wide status and all kinds of crap
//     if(channel_default_p(c2)){
//       channel_set_default(&c1);
//     }else{
//       channel_set_rgb(&c1, rsum, gsum, bsum);
//     }
//     channel_set_alpha(&c1, channel_alpha(c2));
//   }else if(!c2default && !channel_default_p(c1)){
//     rsum = (channel_r(c1) * *blends + rsum) / (*blends + 1);
//     gsum = (channel_g(c1) * *blends + gsum) / (*blends + 1);
//     bsum = (channel_b(c1) * *blends + bsum) / (*blends + 1);
//     channel_set_rgb(&c1, rsum, gsum, bsum);
//     channel_set_alpha(&c1, channel_alpha(c2));
//   }
//   ++*blends;
//   return c1;
// }

#[cfg(test)]
mod test {
    use super::{ffi, Channel, ChannelPair};
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
        let c: Channel = 0x112233 | ffi::CELL_ALPHA_TRANSPARENT;
        assert_eq!(super::channel_alpha(c), ffi::CELL_ALPHA_TRANSPARENT);
    }
    #[test]
    #[serial]
    fn channel_set_alpha() {
        let mut c: Channel = 0x112233;
        assert_eq!(
            0,
            super::channel_set_alpha(&mut c, ffi::CELL_ALPHA_HIGHCONTRAST)
        );
        assert_eq!(ffi::CELL_ALPHA_HIGHCONTRAST, super::channel_alpha(c));

        assert_eq!(
            0,
            super::channel_set_alpha(&mut c, ffi::CELL_ALPHA_TRANSPARENT)
        );
        assert_eq!(ffi::CELL_ALPHA_TRANSPARENT, super::channel_alpha(c));

        assert_eq!(0, super::channel_set_alpha(&mut c, ffi::CELL_ALPHA_BLEND));
        assert_eq!(ffi::CELL_ALPHA_BLEND, super::channel_alpha(c));

        assert_eq!(0, super::channel_set_alpha(&mut c, ffi::CELL_ALPHA_OPAQUE));
        assert_eq!(ffi::CELL_ALPHA_OPAQUE, super::channel_alpha(c));

        assert_eq!(-1, super::channel_set_alpha(&mut c, 12345));
    }

    #[test]
    #[serial]
    fn channel_set_default() {
        const DEFAULT: Channel = 0x112233;

        let mut c: Channel = DEFAULT | ffi::CELL_ALPHA_TRANSPARENT;
        assert!(c != DEFAULT);

        super::channel_set_default(&mut c);
        assert_eq!(c, DEFAULT);
    }

    #[test]
    #[serial]
    fn channel_default_p() {
        let mut c: Channel = 0x112233;
        assert_eq!(true, super::channel_default_p(c));

        // TODO FIXME: succesfully test for the false result
        // let _ = super::channel_set_alpha(&mut c, ffi::CELL_ALPHA_TRANSPARENT);
        // assert_eq!(false, super::channel_default_p(c));

        let _ = super::channel_set_alpha(&mut c, ffi::CELL_ALPHA_OPAQUE);
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
