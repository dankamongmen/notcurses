//! Example 'direct-cursor'
//!
//! Explore cursor functions in direct mode
//!

use libnotcurses_sys::*;

fn main() {
    unsafe {
        let ncd = NcDirect::new();

        let cols = ncdirect_dim_x(ncd);
        let rows = ncdirect_dim_y(ncd);
        println!("terminal size (rows, cols): {}, {}", rows, cols);

        ncdirect_putstr(ncd, 0, cstring![format!("The current coordinates are")]);
        ncdirect_flush(ncd);

        for _n in 0..20 {
            ncdirect_putstr(ncd, 0, cstring!("."));
            ncdirect_flush(ncd);
            sleep![50];
        }

        let (mut cy, mut cx) = (0, 0);
        ncdirect_cursor_yx(ncd, &mut cy, &mut cx);
        ncdirect_putstr(ncd, 0, cstring![format!(" ({},{})\n", cy, cx)]);
        sleep![1000];

        let sentence = vec!["And", "now", "I", "will", "clear", "the", "screen", ".", ".", "."];
        for word in sentence {
            ncdirect_putstr(ncd, 0, cstring!(format!["{} ", word]));
            ncdirect_flush(ncd);
            sleep![200];
        }
        sleep![300];
        ncdirect_putstr(ncd, 0, cstring!("\nbye!\n\n"));
        ncdirect_flush(ncd);
        sleep![600];

        ncdirect_clear(ncd);
        sleep![1000];

        ncdirect_stop(ncd);
    }
}
