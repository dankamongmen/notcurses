//! Example 'direct-image'
//!
//! Explore image rendering in direct mode
//!
//! NOTE: This example uses the Rust style with methods.

use libnotcurses_sys::*;

fn main() -> NcResult<()> {
    let mut dm = DirectMode::new()?;

    render_image(&mut dm, NCBLIT_1x1)?;
    render_image(&mut dm, NCBLIT_2x1)?;
    render_image(&mut dm, NCBLIT_BRAILLE)?;

    Ok(())
}

fn render_image(dm: &mut DirectMode, blit: NcBlitter) -> NcResult<()> {
    if let Err(nc_error) = dm.render_image("image-16x16.png", NCALIGN_CENTER, blit, NCSCALE_NONE) {
        return Err(NcError::with_msg(
            nc_error.int,
            "ERROR: dmirect_render_image(). Make sure you \
            are running this example from the examples folder",
        ));
    }
    Ok(())
}
