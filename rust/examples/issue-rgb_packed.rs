use libnotcurses_sys::*;

// BUGFIX DISCOVERINGS so far:
//
// - Height seems to be irrelevant (reference: 32)
// - using the C API only works with Widths: 1-18, 22-34, 43-50

const H: NcDim = 32;
const W: NcDim = 35;

fn main() -> NcResult<()> {
    let mut nc = Nc::new()?;
    let mut plane = NcPlane::new(&mut nc, 0, 0, H, W)?;

    let buffer_rgb = vec![255; H as usize * W as usize * 3];

    let visual = NcVisual::from_rgb_packed(buffer_rgb.as_slice(), H, W * 3, W, 255)?;
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
    plane.destroy();
    visual.destroy();
    nc.stop()?;
    Ok(())
}
