//use std::ffi::Cstring;

use libnotcurses_sys as nc;

fn main() {

    unsafe {
        // let options = nc::NotcursesOptions::new();
        // let app = nc::Notcurses::with_options(&options);

        let app = nc::Notcurses::new();



        nc::notcurses_stop(app);
    }
}

