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

    let vframe1 = NcDirectF::from_bgra(&buffer, H, W * 4, W).expect("couldn’t create visual");
    let mut voptions1 = NcVisualOptions::without_plane(0, 0, 0, 0, H, W, NCBLIT_PIXEL, 0, 0);
    let v = vframe1.ncdirectf_render(nc, &mut voptions1)
        .expect("failed to render image to sixels");
    nc.raster_frame(v, NCALIGN_LEFT).expect("failed to print sixels");
    vframe1.ncdirectf_free();
    nc.stop().expect("failed to destroy ncdirect context");
    println!();
}
