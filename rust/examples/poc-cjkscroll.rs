//! based on the proof of concept at ../../src/poc/cjkscroll.c

use libnotcurses_sys::*;

fn main() -> NcResult<()> {

    let nc = Notcurses::new()?;
    let plane = nc.stdplane();
    plane.set_scrolling(true);

    let mut wc = '\u{4e00}'; // 一

    loop {
        plane.putchar(wc)?;
        wc = core::char::from_u32(wc as u32 + 1).expect("invalid char");

        if wc == '\u{9fa5}' { // 龣
            wc = '\u{4e00}';
        }
        rsleep![nc, 0, 0, 50];
    }
}
