use libnotcurses_sys::*;

fn main() -> NcResult<()> {
    let mut nc = Nc::without_altscreen()?;

    let (t_rows, t_cols) = nc.term_dim_yx();
    println!("Terminal rows={0}, cols={1}", t_rows, t_cols);

    println!(
        "Can display UTF-8: {0}
Can display braille characters: {1}
Can display sextant characters: {2}
Can display quadrant characters: {3}
Can display half block characters: {4}
Can open images: {5}
Can open videos: {6}
Supports Pixels: {7:?}
Supports True Color: {8}
Supports fading: {9}
Supports changing the palette: {10}
Palette size: {11:?}
",
        nc.canutf8(),
        nc.canbraille(),
        nc.cansextant(),
        nc.canquadrant(),
        nc.canhalfblock(),
        nc.canopen_images(),
        nc.canopen_videos(),
        nc.check_pixel_support(),
        nc.cantruecolor(),
        nc.canfade(),
        nc.canchangecolor(),
        nc.palette_size(),
    );

    let pixelgeom = nc.stdplane().pixelgeom();
    println!("{:#?}", pixelgeom);

    nc.render()?;
    Ok(())
}
