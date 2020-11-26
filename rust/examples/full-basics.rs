// utility macro: sleep for $ms milliseconds
macro_rules! sleep {
    ($ms:expr) => {
        std::thread::sleep(std::time::Duration::from_millis($ms));
    };
}

// utility macro: convert the String $s to *mut CString
// macro_rules! cstring {
//     ($s:expr) => {
//         std::ffi::CString::new($s).unwrap().as_ptr();
//     }
// }

use libnotcurses_sys::*;

fn main() {
    unsafe {
        let nc = Notcurses::new();

        // use standard plane
        let stdplane = notcurses_stdplane(nc);

        for ch in "Initializing cells...".chars() {
            let cell = NcCell::with_char(ch);
            sleep![60];
            ncplane_putc(&mut *stdplane, &cell);
            let _ = notcurses_render(nc);
        }
        sleep![900];

        notcurses_stop(nc);
    }
}
