use libnotcurses_sys::*;

fn main() -> NcResult<()> {
    let dm = NcDirect::new()?;

    let (t_rows, t_cols) = dm.dim_yx();
    println!("Terminal rows={0}, cols={1}", t_rows, t_cols);

    println!(
        "Can display UTF-8: {0}
Can open images: {1}
Supports Pixels: {2:?}
Palette size: {3:?}
",
        dm.canutf8(),
        dm.canopen_images(),
        dm.check_pixel_support(),
        dm.palette_size(),
    );

    Ok(())
}
