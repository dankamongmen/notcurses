//! based on the proof of concept at ../../src/poc/kittyzapper.c

use libnotcurses_sys::*;

fn main() -> NcResult<()> {
    let dm = NcDirect::new()?;

    dm.set_fg_rgb8(100, 100, 100)?;
    dm.set_bg_rgb8(0xff, 0xff, 0xff)?;
    printf!("a");
    dm.set_bg_rgb8(0, 0, 0)?;
    printf!("b");
    printf!(" ");
    printf!(" ");
    dm.set_bg_rgb8(0, 0, 1)?;
    printf!("c");
    printf!(" ");
    printf!(" ");
    dm.set_bg_rgb8(0xff, 0xff, 0xff)?;
    printf!("d");
    printf!("\n");

    Ok(())
}
