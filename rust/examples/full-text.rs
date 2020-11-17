use std::thread::sleep;
use std::time::Duration;
use std::ffi::CString;

/// utility macro: sleep for $s seconds
macro_rules! sleep {
    ($s:expr) => {
        sleep(Duration::new($s, 0));
    };
}

/// utility macro: convert the String $s to *mut CString
macro_rules! cstring {
    ($s:expr) => {
        CString::new($s).unwrap().as_ptr();
    }
}

use libnotcurses_sys::*;

fn main() {
    unsafe {
        let nc = Notcurses::new();

        println!("WIP");
        sleep![2];

        notcurses_stop(nc);
    }
}

