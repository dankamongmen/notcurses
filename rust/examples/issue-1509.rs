//! https://github.com/dankamongmen/notcurses/issues/1509
//! strange color behaviour when moving planes
//
// TODO
// -
//

use std::collections::BTreeMap;

use libnotcurses_sys::*;

fn main() -> NcResult<()> {
    let mut nc = FullMode::new()?;
    
    // get the stdplane and color it green
    let green = nc.stdplane();
    let mut channels = NcChannelPair::with_rgb8(0xFF, 0, 0, 0, 0x88, 0);
    green.set_base("-", 0, channels)?;

    // create one 1x1 blue plane at 1,1
    let blue = NcPlane::new_bound(green, 1, 1, 1, 1)?;
    blue.set_base("B", 0, channels.set_bg_rgb8(0, 0, 0x88))?;
    rsleep![&mut nc, 1];

    // move it to 4,4
    // BUG: here it shows something is wrong
    blue.move_yx(4, 4)?;
    rsleep![&mut nc, 1];

    Ok(())
}
