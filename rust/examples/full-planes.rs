use libnotcurses_sys::*;

fn main() -> NcResult<()> {
    let mut nc = Notcurses::new()?;

    // get the terminal size in character rows & columns
    let (t_rows, t_cols) = nc.term_dim_yx();

    // the standard plane has the same size
    let mut stdplane = nc.stdplane();
    assert_eq![(t_rows, t_cols), stdplane.dim_yx()];

    // set the standard plane's base cell's foreground and background colors
    let channels = NcChannelPair::with_rgb(0x88aa00, 0x2222288);
    stdplane.set_base("x", 0, channels)?;
    rsleep![&mut nc, 0, 500];

    // add a green plane to the stdplane's pile
    let plane_green = NcPlane::new_bound(&mut stdplane, 0, 0, 16, 30)?;
    plane_green.set_base("·", 0, NcChannelPair::with_rgb(0x224411, 0x229922))?;
    rsleep![&mut nc, 0, 800];

    // add a smaller red plane, a bit displaced to the bottom right
    let plane_red = NcPlane::new_bound(&mut stdplane, 8, 12, 10, 20)?;
    plane_red.set_base("~", 0, NcChannelPair::with_rgb(0xaadd2b, 0x882222))?;
    rsleep![&mut nc, 0, 800];

    // write something
    plane_green.putstr("PLANE 11111")?;
    plane_red.putstr("PLANE 22222")?;
    rsleep![&mut nc, 0, 800];

    // move the green plane down
    for _ in 0..16 {
        plane_green.move_rel(1, 1)?;
        rsleep![&mut nc, 0, 30];
    }
    // and up
    for _ in 0..20 {
        plane_green.move_rel(-1, -1)?;
        rsleep![&mut nc, 0, 35];
    }

    // move the red plane right
    for _ in 0..26 {
        plane_red.move_rel(0, 1)?;
        rsleep![&mut nc, 0, 40];
    }
    sleep![1];

    // resize the red plane
    plane_red.resize_simple(14, 24)?;
    rsleep![&mut nc, 0, 300];
    plane_red.move_rel(-2, -2)?;
    rsleep![&mut nc, 0, 300];
    plane_red.resize_simple(8, 16)?;

    // TODO…

    rsleep![&mut nc, 3];

    nc.stop()?;
    Ok(())
}
