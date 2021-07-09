//! pixel-cell example
//!
//! Shows how to get the size of a cell in pixels
//!
//! It works on the following terminals:
//! - kitty
//! - xterm (invoked with `xterm -ti vt340`)
//! - alacritty (WIP: https://github.com/ayosec/alacritty/tree/graphics)

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
    println!("\ninterpolated  not-interpolated  not-interpolated  interpolated");
    println!("   SCALE          SCALE               RESIZE          RESIZE");

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
    let v1 = NcVisual::from_rgba(buffer.as_slice(), pg.cell_y, pg.cell_x * 4, pg.cell_x)?;
    let voptions =
        NcVisualOptions::without_plane(1, 2, 0, 0, pg.cell_y, pg.cell_x, NCBLIT_PIXEL, 0, 0);
    v1.render(&mut nc, &voptions)?;
    nrs![&mut nc, 1];

    // show the ncvisual, scaled with interpolated values
    let mut vplane2 = NcPlane::new_bound(&mut stdplane, 7, 4, 5, 4)?;
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
    v1.render(&mut nc, &voptions2)?;
    nrs![&mut nc, 0, 250];

    // show the ncvisual, scaled without using interpolation
    let mut vplane3 = NcPlane::new_bound(&mut stdplane, 7, 19, 5, 4)?;
    let voptions3 = NcVisualOptions::with_plane(
        &mut vplane3,
        NCSCALE_SCALE,
        0,
        0,
        0,
        0,
        pg.cell_y,
        pg.cell_x,
        NCBLIT_PIXEL,
        NCVISUAL_OPTION_NOINTERPOLATE,
        0,
    );
    v1.render(&mut nc, &voptions3)?;
    nrs![&mut nc, 0, 250];

    // resize the ncvisual (doesn't use interpolation)
    let voptions4 =
        NcVisualOptions::without_plane(7, 39, 0, 0, pg.cell_y, pg.cell_x, NCBLIT_PIXEL, 0, 0);
    v1.resize_noninterpolative(pg.cell_y * 4, pg.cell_x * 4)?;
    v1.render(&mut nc, &voptions4)?;
    nrs![&mut nc, 0, 250];

    // resize the ncvisual (uses interpolation)
    let v5 = NcVisual::from_rgba(buffer.as_slice(), pg.cell_y, pg.cell_x * 4, pg.cell_x)?;
    let voptions5 =
        NcVisualOptions::without_plane(7, 56, 0, 0, pg.cell_y, pg.cell_x, NCBLIT_PIXEL, 0, 0);
    v5.resize(pg.cell_y * 4, pg.cell_x * 4)?;
    v5.render(&mut nc, &voptions5)?;
    nrs![&mut nc, 0, 250];

    sleep![2];

    v1.destroy();
    v5.destroy();
    nc.stop()?;
    Ok(())
}
