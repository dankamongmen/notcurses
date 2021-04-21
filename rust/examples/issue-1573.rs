//! example to figure out issues with visuals and pixels
//!

use libnotcurses_sys::*;

// background size
const BG_W: u32 = 64;
const BG_H: u32 = 32;

// sprite size
const SP_W: u32 = 32;
const SP_H: u32 = 64;


fn main() -> NcResult<()> {
    let mut nc = Nc::new()?;

    let pixels: bool = nc.check_pixel_support()?;
    if !pixels {
        println!("Current terminal doesn't support pixels. . .");
        sleep![1];
    }

    // create the white background buffer
    let mut bg_buffer = Vec::<u8>::with_capacity(BG_H as usize * BG_W as usize * 4);
    #[allow(unused_parens)]
    for _byte in (0..={ BG_H * BG_W }) {
        bg_buffer.push(251);
        bg_buffer.push(245);
        bg_buffer.push(248);
        bg_buffer.push(255);
    }

    // create the red sprite buffer
    let mut sp_buffer = Vec::<u8>::with_capacity((SP_H * SP_W) as usize * 4);
    #[allow(unused_parens)]
    for _byte in (0..={ SP_H * SP_W }) {
        sp_buffer.push(130);
        sp_buffer.push(40);
        sp_buffer.push(60);
        sp_buffer.push(255);
    }

    // prepare the white background visual
    let bg = NcVisual::from_rgba(bg_buffer.as_slice(), BG_H, BG_W * 4, BG_W)?;
    let bg_opt = NcVisualOptions::without_plane(0, 0, 0, 0, BG_H, BG_W, NCBLIT_PIXEL, 0, 0);
    // if pixels {
    //     bg.inflate(2)?;
    // }

    // prepare the red sprite visual
    let sp = NcVisual::from_rgba(sp_buffer.as_slice(), SP_H, SP_W * 4, SP_W)?;
    let sp_opt = NcVisualOptions::without_plane(0, 0, 0, 0, SP_H, SP_W, NCBLIT_PIXEL, 0, 0);
    // if pixels {
    //     sp.inflate(2)?;
    // }

    // first render the white background in a new plane
    let bg_plane = bg.render(&mut nc, &bg_opt)?;
    // nc.render()?; // ← FIXME: when this render call is commented out,
                     // the red sprite doesn't show on top in xterm and kitty but
                     // does show in terminals without pixel support (xterm -ti vt100)

    // then render the red sprite in another plane
    let sp_plane = sp.render(&mut nc, &sp_opt)?;
    nc.render()?;

    sleep![2];
    Ok(())
}
