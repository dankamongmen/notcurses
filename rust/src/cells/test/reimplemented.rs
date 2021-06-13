//! Test `cell*_*` reimplemented functions

use serial_test::serial;

use crate::NcCell;

#[test]
#[serial]
fn rgb() {
    // rgb

    let mut c1 = NcCell::new();
    assert_eq![0, crate::nccell_fg_rgb(&c1)];
    assert_eq![0, crate::nccell_bg_rgb(&c1)];

    crate::nccell_set_fg_rgb(&mut c1, 0x99112233);
    assert_eq![0x112233, crate::nccell_fg_rgb(&c1)];
    crate::nccell_set_bg_rgb(&mut c1, 0x99445566);
    assert_eq![0x445566, crate::nccell_bg_rgb(&c1)];

    // rgb8

    let mut c2 = NcCell::new();
    let (mut r, mut g, mut b) = (0, 0, 0);

    crate::nccell_set_fg_rgb8(&mut c2, 0x11, 0x22, 0x33);
    let fchannel = crate::nccell_fg_rgb8(&c2, &mut r, &mut g, &mut b);
    assert_eq!((0x11, 0x22, 0x33), (r, g, b));
    assert_eq![0x112233, fchannel & !crate::NCCELL_BGDEFAULT_MASK];

    crate::nccell_set_bg_rgb8(&mut c2, 0x44, 0x55, 0x66);
    let bchannel = crate::nccell_bg_rgb8(&c2, &mut r, &mut g, &mut b);
    assert_eq!((0x44, 0x55, 0x66), (r, g, b));
    assert_eq![0x445566, bchannel & !crate::NCCELL_BGDEFAULT_MASK];
}

#[test]
#[serial]
fn alpha() {
    let mut c1 = NcCell::new();
    assert_eq![0, crate::nccell_fg_alpha(&c1)];
    assert_eq![0, crate::nccell_bg_alpha(&c1)];

    crate::nccell_set_fg_alpha(&mut c1, crate::NCCELL_TRANSPARENT);
    assert_eq![crate::NCCELL_TRANSPARENT, crate::nccell_fg_alpha(&c1)];

    crate::nccell_set_bg_alpha(&mut c1, crate::NCCELL_BLEND);
    assert_eq![crate::NCCELL_BLEND, crate::nccell_bg_alpha(&c1)];
}

#[test]
#[serial]
fn default() {
    let mut c1 = NcCell::new();
    assert_eq![true, crate::nccell_fg_default_p(&c1)];
    assert_eq![true, crate::nccell_bg_default_p(&c1)];

    // rgb
    crate::nccell_set_fg_rgb(&mut c1, 0x112233);
    crate::nccell_set_bg_rgb(&mut c1, 0x445566);
    assert_eq![false, crate::nccell_fg_default_p(&c1)];
    assert_eq![false, crate::nccell_bg_default_p(&c1)];

    // reset
    crate::nccell_set_fg_default(&mut c1);
    crate::nccell_set_bg_default(&mut c1);
    assert_eq![true, crate::nccell_fg_default_p(&c1)];
    assert_eq![true, crate::nccell_bg_default_p(&c1)];

    // rgb8
    crate::nccell_set_fg_rgb8(&mut c1, 0x11, 0x22, 0x33);
    crate::nccell_set_bg_rgb8(&mut c1, 0x44, 0x55, 0x66);
    assert_eq![false, crate::nccell_fg_default_p(&c1)];
    assert_eq![false, crate::nccell_bg_default_p(&c1)];

    // reset
    crate::nccell_set_fg_default(&mut c1);
    crate::nccell_set_bg_default(&mut c1);

    // palette
    crate::nccell_set_fg_palindex(&mut c1, 5);
    crate::nccell_set_bg_palindex(&mut c1, 6);
    assert_eq![false, crate::nccell_fg_default_p(&c1)];
    assert_eq![false, crate::nccell_bg_default_p(&c1)];
}

#[test]
#[serial]
fn palette() {
    let mut c1 = NcCell::new();
    assert_eq![false, crate::nccell_fg_palindex_p(&c1)];
    assert_eq![false, crate::nccell_bg_palindex_p(&c1)];
    assert_eq![0, crate::nccell_fg_palindex(&c1)];
    assert_eq![0, crate::nccell_bg_palindex(&c1)];

    crate::nccell_set_fg_palindex(&mut c1, 5);
    crate::nccell_set_bg_palindex(&mut c1, 6);
    assert_eq![true, crate::nccell_fg_palindex_p(&c1)];
    assert_eq![true, crate::nccell_bg_palindex_p(&c1)];

    assert_eq![5, crate::nccell_fg_palindex(&c1)];
    assert_eq![6, crate::nccell_bg_palindex(&c1)];
}
