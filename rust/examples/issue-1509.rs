//! https://github.com/dankamongmen/notcurses/issues/1509
//! strange color behaviour when moving planes
//
// TODO
// -
//

use std::collections::BTreeMap;

use libnotcurses_sys::*;

fn main() {
    let mut map = BTreeMap::<String, NcCell>::new();

    let _res = notcurses(&mut map);

    for (k, v) in map {
        //println!("{}: {:016X}", k, v.channels);
        //println!("{}: {:064b}", k, v.channels);
        println!("{}: {:?}", k, v);
    }
}

fn notcurses(map: &mut BTreeMap<String, NcCell>) -> NcResult<()> {
    let mut nc = FullMode::new()?;
    
    let stdplane = nc.stdplane();
    map.insert("stdp_base0".into(), stdplane.base()?);

    let mut channels = NcChannelPair::with_rgb8(0, 0, 0, 0, 0x88, 0);
    stdplane.set_base(" ", 0, channels)?;
    map.insert("stdp_base1".into(), stdplane.base()?);

    // create one 1x1 blue plane on the top left corner
    let blue = NcPlane::new_bound(stdplane, 0, 0, 1, 1)?;
    map.insert("blue_base0".into(), blue.base()?);
    blue.set_base(" ", 0, channels.set_bg_rgb8(0, 0, 0x88))?;
    map.insert("blue_base1".into(), blue.base()?);
    rsleep![&mut nc, 0, 500];

    // create another 1x1 red plane, on top
    let red = NcPlane::new_bound(stdplane, 0, 0, 1, 1)?;
    map.insert(" red_base0".into(), red.base()?);
    red.set_base(" ", 0, channels.set_bg_rgb8(0x88, 0, 0))?;
    map.insert(" red_base1".into(), red.base()?);
    rsleep![&mut nc, 0, 500];

    // move the red plane to the bottom
    // BUG: the underlying blue plane renders black
    red.move_yx(1, 0)?;
    map.insert("blue_base2".into(), blue.base()?);
    map.insert(" red_base2".into(), red.base()?);
    map.insert("stdp_base2".into(), stdplane.base()?);
    rsleep![&mut nc, 1];

    // move the blue plane to the right
    // BUG: the underlying green stdplane plane renders black
    blue.move_yx(0, 1)?;
    map.insert("stdp_base3".into(), stdplane.base()?); // CHECK
    map.insert(" red_base3".into(), red.base()?);
    map.insert("blue_base3".into(), blue.base()?);
    rsleep![&mut nc, 1];

    Ok(())
}
