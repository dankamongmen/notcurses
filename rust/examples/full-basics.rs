use libnotcurses_sys::*;

fn main() {
    unsafe {
        let nc = Notcurses::new();

        // use standard plane
        let stdplane = notcurses_stdplane(nc);

        for ch in "Initializing cells...".chars() {
            let cell = NcCell::with_7bitchar(ch);
            sleep![60];
            ncplane_putc(&mut *stdplane, &cell);
            let _ = notcurses_render(nc);
        }
        sleep![900];

        notcurses_stop(nc);
    }
}
