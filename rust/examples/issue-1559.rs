//! https://github.com/dankamongmen/notcurses/issues/1559

use libnotcurses_sys::*;

const WIDTH: u32 = 10;
const HEIGHT: u32 = 10;

fn main() -> NcResult<()> {
    let mut nc = Nc::new()?;

    if !nc.check_pixel_support()? {
        return Err(NcError::new_msg("Current terminal doesn't support pixels."));
    }

    let mut buffer = Vec::<u8>::with_capacity((HEIGHT * WIDTH * 4) as usize);
    for _byte in (0..={ HEIGHT * WIDTH }) {
        buffer.push(230);
        buffer.push(30);
        buffer.push(40);
        buffer.push(255);
    }

    let vframe = NcVisual::from_rgba(buffer.as_slice(), HEIGHT, WIDTH * 4, WIDTH)?;
    let voptions =
        NcVisualOptions::without_plane(0, 0, 0, 0, HEIGHT, WIDTH, NCBLIT_PIXEL, 0, 0);

    vframe.resize(1, 1)?; // it doesn't matter which values we put here 

    // FIXME: render always gives an error
    vframe.render(&mut nc, &voptions)?;

    rsleep![&mut nc, 2];
    vframe.destroy();
    Ok(())
}
