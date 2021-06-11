use libnotcurses_sys::*;

fn main() -> NcResult<()> {
    let mut nc = Notcurses::new()?;

    // NOT USING STDPLANE movement is relative

    let pred = NcPlane::new(&mut nc, 0, 0, 10, 20)?;
    pred.set_base("·", 0, NcChannelPair::with_rgb(0xaadd2b, 0x882222))?;
    prsleep![pred, 0, 300];

    for _ in 0..8 {
        pred.move_yx(1, 1)?;
        // pred.move_rel(1, 1)?; // this would move in geometric proportion
        prsleep![pred, 0, 100];
    }
    for _ in 0..8 {
        pred.move_yx(-1, -1)?;
        // pred.move_rel(-1, -1)?; //
        prsleep![pred, 0, 100];
    }
    sleep![1];
    pred.destroy()?;


    // USING STDPLANE movement is absolute!

    let pgreen = NcPlane::new_bound(nc.stdplane(), 0, 0, 10, 20)?;
    pgreen.set_base("s", 0, NcChannelPair::with_rgb(0x224411, 0x229922))?;
    rsleep![&mut nc, 0, 800];

    for _ in 0..8 {
        pgreen.move_yx(1, 1)?;
        // pgreen.move_rel(1, 1)?; // this works like move_yx on the red plane
        rsleep![&mut nc, 0, 100];
    }
    for _ in 0..8 {
        pgreen.move_yx(-1, -1)?;
        // pgreen.move_rel(-1, -1)?; //
        rsleep![&mut nc, 0, 100];
    }
    sleep![2];
    pgreen.destroy()?;

    nc.stop()?;
    Ok(())
}
