#![allow(unused_imports)]

use libnotcurses_sys::*;

fn main() -> NcResult<()> {
    let mut dm = DirectMode::new()?;

    // INFO

    let t_rows = dm.dim_x();
    let t_cols = dm.dim_y();
    println!("Terminal rows={0}, cols={1}", t_rows, t_cols);

    println!(
        "Can open images: {0}\nCan UTF-8: {1}\nSupports Pixels: {2:?}",
        dm.canopen_images(),
        dm.canutf8(),
        dm.check_pixel_support()?,
    );

    println!("palette_size: {}", dm.palette_size()?);

    // TEXT & STYLE

    // let stylesv = vec![
    //     ("[DIM]", Style::Dim),
    //     ("[UNDERLINE]", Style::Underline),
    //     ("[ITALIC]", Style::Italic),
    //     ("[BOLD]", Style::Bold),
    //     ("[STRUCK]", Style::Struck),
    //     ("[REVERSE]", Style::Reverse),
    //     ("[BLINK]", Style::Blink),
    //     ("[INVIS]", Style::Invis),
    //     ("[PROTECT]", Style::Protect),
    //     ("[STANDOUT]", Style::Standout),
    // ];
    //
    // dm.print_colored(0, "\nSingle styles:\n")?;
    //
    // dm.print_colored(0, "[DEFAULT]")?;
    // for (label, style) in stylesv.iter() {
    //     dm.styles_on(*style)?;
    //     dm.print_colored(0, label)?;
    //     dm.styles_off(*style)?;
    // }
    //
    // dm.print_colored(0, "\nJoint styles:\n")?;
    //
    // dm.print_colored(0, "[DEFAULT ")?;
    // for (label, style) in stylesv.iter() {
    //     dm.styles_on(*style)?;
    //     dm.print_colored(
    //         0,
    //         &label
    //             .chars()
    //             .map(|c| match c {
    //                 '[' | ']' => ' ',
    //                 _ => c,
    //             })
    //             .collect::<String>(),
    //     )?;
    //     if let Style::Blink = style {
    //         break;
    //     }
    // }
    // dm.styles_off_all()?;
    // dm.print_colored(0, "]")?;
    //
    // // TEXT mixing Rust's print!() & println!() and notcurses' print_colored() & print()
    // //
    // dm.print_colored(0, "\n\n1")?;
    // println!("2 < instead of printing this concatenated AFTER, it appears BEFORE 1");
    //
    // dm.print_colored(0, "\n\n1 \n")?;
    // println!("2 < it does work (better) with a `\\n` after 1");
    //
    // // TODO: more tests with styles_set & bold+italic
    // //
    // //dm.styles_off(Style::Bold)?;
    // //dm.styles_on(Style::Italic)?;
    //
    // // COLORS & TEXT (WIP)
    //
    // dm.bg(0x00FF00 as u32)?; // FIXME: colors don't seem to work
    // dm.fg(0xFF0000 as u32)?;
    // println!("\nhello colors? (investigate)");
    // dm.print_colored(
    //     sys::channels_combine(0xFF008800, 0xFFBB0099),
    //     "hello colors 2",
    // )?;
    // dm.print_colored(0, "...")?;

    // TODO: should be able to use print!() & println!()
    // dm.clear()?;
    // dm.print_aligned(0, Align::Center, "PRINTED")?;
    // dm.print_aligned(40, Align::Left, "PRINTED")?;
    // dm.print_aligned(5, Align::Right, "PRINTED")?;

    // WIP----------------------- ↓

    // CURSOR & TEXT

    // println!("Cursor position: {:?}", dm.cursor_yx()?);
    // dm.cursor_move_yx(200,100)?;
    // dm.cursor_move_yx(yx.0, yx.1)?;
    // dm.cursor_disable()?;
    // dm.cursor_enable()?;

    Ok(())
}
