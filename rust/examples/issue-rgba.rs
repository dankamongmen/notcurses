use libnotcurses_sys::*;

fn main() -> NcResult<()> {
    let mut nc = FullMode::new()?;
    let stdplane = nc.stdplane();

    let plane = NcPlane::new_bound(stdplane, 0, 0, 2, 5)?;
    plane.set_base("·", 0, NcChannelPair::with_rgb(0x224411, 0x229922))?;

    // BUG FIXME: uncommenting this makes next as_rgba() call fail
    // plane.putstr("PLANE")?;

    rsleep![&mut nc, 0, 500];

    let rgba = plane.as_rgba(NCBLIT_1x1, 0, 0, None, None)?;
    println!("\n\n array: {:#?}", rgba);

    rsleep![&mut nc, 3];
    Ok(())
}
