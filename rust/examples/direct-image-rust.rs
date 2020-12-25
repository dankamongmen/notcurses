//! Example 'direct-image'
//!
//! Explore image rendering in direct mode
//!
//! NOTE: This example uses the Rust style with methods.

use libnotcurses_sys::*;

fn main() -> NcResult<()> {
        let ncd = NcDirect::new()?;

        render_image(ncd, NCBLIT_1x1)?;
        render_image(ncd, NCBLIT_2x1)?;
        render_image(ncd, NCBLIT_BRAILLE)?;

        ncd.stop()?;
        Ok(())
}

fn render_image(ncd: &mut NcDirect, blit: NcBlitter) -> NcResult<()>{
    if let Err(nc_error) = ncd.render_image(
        "image-16x16.png",
        NCALIGN_CENTER,
        blit,
        NCSCALE_NONE,
    ) {
        return Err(NcError::with_msg(nc_error.int,
            "ERROR: ncdirect_render_image(). Make sure you \
            are running this example from the examples folder"));
    }
    Ok(())
}
