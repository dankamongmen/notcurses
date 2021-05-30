// https://github.com/dankamongmen/notcurses/issues/1699
// https://github.com/dankamongmen/notcurses/issues/1700

use libnotcurses_sys::*;

const W: u32 = 16;
const H: u32 = 8;

fn main() -> NcResult<()> {
    let nc = Notcurses::without_altscreen()?;

    println!("pixel support: {:?}", nc.check_pixel_support());

    // create a purple rectangle
    let mut buffer = Vec::<u8>::with_capacity(H as usize * W as usize * 4);
    #[allow(unused_parens)]
    for _byte in (0..={ H * W }) {
        buffer.push(190);
        buffer.push(20);
        buffer.push(80);
        buffer.push(255);
    }

    let vframe1 = NcVisual::from_bgra(&buffer, H, W * 4, W)?;

    // BUG: draws vertical stripes
    let voptions = NcVisualOptions::without_plane(0, 0, 0, 0, H, W, NCBLIT_1x1, 0, 0);
    // BUG: nothing gets drawn
    // let voptions = NcVisualOptions::without_plane(0, 0, 0, 0, H, W, NCBLIT_PIXEL, 0, 0);

    vframe1.render(nc, &voptions)?;

    nc.render()?;
    vframe1.destroy();
    nc.stop()?;
    Ok(())
}
