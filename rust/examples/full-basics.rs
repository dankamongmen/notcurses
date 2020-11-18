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
        let stdplane = notcurses_stdplane(nc);

        let c1 = cell_char_initializer!('A');
        ncplane_putc(&mut *stdplane, &c1);

        let _ = notcurses_render(nc);

        sleep![1200];
        notcurses_stop(nc);
    }
}
