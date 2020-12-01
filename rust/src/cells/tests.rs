//! [`NcCell`] tests

use crate::NcCell;

use serial_test::serial;

#[test]
#[serial]
fn constructors() {
    let _c1 = NcCell::new_blank();

    let _c2 = NcCell::with_char('C');

    let _c3 = NcCell::new('c', 0, 0);
}

#[test]
#[serial]
fn channels() {
    let mut c1 = NcCell::new_blank();
    assert_eq![0, crate::cell_fchannel(&c1)];
    assert_eq![0, crate::cell_bchannel(&c1)];

    crate::cell_set_fchannel(&mut c1, 0x77112233);
    assert_eq![0x77112233, crate::cell_fchannel(&c1)];
    crate::cell_set_bchannel(&mut c1, 0x88445566);
    assert_eq![0x88445566, crate::cell_bchannel(&c1)];

    let c2 = NcCell::new(' ', 0, 0x0011223300445566);
    assert_eq![0x112233, crate::cell_fchannel(&c2)];
    assert_eq![0x445566, crate::cell_bchannel(&c2)];
}

#[test]
#[serial]
fn rgb() {
    // rgb

    let mut c1 = NcCell::new_blank();
    assert_eq![0, crate::cell_fg_rgb(&c1)];
    assert_eq![0, crate::cell_bg_rgb(&c1)];

    crate::cell_set_fg_rgb(&mut c1, 0x99112233);
    assert_eq![0x112233, crate::cell_fg_rgb(&c1)];
    crate::cell_set_bg_rgb(&mut c1, 0x99445566);
    assert_eq![0x445566, crate::cell_bg_rgb(&c1)];

    // rgb8

    let mut c2 = NcCell::new_blank();
    let (mut r, mut g, mut b) = (0, 0, 0);

    crate::cell_set_fg_rgb8(&mut c2, 0x11, 0x22, 0x33);
    let fchannel = crate::cell_fg_rgb8(&c2, &mut r, &mut g, &mut b);
    assert_eq!((0x11, 0x22, 0x33), (r, g, b));
    assert_eq![0x112233, fchannel & !crate::NCCELL_BGDEFAULT_MASK];

    crate::cell_set_bg_rgb8(&mut c2, 0x44, 0x55, 0x66);
    let bchannel = crate::cell_bg_rgb8(&c2, &mut r, &mut g, &mut b);
    assert_eq!((0x44, 0x55, 0x66), (r, g, b));
    assert_eq![0x445566, bchannel & !crate::NCCELL_BGDEFAULT_MASK];
}

#[test]
#[serial]
fn alpha() {
    let mut c1 = NcCell::new_blank();
    assert_eq![0, crate::cell_fg_alpha(&c1)];
    assert_eq![0, crate::cell_bg_alpha(&c1)];

    crate::cell_set_fg_alpha(&mut c1, crate::NCCELL_ALPHA_TRANSPARENT);
    assert_eq![crate::NCCELL_ALPHA_TRANSPARENT, crate::cell_fg_alpha(&c1)];

    crate::cell_set_bg_alpha(&mut c1, crate::NCCELL_ALPHA_BLEND);
    assert_eq![crate::NCCELL_ALPHA_BLEND, crate::cell_bg_alpha(&c1)];
}

#[test]
#[serial]
fn default() {
    let mut c1 = NcCell::new_blank();
    assert_eq![true, crate::cell_fg_default_p(&c1)];
    assert_eq![true, crate::cell_bg_default_p(&c1)];

    // rgb
    crate::cell_set_fg_rgb(&mut c1, 0x112233);
    crate::cell_set_bg_rgb(&mut c1, 0x445566);
    assert_eq![false, crate::cell_fg_default_p(&c1)];
    assert_eq![false, crate::cell_bg_default_p(&c1)];

    // reset
    crate::cell_set_fg_default(&mut c1);
    crate::cell_set_bg_default(&mut c1);
    assert_eq![true, crate::cell_fg_default_p(&c1)];
    assert_eq![true, crate::cell_bg_default_p(&c1)];

    // rgb8
    crate::cell_set_fg_rgb8(&mut c1, 0x11, 0x22, 0x33);
    crate::cell_set_bg_rgb8(&mut c1, 0x44, 0x55, 0x66);
    assert_eq![false, crate::cell_fg_default_p(&c1)];
    assert_eq![false, crate::cell_bg_default_p(&c1)];

    // reset
    crate::cell_set_fg_default(&mut c1);
    crate::cell_set_bg_default(&mut c1);

    // palette
    crate::cell_set_fg_palindex(&mut c1, 5);
    crate::cell_set_bg_palindex(&mut c1, 6);
    assert_eq![false, crate::cell_fg_default_p(&c1)];
    assert_eq![false, crate::cell_bg_default_p(&c1)];
}

#[test]
#[serial]
fn palette() {
    let mut c1 = NcCell::new_blank();
    assert_eq![false, crate::cell_fg_palindex_p(&c1)];
    assert_eq![false, crate::cell_bg_palindex_p(&c1)];
    assert_eq![0, crate::cell_fg_palindex(&c1)];
    assert_eq![0, crate::cell_bg_palindex(&c1)];

    crate::cell_set_fg_palindex(&mut c1, 5);
    crate::cell_set_bg_palindex(&mut c1, 6);
    assert_eq![true, crate::cell_fg_palindex_p(&c1)];
    assert_eq![true, crate::cell_bg_palindex_p(&c1)];

    assert_eq![5, crate::cell_fg_palindex(&c1)];
    assert_eq![6, crate::cell_bg_palindex(&c1)];
}
