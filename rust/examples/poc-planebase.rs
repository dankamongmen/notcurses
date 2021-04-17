use libnotcurses_sys::*;

fn main() -> NcResult<()> {
    let mut nc = Nc::with_debug(NCLOGLEVEL_INFO, 0)?;

    let (dimy, dimx) = nc.term_dim_yx();
    let stdn = nc.stdplane();

    let p1 = NcPlane::new_bound(stdn, 0, 0, dimy, dimx)?;

    let mut channels = NcChannelPair::with_rgb8(0, 0xcc, 0, 0, 0x88, 0);
    p1.set_base(" ", 0, channels)?;

    let p2 = NcPlane::new_bound(stdn, 0, 0, dimy / 3, dimx / 3)?;
    p2.set_base(" ", 0, channels.set_bg_rgb8(0x88, 0, 0))?;

    let p3 = NcPlane::new_bound(stdn, 0, 0, dimy / 9, dimx / 9)?;
    p3.set_base(" ", 0, channels.set_bg_rgb8(0, 0, 0x88))?;

    rsleep![&mut nc, 1];

    p3.move_yx((dimy - dimy / 9) as i32, (dimx - dimx / 9) as i32)?;
    rsleep![&mut nc, 1];

    p2.move_yx((dimy / 2) as i32, (dimx / 2) as i32)?;
    rsleep![&mut nc, 1];

    Ok(())
}
