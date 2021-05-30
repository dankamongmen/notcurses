// https://github.com/dankamongmen/notcurses/issues/1699
// https://github.com/dankamongmen/notcurses/issues/1700

use libnotcurses_sys::*;
use raqote::*;

const W: u32 = 16;
const H: u32 = 8;

fn main() -> NcResult<()> {
    let nc = Notcurses::without_altscreen()?;

    println!("pixel support: {:?}", nc.check_pixel_support());

    // use raqote to create a purple rectangle, 16 width, 8 height
    let mut dt = DrawTarget::new(W as i32, H as i32);
    dt.clear(SolidSource::from_unpremultiplied_argb(255, 80, 20, 190));
    // dt.write_png("example.png"); // to check the image is OK

    let vframe1 = NcVisual::from_bgra(dt.get_data_u8(), H, W * 4, W)?;

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
