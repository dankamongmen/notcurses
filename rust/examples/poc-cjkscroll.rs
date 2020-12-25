//! based on the proof of concept at ../../src/poc/cjkscroll.c

use libnotcurses_sys::*;

fn main() -> NcResult<()> {

    let nc = Notcurses::new()?;
    let plane = nc.stdplane();
    plane.set_scrolling(true);

    let mut wc = '\u{4e00}'; // 一

    loop {
        sleep![1];

        if plane.putchar(wc) == NCRESULT_ERR {
            break;
        }

        wc = core::char::from_u32(wc as u32 + 1).expect("invalid char");

        if wc == '\u{9fa5}' { // 龣
            wc = '\u{4e00}';
        }

        if nc.render() == NCRESULT_ERR {
            break;
        }
    }
    nc.stop()?;
    Ok(())
}
