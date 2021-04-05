//! sixel example
//!
//! used to determine the number of pixels per cell, which is relative to font size

use libnotcurses_sys::*;

use rand::{distributions::Uniform, Rng};

fn main() -> NcResult<()> {
    let mut nc = FullMode::new()?;

    if !nc.check_pixel_support()? {
        return Err(NcError::new_msg("Current terminal doesn't support pixels."));
    }

    // the dimensions of the terminal
    // let (trows, tcols) = nc.term_dim_yx();

    // choose the size of the cell in pixels
    let pixels_w = 10;
    let pixels_h = 19;

    // print visual delimiters around our pixelized cell
    println!("0▗│▖\n│─ ─...\n2▝│▘");
    println!("pixels per cell: {}x{}", pixels_w, pixels_h);

    // fill the buffer with random color pixels
    let mut rng = rand::thread_rng();
    let range = Uniform::from(50..=180);
    let mut buffer = Vec::<u8>::with_capacity((pixels_w*pixels_h*4) as usize);
    #[allow(unused_parens)]
    for _byte in (0..={pixels_w*pixels_h}) {
        buffer.push(rng.sample(&range));
        buffer.push(rng.sample(&range));
        buffer.push(rng.sample(&range));
        buffer.push(255);
    }

    let pixels = NcVisual::from_rgba(buffer.as_slice(), pixels_h, pixels_w*4, pixels_w)?;
    let voptions = NcVisualOptions::without_plane(1, 2, 0, 0, pixels_h, pixels_w, NCBLIT_PIXEL, 0);
    pixels.render(&mut nc, &voptions)?;

    rsleep![&mut nc, 10];

    pixels.destroy();

    Ok(())
}
