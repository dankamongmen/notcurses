//! Example 'direct-cursor'
//!
//! Explore cursor functions in direct mode
//!

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
        let ncd = NcDirect::new();

        let cols = ncdirect_dim_x(ncd);
        let rows = ncdirect_dim_y(ncd);
        println!("terminal size (rows, cols): {}, {}", rows, cols);

        // show current coordinates
        let (mut cy, mut cx) = (0, 0);
        ncdirect_cursor_yx(ncd, &mut cy, &mut cx);
        ncdirect_putstr(ncd, 0, cstring![format!(" ({},{})\n", cy, cx)],
        );

        sleep![1];

        ncdirect_putstr(ncd, 0, cstring!["HELLO"]);
        ncdirect_flush(ncd);

        sleep![1];

        ncdirect_putstr(ncd, 0, cstring!["HELLO"]);
        ncdirect_flush(ncd);

        sleep![2];

        // show current coordinates
        ncdirect_cursor_yx(ncd, &mut cy, &mut cx);
        ncdirect_putstr( ncd, 0, cstring![format!(" ({},{})\n", cy, cx)],
        );

        sleep![1];
        ncdirect_stop(ncd);
    }
}
