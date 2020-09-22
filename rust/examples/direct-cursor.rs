use std::thread::sleep;
use std::time::Duration;

use cstr_core::CString;

use libnotcurses_sys as nc;

fn main() {
    unsafe {
        let ncd = nc::ncdirect_start();

        let cols = nc::ncdirect_dim_x(ncd);
        let rows = nc::ncdirect_dim_y(ncd);
        println!("terminal size (rows, cols): {}, {}", rows, cols);

        // show current coordinates
        let (mut cy, mut cx) = (0,0);
        nc::ncdirect_cursor_yx(ncd, &mut cy, &mut cx);
        nc::ncdirect_putstr(ncd, 0, CString::new(format!("({},{})\n", cy, cx)).unwrap().as_ptr());

        // Write HELLO WORLD in steps

        sleep(Duration::new(1, 0));

        nc::ncdirect_putstr(ncd, 0, CString::new("HELLO").unwrap().as_ptr());
        nc::ncdirect_flush(ncd);

        sleep(Duration::new(1, 0));

        nc::ncdirect_putstr(ncd, 0, CString::new(" WORLD").unwrap().as_ptr());
        nc::ncdirect_flush(ncd);

        sleep(Duration::new(1, 0));

        // show current coordinates
        nc::ncdirect_cursor_yx(ncd, &mut cy, &mut cx);
        nc::ncdirect_putstr(ncd, 0, CString::new(format!(" ({},{})\n", cy, cx)).unwrap().as_ptr());

        sleep(Duration::new(1, 0));
        nc::ncdirect_stop(ncd);
    }
}
