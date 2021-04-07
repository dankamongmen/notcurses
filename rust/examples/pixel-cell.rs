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
    let mut nc = FullMode::new()?;

    if !nc.check_pixel_support()? {
        return Err(NcError::new_msg("Current terminal doesn't support pixels."));
    }

    // get the dimensions of the terminal in rows,cols & x,y pixels
    let mut ws: libc::winsize = unsafe { std::mem::zeroed() };
    unsafe { libc::ioctl(0, libc::TIOCGWINSZ, &mut ws) };
    let term_rows = ws.ws_row;
    let term_cols = ws.ws_col;
    let term_y = ws.ws_ypixel;
    let term_x = ws.ws_xpixel;
    // calculate the size of the cell in pixels
    let cell_y = (term_y / term_rows) as u32;
    let cell_x = (term_x / term_cols) as u32;
    // println!(
    //     "rows,cols={},{}; term y,x={},{}; cell y,x={},{}",
    //     term_rows, term_cols, term_y, term_x, cell_y, cell_x
    // );

    // print visual delimiters around our pixelized cell
    println!("0▗│▖\n│─ ─\n2▝│▘");
    println!("a cell is {}x{} pixels", cell_y, cell_x);

    // fill the buffer with random color pixels
    let mut rng = rand::thread_rng();
    let range = Uniform::from(50..=180);
    let mut buffer = Vec::<u8>::with_capacity((cell_y * cell_x * 4) as usize);
    #[allow(unused_parens)]
    for _byte in (0..={ cell_y * cell_x }) {
        buffer.push(rng.sample(&range));
        buffer.push(rng.sample(&range));
        buffer.push(rng.sample(&range));
        buffer.push(255);
    }

    // show the newly created ncvisual delimited with the box drawing characters
    let pixels = NcVisual::from_rgba(buffer.as_slice(), cell_y, cell_x * 4, cell_x)?;
    let voptions = NcVisualOptions::without_plane(1, 2, 0, 0, cell_y, cell_x, NCBLIT_PIXEL, 0);
    pixels.render(&mut nc, &voptions)?;

    // FIXME: segfaults
    // let mut stdplane = nc.stdplane();
    // let mut vplane = NcPlane::new_bound(&mut stdplane, 0, 0, 4, 4)?

    rsleep![&mut nc, 10];
    pixels.destroy();

    Ok(())
}
