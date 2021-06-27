//! https://github.com/dankamongmen/notcurses/issues/1832

use libnotcurses_sys::*;

const W: u32 = 32;
const H: u32 = 32;

fn main() -> NcResult<()> {
    let mut nc = Nc::new()?;


    // create a white rectangle visual for the background
    let buffer1 = vec![255; H as usize * W as usize * 3];
    let mut bg_plane = NcPlane::new(&mut nc, 0, 0, H, W)?;
    let v = NcVisual::from_rgb_packed(buffer1.as_slice(), H, W * 3, W, 255)?;
    let vo = NcVisualOptions::with_plane(&mut bg_plane, NCSCALE_NONE, 0, 0, 0, 0, 0, 0, NCBLIT_PIXEL, 0, 0);

    // create a blue plane for the foreground
    let mut fg_plane = NcPlane::new_bound(&mut bg_plane, 1, 1, 2, 16)?;
    fg_plane.set_base(" ", 0, NcChannels::from_rgb(0x88aa00, 0x222288))?;

    let mut counter = 0;
    for _ in 0..4 {
        fg_plane.putstr_yx(0,0, &format!["counter: {}", &counter]);
        counter += 1;

        v.render(nc, &vo)?;
        bg_plane.render()?;
        bg_plane.rasterize()?;
        sleep![0, 500];
    }


    sleep![1];
    v.destroy();
    bg_plane.destroy();
    nc.stop()?;
    Ok(())
}
