//! [`NcChannel`] & [`NcChannelPair`] tests

use crate::{
    NcChannel, NcChannelPair, NCCELL_ALPHA_BLEND, NCCELL_ALPHA_HIGHCONTRAST, NCCELL_ALPHA_OPAQUE,
    NCCELL_ALPHA_TRANSPARENT,
};

use serial_test::serial;

#[test]
#[serial]
fn channel_r() {
    let c: NcChannel = 0x112233;
    assert_eq!(crate::channel_r(c), 0x11);
}

#[test]
#[serial]
fn channel_g() {
    let c: NcChannel = 0x112233;
    assert_eq!(crate::channel_g(c), 0x22);
}

#[test]
#[serial]
fn channel_b() {
    let c: NcChannel = 0x112233;
    assert_eq!(crate::channel_b(c), 0x33);
}

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

#[test]
#[serial]
fn channel_set_rgb8() {
    let mut c: NcChannel = 0x000000;
    crate::channel_set_rgb8(&mut c, 0x11, 0x22, 0x33);
    assert_eq!(crate::channel_r(c), 0x11);
    assert_eq!(crate::channel_g(c), 0x22);
    assert_eq!(crate::channel_b(c), 0x33);
}

#[test]
#[serial]
fn channel_alpha() {
    let c: NcChannel = 0x112233 | NCCELL_ALPHA_TRANSPARENT;
    assert_eq!(crate::channel_alpha(c), NCCELL_ALPHA_TRANSPARENT);
}

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

#[test]
#[serial]
fn channel_set_default() {
    const DEFAULT: NcChannel = 0x112233;

    let mut c: NcChannel = DEFAULT | NCCELL_ALPHA_TRANSPARENT;
    assert!(c != DEFAULT);

    crate::channel_set_default(&mut c);
    assert_eq!(c, DEFAULT);
}

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

#[test]
#[serial]
#[allow(non_snake_case)]
fn channels_set_fchannel__channels_fchannel() {
    let fc: NcChannel = 0x112233;
    let mut cp: NcChannelPair = 0;
    crate::channels_set_fchannel(&mut cp, fc);
    assert_eq!(crate::channels_fchannel(cp), fc);
}

#[test]
#[serial]
#[allow(non_snake_case)]
fn channels_set_bchannel__channels_bchannel() {
    let bc: NcChannel = 0x112233;
    let mut cp: NcChannelPair = 0;
    crate::channels_set_bchannel(&mut cp, bc);
    assert_eq!(crate::channels_bchannel(cp), bc);
}

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
