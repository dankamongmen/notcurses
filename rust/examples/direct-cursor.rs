use std::thread::sleep;
use std::time::Duration;

use cstr_core::CString;

use libnotcurses_sys as nc;

fn main() {
    unsafe {
        let ncd = nc::ncdirect_start();

        let (mut cy, mut cx) = (0,0);
        nc::ncdirect_cursor_yx(ncd, &mut cy, &mut cx);
        nc::ncdirect_putstr(ncd, 0, CString::new(format!("({},{})\n", cy, cx)).unwrap().as_ptr());

        // ISSUE 1: printing without a newline doesn't update the screen
        sleep(Duration::new(2, 0));
        nc::ncdirect_putstr(ncd, 0, CString::new("HELLO").unwrap().as_ptr());
        sleep(Duration::new(2, 0));
        nc::ncdirect_putstr(ncd, 0, CString::new("WORLD").unwrap().as_ptr());

        nc::ncdirect_cursor_yx(ncd, &mut cy, &mut cx);
        nc::ncdirect_putstr(ncd, 0, CString::new(format!("({},{})\n", cy, cx)).unwrap().as_ptr());

        nc::ncdirect_stop(ncd);
    }
}
