//! based on the proof of concept at ../../src/poc/menu.c

use libnotcurses_sys::*;

fn main() -> NcResult<()> {
    let nc = Notcurses::new()?;
    nc.mouse_enable()?;

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

    let mut help_items = [NcMenuItem::new("About", NcInput::with_ctrl('a'))];

    let mut sections = [
        NcMenuSection::new("Schwarzgerät", &mut demo_items, NcInput::with_alt('ä')),
        NcMenuSection::new("File", &mut file_items, NcInput::with_alt('f')),
        NcMenuSection::new_separator(),
        NcMenuSection::new("Help", &mut help_items, NcInput::with_alt('h')),
    ];

    let mut mopts = NcMenuOptions::new(&mut sections);
    mopts.header_channels_mut().set_fg_rgb(0x00ff00);
    mopts.header_channels_mut().set_bg_rgb(0x440000);
    mopts.section_channels_mut().set_fg_rgb(0xb0d700);
    mopts.section_channels_mut().set_bg_rgb(0x002000);

    let plane = nc.stdplane()?;
    let (dim_y, _dim_x) = plane.dim_yx();
    let menu_top = NcMenu::new(plane, mopts)?;
    menu_top.item_set_status("Schwarzgerät", "Disabled", false)?;
    menu_top.item_set_status("Schwarzgerät", "Restart", false)?;

    let mut channels: NcChannelPair = 0;
    channels.set_fg_rgb(0x88aa00);
    channels.set_bg_rgb(0x000088);
    plane.set_base('x', 0, channels)?;

    nc.render()?;

    plane.set_fg_rgb(0x00dddd);
    plane.putstr_aligned(
        dim_y - 1,
        NCALIGN_RIGHT,
        " -=+ menu poc. press q to exit +=-",
    )?;
    run_menu(nc, menu_top)?;

    plane.erase();

    mopts.flags |= NCMENU_OPTION_BOTTOM;
    let menu_bottom = NcMenu::new(plane, mopts)?;
    // FIXME:
    plane.putstr_aligned(1, NCALIGN_RIGHT, " -=+ menu poc. press q to exit +=-")?;
    run_menu(nc, menu_bottom)?;

    nc.stop()?;
    Ok(())
}

fn run_menu(nc: &mut Notcurses, menu: &mut NcMenu) -> NcResult<()> {
    // yellow rectangle
    let planeopts = NcPlaneOptions::new_aligned(10, NCALIGN_CENTER, 3, 40);
    let stdplane = nc.stdplane()?;
    let selplane = NcPlane::with_options_bound(stdplane, planeopts)?;

    selplane.set_fg_rgb(0);
    selplane.set_bg_rgb(0xdddddd);
    let mut channels = 0;
    channels.set_fg_rgb(0x000088);
    channels.set_bg_rgb(0x88aa00);
    selplane.set_base(' ', 0, channels)?;

    let mut ni = NcInput::new_empty();
    let mut keypress: char;

    // FIXME: screen updates one keypress later.
    loop {
        keypress = nc.getc_blocking(Some(&mut ni))?;

        if !menu.offer_input(ni) {
            if keypress == 'q' {
                menu.destroy()?;
                selplane.destroy()?;
                return Ok(());
            } else if keypress == NCKEY_ENTER {
                // selected a menu item
                // BUG FIXME:
                let sel = menu.selected(Some(&mut ni))?;
                if sel == "Quit" {
                    menu.destroy()?;
                    selplane.destroy()?;
                    return Ok(());
                }
            }
        }

        let mut selni = NcInput::new_empty();
        // if let Some(selitem) = menu.selected(Some(&mut selni)) {
        //     selplane.putstr_aligned(1, NCALIGN_CENTER, &selitem)?;
        // } else {
        //     // DEBUG
        //     selplane.putstr_aligned(1, NCALIGN_CENTER, "nothing opened")?;
        // }

        nc.render()?;
    }
}
