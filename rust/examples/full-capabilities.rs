use libnotcurses_sys::*;

fn main() -> NcResult<()> {
    let mut nc = Nc::without_altscreen()?;

    let (t_rows, t_cols) = nc.term_dim_yx();
    println!("Terminal rows={0}, cols={1}", t_rows, t_cols);

    println!(
        "Can display UTF-8: {0}
Can display braille characters: {1}
Can display sextant characters: {2}
Can open images: {3}
Can open videos: {4}
Supports Pixels: {5:?}
Supports True Color: {6}
Palette size: {7:?}
",
        nc.canutf8(),
        nc.canbraille(),
        nc.cansextant(),
        nc.canopen_images(),
        nc.canopen_videos(),
        nc.check_pixel_support(),
        nc.cantruecolor(),
        nc.palette_size(),
    );

    let pixelgeom = nc.stdplane().pixelgeom();
    println!("{:#?}", pixelgeom);

    nc.render()?;
    Ok(())
}
