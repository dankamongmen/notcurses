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

    // BUG: when using from_bgra() it will draw vertical stripes 
    let vframe1 = NcVisual::from_bgra(&buffer, H, W * 4, W)?;
    // from_rgba() does work well!
    // let vframe1 = NcVisual::from_rgba(&buffer, H, W * 4, W)?;

    let voptions = NcVisualOptions::without_plane(0, 0, 0, 0, H, W, NCBLIT_1x1, 0, 0);

    // BUG: nothing gets drawn when using NCBLIT_PIXEL
    // let voptions = NcVisualOptions::without_plane(0, 0, 0, 0, H, W, NCBLIT_PIXEL, 0, 0);

    vframe1.render(nc, &voptions)?;

    nc.render()?;
    vframe1.destroy();
    nc.stop()?;
    Ok(())
}
