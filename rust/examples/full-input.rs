//! Input
//!
//! https://github.com/dankamongmen/notcurses/blob/master/USAGE.md#input
//!

use libnotcurses_sys::*;

fn main() -> NcResult<()> {
    let mut nc = Nc::with_flags(
        NCOPTION_SUPPRESS_BANNERS | NCOPTION_NO_WINCH_SIGHANDLER | NCOPTION_NO_QUIT_SIGHANDLERS,
    )?;

    println!("Exit with F1\n");

    let mut input = NcInput::new_empty();

    loop {
        let key = notcurses_getc_nblock(&mut nc, &mut input);

        if key as i32 != -1 {
            println!("'{0}' ({1:x})\n{2:?}", key, key as u32, input);
        }

        rsleep![&mut nc, 0, 10];

        match key {
            NCKEY_F01 => break,
            _ => (),
        }
    }

    println!("\nExiting...");

    rsleep![&mut nc, 1];
    nc.stop()?;
    Ok(())
}
