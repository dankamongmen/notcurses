//! Example 'direct-cursor'
//!
//! Explore cursor functions in direct mode
//!

use libnotcurses_sys::*;

fn main() -> NcResult<()> {
    unsafe {
        let ncd = NcDirect::new()?;

        let cols = ncdirect_dim_x(ncd);
        let rows = ncdirect_dim_y(ncd);
        println!("terminal size (rows, cols): {}, {}", rows, cols);

        ncd.putstr(0, "The current coordinates are")?;
        ncd.flush()?;

        for _n in 0..20 {
            ncd.putstr(0, ".")?;
            ncd.flush()?;
            sleep![50];
        }

        if let Some((cy, cx)) = ncd.cursor_yx() {
            ncd.putstr(0, &format!(" ({},{})\n", cy, cx))?;
        }
        sleep![1000];

        let sentence = vec!["And", "now", "I", "will", "clear", "the", "screen", ".", ".", "."];
        for word in sentence {
            ncd.putstr(0, &format!["{} ", word])?;
            ncd.flush()?;
            sleep![200];
        }
        sleep![300];
        ncd.putstr(0, "\nbye!\n\n")?;
        ncd.flush()?;
        sleep![600];

        ncd.clear()?;
        sleep![1000];

        ncd.stop()?;
    }
    Ok(())
}
