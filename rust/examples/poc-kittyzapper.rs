//! based on the proof of concept at ../../src/poc/kittyzapper.c

use libnotcurses_sys::*;

fn main() {
    let ncd = NcDirect::new();
    ncd.fg_rgb8(100, 100, 100);
    ncd.bg_rgb8(0xff, 0xff, 0xff);
    printf!("a");
    ncd.bg_rgb8(0, 0, 0);
    printf!("b");
    printf!(" ");
    printf!(" ");
    ncd.bg_rgb8(0, 0, 1);
    printf!("c");
    printf!(" ");
    printf!(" ");
    ncd.bg_rgb8(0xff, 0xff, 0xff);
    printf!("d");
    printf!("\n");
    ncd.stop();
}
