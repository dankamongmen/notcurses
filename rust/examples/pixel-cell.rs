//! pixel-cell example
//!
//! Shows how to get the size of a cell in pixels
//!
//! It works on the following terminals:
//! - kitty
//! - xterm (invoked with `xterm -ti vt340`)
//! - alacritty (WIP https://github.com/ayosec/alacritty/tree/graphics)

use rand::{distributions::Uniform, Rng};

use libnotcurses_sys::*;

fn main() -> NcResult<()> {
    let mut nc = Nc::new()?;

    if !nc.check_pixel_support()? {
        return Err(NcError::new_msg("Current terminal doesn't support pixels."));
    }

    let mut stdplane = nc.stdplane();
    let pg = stdplane.pixelgeom();

    // print visual delimiters around our pixelized cell
    println!("0▗│▖\n│─ ─\n2▝│▘");
    println!("a cell is {}x{} pixels", pg.cell_y, pg.cell_x);

    // fill the buffer with random color pixels
    let mut rng = rand::thread_rng();
    let range = Uniform::from(50..=180);
    let mut buffer = Vec::<u8>::with_capacity((pg.cell_y * pg.cell_x * 4) as usize);
    #[allow(unused_parens)]
    for _byte in (0..={ pg.cell_y * pg.cell_x }) {
        buffer.push(rng.sample(&range));
        buffer.push(rng.sample(&range));
        buffer.push(rng.sample(&range));
        buffer.push(255);
    }

    // show the newly created ncvisual delimited with the box drawing characters
    let mut pixels = NcVisual::from_rgba(buffer.as_slice(), pg.cell_y, pg.cell_x * 4, pg.cell_x)?;
    let voptions =
        NcVisualOptions::without_plane(1, 2, 0, 0, pg.cell_y, pg.cell_x, NCBLIT_PIXEL, 0, 0);
    pixels.render(&mut nc, &voptions)?;
    rsleep![&mut nc, 1];

    // show the ncvisual, scaled
    let mut vplane2 = NcPlane::new_bound(&mut stdplane, 4, 4, 4, 4)?;
    let voptions2 = NcVisualOptions::with_plane(
        &mut vplane2,
        NCSCALE_SCALE,
        0,
        0,
        0,
        0,
        pg.cell_y,
        pg.cell_x,
        NCBLIT_PIXEL,
        0,
        0,
    );
    pixels.render(&mut nc, &voptions2)?;
    rsleep![&mut nc, 1];

    // show the ncvisual, inflated
    let voptions3 = NcVisualOptions::without_plane(
        4,
        10,
        0,
        0,
        pg.cell_y,
        pg.cell_x,
        NCBLIT_PIXEL,
        0,
        0,
    );
    pixels.inflate(2)?; // FIXME doesn't work (try different values)
    pixels.render(&mut nc, &voptions3)?;
    rsleep![&mut nc, 2];

    pixels.destroy();

    Ok(())
}
