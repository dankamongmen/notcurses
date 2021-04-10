use libnotcurses_sys::*;

fn main() -> NcResult<()> {
    let mut nc = FullMode::without_altscreen()?;

    let (t_rows, t_cols) = nc.term_dim_yx();
    println!("Terminal rows={0}, cols={1}", t_rows, t_cols);

    println!(
        "Can display UTF-8: {0}
Can display sextant characters: {1}
Can open images: {2}
Can open videos: {3}
Supports Pixels: {4:?}
Supports True Color: {5}
Palette size: {6:?}
",
        nc.canutf8(),
        nc.cansextant(),
        nc.canopen_images(),
        nc.canopen_videos(),
        nc.check_pixel_support(),
        nc.cantruecolor(),
        nc.palette_size(),
    );

    let pixelgeom = nc.stdplane().pixelgeom();
    println!("{:#?}", pixelgeom);

    rsleep![&mut nc, 1];
    Ok(())
}
