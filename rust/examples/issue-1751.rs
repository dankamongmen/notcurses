use libnotcurses_sys::*;

const W: u32 = 32;
const H: u32 = 32;

fn main() -> NcResult<()> {
    let nc = Nc::new()?;

    // create a white rectangle
    let buffer = vec![255; H as usize * W as usize * 4];
    let v = NcVisual::from_rgba(buffer.as_slice(), H, W * 4, W)?;
    let vo = NcVisualOptions::without_plane(0, 0, 0, 0, H, W, NCBLIT_PIXEL, 0, 0);

    // BUG: render function fails when downsizing
    v.resize(H / 2, W / 2)?;
    v.render(nc, &vo)?;

    rsleep![nc, 1];

    v.destroy();
    nc.stop()?;
    Ok(())
}
