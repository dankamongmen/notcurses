use libnotcurses_sys::*;

// BUGFIX DISCOVERINGS so far:
//
// - using a Height of 32, it only works with Widths: 1-18, 22-34, 43-50, …
// - using a Height of 4, it only works with Widths: 1-34, 43-50, …

const H: NcDim = 4;
const W: NcDim = 35;

fn main() -> NcResult<()> {
    let mut nc = Nc::without_altscreen()?;
    let mut plane = NcPlane::new(&mut nc, 0, 0, H, W)?;

    let buffer_rgb = vec![255; H as usize * W as usize * 3];

    let visual = NcVisual::from_rgb_packed(buffer_rgb.as_slice(), H, W * 3, W, 0xFE)?;
    let voptions = NcVisualOptions::with_plane(
        &mut plane,
        NCSCALE_NONE,
        0,
        0,
        0,
        0,
        0,
        0,
        NCBLIT_PIXEL,
        0,
        0,
    );
    visual.render(nc, &voptions)?;
    plane.render()?;
    plane.rasterize()?;
    sleep![1];
    plane.destroy()?;
    visual.destroy();
    nc.stop()?;
    Ok(())
}
