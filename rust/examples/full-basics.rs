use libnotcurses_sys::*;

fn main() -> NcResult<()> {
    let mut nc = FullMode::new()?;
    let stdplane = nc.stdplane()?;

    for ch in "Initializing cells...".chars() {
        let cell = NcCell::with_char7b(ch);
        stdplane.putc(&cell)?;
        rsleep![&mut nc, 0, 40];
    }
    sleep![0, 900];

    Ok(())
}
