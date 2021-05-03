//! Test `channel*_*` reimplemented functions.

use serial_test::serial;

use crate::{
    NcChannel, NcChannelPair, NCCELL_ALPHA_BLEND, NCCELL_ALPHA_HIGHCONTRAST, NCCELL_ALPHA_OPAQUE,
    NCCELL_ALPHA_TRANSPARENT,
};

// NcChannel tests -------------------------------------------------------------

/// retrieves the red NcColor component
#[test]
#[serial]
fn channel_r() {
    let c: NcChannel = 0x112233;
    assert_eq!(crate::channel_r(c), 0x11);
}

/// retrieves the green NcColor component
#[test]
#[serial]
fn channel_g() {
    let c: NcChannel = 0x112233;
    assert_eq!(crate::channel_g(c), 0x22);
}

/// retrieves the blue NcColor component
#[test]
#[serial]
fn channel_b() {
    let c: NcChannel = 0x112233;
    assert_eq!(crate::channel_b(c), 0x33);
}

/// writes out the three RGB NcColor components
#[test]
#[serial]
fn channel_rgb8() {
    let c: NcChannel = 0x112233;
    let mut r = 0;
    let mut g = 0;
    let mut b = 0;
    crate::channel_rgb8(c, &mut r, &mut g, &mut b);
    assert_eq!(r, 0x11);
    assert_eq!(g, 0x22);
    assert_eq!(b, 0x33);
}

/// sets the three RGB NcColor components
#[test]
#[serial]
fn channel_set_rgb8() {
    let mut c = 0x000000;
    // by default it uses the default color
    assert_eq!(true, crate::channel_default_p(c));

    crate::channel_set_rgb8(&mut c, 0x11, 0x22, 0x33);

    assert_eq!(crate::channel_r(c), 0x11);
    assert_eq!(crate::channel_g(c), 0x22);
    assert_eq!(crate::channel_b(c), 0x33);

    // now it shoud be marked as NOT using the default color
    assert_eq!(false, crate::channel_default_p(c));
}

/// sets the NcRGB color components
#[test]
#[serial]
fn channel_set() {
    let mut c = 0x000000;
    // by default it uses the default color
    assert_eq!(true, crate::channel_default_p(c));

    crate::channel_set(&mut c, 0x112233);
    println!("\n {:08x}", c); // DEBUG

    assert_eq!(crate::channel_r(c), 0x11);
    assert_eq!(crate::channel_g(c), 0x22);
    assert_eq!(crate::channel_b(c), 0x33);

    // now it shoud be marked as NOT using the default color
    assert_eq!(false, crate::channel_default_p(c));
}


/// gets the alpha component
#[test]
#[serial]
fn channel_alpha() {
    let c: NcChannel = 0x112233;
    assert_ne!(crate::channel_alpha(c), NCCELL_ALPHA_TRANSPARENT);

    let c: NcChannel = 0x112233 | NCCELL_ALPHA_TRANSPARENT;
    assert_eq!(crate::channel_alpha(c), NCCELL_ALPHA_TRANSPARENT);
}

/// sets the alpha component
#[test]
#[serial]
fn channel_set_alpha() {
    let mut c: NcChannel = 0x112233;
    crate::channel_set_alpha(&mut c, NCCELL_ALPHA_HIGHCONTRAST);
    assert_eq!(NCCELL_ALPHA_HIGHCONTRAST, crate::channel_alpha(c));

    crate::channel_set_alpha(&mut c, NCCELL_ALPHA_TRANSPARENT);
    assert_eq!(NCCELL_ALPHA_TRANSPARENT, crate::channel_alpha(c));

    crate::channel_set_alpha(&mut c, NCCELL_ALPHA_BLEND);
    assert_eq!(NCCELL_ALPHA_BLEND, crate::channel_alpha(c));

    crate::channel_set_alpha(&mut c, NCCELL_ALPHA_OPAQUE);
    assert_eq!(NCCELL_ALPHA_OPAQUE, crate::channel_alpha(c));
    // TODO: CHECK for NCCELL_BGDEFAULT_MASK
}

/// sets the channel as using the default color
#[test]
#[serial]
fn channel_set_default() {
    let channel = 0x_00_112233;
    // By default a channel uses the default color, if the proper bit isn't set
    assert_eq!(true, crate::channel_default_p(channel));

    // If we change it from being opaque...
    let mut channel_transp = channel | NCCELL_ALPHA_TRANSPARENT;
    assert_eq!(0x_20_112233, channel_transp); // the transparent bit is now set

    crate::channel_set_not_default(&mut channel_transp);
     // both the "not default" & transparent bits are now set
    assert_eq!(0x_60_112233, channel_transp);

    // and calling set_default() should make it both default & opaque again
    assert_eq!(0x_00_112233, crate::channel_set_default(&mut channel_transp));
}

/// sets the channel as *not* using the default color
//
// more functions that marks as NOT using the default color:
// - channel_set()
// - channel_set_rgb8()
#[test]
#[serial]
fn channel_set_not_default() {
    let mut channel = 0x_00_112233;
    // By default a channel uses the default color, if the proper bit isn't set
    assert_eq!(true, crate::channel_default_p(channel));

    // marking it as NOT using the default color
    crate::channel_set_not_default(&mut channel);
    assert_eq!(0x_40_112233, channel); // check the "not default" bit is set
    assert_eq!(false, crate::channel_default_p(channel));
}

/// checks whether the channel is using the default color
#[test]
#[serial]
fn channel_default_p() {
    let mut c: NcChannel = 0x112233;
    assert_eq!(true, crate::channel_default_p(c));

    let _ = crate::channel_set_alpha(&mut c, NCCELL_ALPHA_OPAQUE);
    assert_eq!(true, crate::channel_default_p(c));

    crate::channel_set(&mut c, 0x112233);
    assert_eq!(false, crate::channel_default_p(c));
}

// NcChannelPair tests ---------------------------------------------------------

/// 
#[test]
#[serial]
#[allow(non_snake_case)]
fn channels_set_fchannel__channels_fchannel() {
    let fc: NcChannel = 0x112233;
    let mut cp: NcChannelPair = 0;
    crate::channels_set_fchannel(&mut cp, fc);
    assert_eq!(crate::channels_fchannel(cp), fc);
}

/// 
#[test]
#[serial]
#[allow(non_snake_case)]
fn channels_set_bchannel__channels_bchannel() {
    let bc: NcChannel = 0x112233;
    let mut cp: NcChannelPair = 0;
    crate::channels_set_bchannel(&mut cp, bc);
    assert_eq!(crate::channels_bchannel(cp), bc);
}

/// 
#[test]
#[serial]
fn channels_combine() {
    let bc: NcChannel = 0x112233;
    let fc: NcChannel = 0x445566;
    let mut cp1: NcChannelPair = 0;
    let mut _cp2: NcChannelPair = 0;
    crate::channels_set_bchannel(&mut cp1, bc);
    crate::channels_set_fchannel(&mut cp1, fc);
    _cp2 = crate::channels_combine(fc, bc);
    assert_eq!(cp1, _cp2);
}

/// 
#[test]
#[serial]
fn channels_palette() {
    let bc: NcChannel = 0x112233;
    let fc: NcChannel = 0x445566;
    assert_eq!(false, crate::channel_palindex_p(bc));
    assert_eq!(false, crate::channel_palindex_p(fc));

    let mut channels = crate::channels_combine(fc, bc);
    assert_eq!(false, crate::channels_fg_palindex_p(channels));
    assert_eq!(false, crate::channels_bg_palindex_p(channels));

    crate::channels_set_fg_palindex(&mut channels, 5);
    crate::channels_set_bg_palindex(&mut channels, 6);
    assert_eq!(true, crate::channels_fg_palindex_p(channels));
    assert_eq!(true, crate::channels_bg_palindex_p(channels));
}
