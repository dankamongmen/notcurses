//! based on the proof of concept at ../../src/poc/cjkscroll.c

use libnotcurses_sys::*;

fn main() -> NcResult<()> {
    let mut nc = Notcurses::new()?;

    let plane = nc.stdplane();
    plane.set_scrolling(true);

    let mut wc = '\u{4e00}'; // 一

    loop {
        plane.putchar(wc)?;
        wc = core::char::from_u32(wc as u32 + 1).expect("invalid char");

        // 龣
        if wc == '\u{9fa5}' {
            wc = '\u{4e00}';
        }
        rsleep![&mut nc, 0, 0, 30];
    }

    // nc.stop()?;
}
