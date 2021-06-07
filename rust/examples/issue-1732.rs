use libnotcurses_sys::*;

fn main() -> NcResult<()> {
    let mut nc = Notcurses::new()?;

    let chan_blue = NcChannelPair::with_rgb(0x88aa00, 0x2222287);
    let chan_green = NcChannelPair::with_rgb(0x224411, 0x229922);

    // FIXME: this doesn't show at all :/
    let plane1 = NcPlane::new(&mut nc, 0, 0, 20, 40)?;
    plane1.set_base("1", 0, chan_green)?;
    let plane2 = NcPlane::new_bound(plane1, 0, 0, 2, 4)?;
    plane2.set_base("2", 0, chan_blue)?;
    plane1.render()?;
    plane1.rasterize()?;
    rsleep![&mut nc, 2];

    // but the stdplane does show
    nc.stdplane().set_base("s", 0, chan_blue)?;
    let substdplane = NcPlane::new_bound(nc.stdplane(), 5, 10, 10, 20)?;
    substdplane.set_base("u", 0, chan_green)?;
    rsleep![&mut nc, 2];

    nc.stop()?;
    Ok(())
}
