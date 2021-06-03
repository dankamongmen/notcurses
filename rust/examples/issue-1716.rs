use libnotcurses_sys::*;

const W: u32 = 32;
const H: u32 = 32;

fn main() {
    let nc = NcDirect::new().expect("couldn’t create ncdirect");
    nc.check_pixel_support()
        .expect("failed to check for sixel support");

    // create a purple rectangle
    let mut buffer = Vec::<u8>::with_capacity(H as usize * W as usize * 4);
    #[allow(unused_parens)]
    for _byte in (0..={ H * W }) {
        buffer.push(190);
        buffer.push(20);
        buffer.push(80);
        buffer.push(255);
    }

    let vframe1 = NcVisual::from_bgra(&buffer, H, W * 4, W).expect("couldn’t create visual");
    // let voptions = NcVisualOptions::fullsize_pixel_without_plane(0, 0, H, W);

    unsafe {
        let v = ffi::ncdirectf_render(nc, vframe1, NCBLIT_PIXEL, NCSCALE_NONE, 0, 0);
        if !v.is_null() {
            ncdirect_raster_frame(nc, v, NCALIGN_LEFT);
        }
    }

    vframe1.destroy();
    nc.stop().expect("failed to destroy ncdirect context");
}
