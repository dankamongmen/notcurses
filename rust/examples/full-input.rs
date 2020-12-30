//! Input
//!
//! https://github.com/dankamongmen/notcurses/blob/master/USAGE.md#input
//!

use libnotcurses_sys::*;

fn main() -> NcResult<()> {
    let nc = Notcurses::with_flags(
        NCOPTION_SUPPRESS_BANNERS
        | NCOPTION_NO_WINCH_SIGHANDLER
        | NCOPTION_NO_QUIT_SIGHANDLERS
        )?;

    // doesn't seem to be necessary:
    // let ready = unsafe { notcurses_inputready_fd(nc) };
    // println!("{}", ready);

    println!("Exit with F1\n");

    let mut input = NcInput::new();

    loop {
        let key = notcurses_getc_nblock(nc, &mut input);

        if key as i32 != -1 {
            println!("'{0}' ({1:x})\n{2:?}", key, key as u32, input);
        }

        rsleep![nc, 0, 10];

        match key {
            NCKEY_F01 => break,
            _ => (),
        }
    }

    println!("\nExiting...");

    rsleep![nc, 1, 500];
    nc.stop()?;
    Ok(())
}
