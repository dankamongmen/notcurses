//! Example 'direct-cursor'
//!
//! Explore cursor functions in direct mode
//!

use rand::{thread_rng, Rng};

use libnotcurses_sys::*;

fn main() -> NcResult<()> {
    let mut rng = thread_rng();

    let mut dm = NcDirect::new()?;

    let cols = dm.dim_x();
    let rows = dm.dim_y();
    println!("terminal size (rows, cols): {}, {}", rows, cols);

    let mut channels =
        NcChannelPair::combine(NcChannel::with_rgb(0xAA2244), NcChannel::with_rgb(0x112233));
    dm.putstr(channels, "The current coordinates are")?;

    for _n in 0..40 {
        fsleep![&mut dm, 0, 30];
        channels.set_fg_rgb8(
            rng.gen_range(0x66..=0xEE),
            rng.gen_range(0x66..=0xEE),
            rng.gen_range(0x66..=0xEE),
        );
        channels.set_bg_rgb8(
            rng.gen_range(0..=0x9),
            rng.gen_range(0..=0x9),
            rng.gen_range(0..=0x9),
        );
        dm.putstr(channels, ".")?;
    }

    let (cy, cx) = dm.cursor_yx()?;
    dm.putstr(channels, &format!(" ({},{})\n", cy, cx))?;
    sleep![1];

    let sentence = vec![
        "And", "now", "I", "will", "clear", "the", "screen", ".", ".", ".",
    ];
    for word in sentence {
        channels.set_fg_rgb(channels.fg_rgb().wrapping_sub(0x050505));
        channels.set_bg_rgb(channels.bg_rgb().wrapping_add(0x090909));
        dm.putstr(channels, &format!["{} ", word])?;
        fsleep![&mut dm, 0, 150];
    }
    sleep![0, 300];
    channels.set_fg_rgb(0xFFFFFF);
    channels.set_bg_default();
    dm.putstr(channels, "\nbye!\n\n")?;
    fsleep![&mut dm, 0, 600];
    dm.clear()?;
    Ok(())
}
