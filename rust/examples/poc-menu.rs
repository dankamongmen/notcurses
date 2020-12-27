//! based on the proof of concept at ../../src/poc/menu.c

use libnotcurses_sys::*;

fn main() -> NcResult<()> {
    //let nc = Notcurses::new()?;
    let nc = Notcurses::new()?;
    nc.mouse_enable()?;

    //let demo_items = vec![
    let mut demo_items = [
        NcMenuItem::new("Restart", NcInput::with_ctrl('r')),
        NcMenuItem::new("Disabled", NcInput::with_ctrl('d')),
    ];
    let mut file_items = [
        NcMenuItem::new("New", NcInput::with_ctrl('n')),
        NcMenuItem::new("Open", NcInput::with_ctrl('o')),
        NcMenuItem::new("Close", NcInput::with_ctrl('c')),
        NcMenuItem::new_empty(),
        NcMenuItem::new("Quit", NcInput::with_ctrl('q')),
    ];

    let mut help_items = [
        NcMenuItem::new("About", NcInput::with_ctrl('a')),
    ];

    let mut sections = [
        NcMenuSection::new("Schwarzgerät", &mut demo_items, NcInput::with_alt('ä')),
        NcMenuSection::new("File", &mut file_items, NcInput::with_alt('f')),
        //NcMenuSection::new_empty(),
        NcMenuSection::new("Help", &mut help_items, NcInput::with_alt('h')),
    ];

    let mut mopts = NcMenuOptions::new(&mut sections);
    mopts.header_channels_mut().set_fg_rgb(0x00ff00);
    mopts.header_channels_mut().set_bg_rgb(0x440000);
    mopts.section_channels_mut().set_fg_rgb(0xb0d700);
    mopts.section_channels_mut().set_bg_rgb(0x002000);

    let (mut dimy, mut dimx) = (0, 0);
    let plane = nc.stddim_yx(&mut dimy, &mut dimx)?;
    let top = NcMenu::new(plane, mopts)?;
    top.item_set_status("Schwarzgerät", "Disabled", false)?;
    top.item_set_status("Schwarzgerät", "Restart", false)?;

    let mut channels: NcChannelPair = 0;
    channels.set_fg_rgb(0x88aa00);
    channels.set_fg_rgb(0x000088);
    plane.set_base('x', 0, channels)?;

    //nc.render()?;

    nc.stop()?;
    Ok(())
}

// fn run_menu(nc: &mut Notcurses, menu: &mut NcMenu) -> NcResult<()> {
//     let nopts = NcPlaneOptions::new_aligned(10, NCALIGN_CENTER, 3, 40);
//     let selplane = NcPlane::with_options(nc, nopts)?;
//     Ok(())
// }
