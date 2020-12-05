//! `ncvisual`

// TODO: implement constructors & Drop

// functions already exported by bindgen : 17
// -----------------------------------------
// (#) test: 0 / 17
// -----------------------------------------
//  ncvisual_at_yx
//  ncvisual_decode
//  ncvisual_decode_loop
//  ncvisual_destroy
//  ncvisual_from_bgra
//  ncvisual_from_file
//  ncvisual_from_plane
//  ncvisual_from_rgba
//  ncvisual_geom
//  ncvisual_polyfill_yx
//  ncvisual_render
//  ncvisual_resize
//  ncvisual_rotate
//  ncvisual_set_yx
//  ncvisual_simple_streamer
//  ncvisual_stream
//  ncvisual_subtitle
//
// functions manually reimplemented: 1
// ------------------------------------------
// (+) done: 1 / 0
// (#) test: 0 / 1
// ------------------------------------------
// + ncvisual_default_blitter

use crate::{NCBLIT_1x1, NCBLIT_2x1, NCBLIT_2x2, NcBlitter, NcScale, NCSCALE_STRETCH};

/// Returns the best default blitter available
///
/// NCBLIT_3x2 is better image quality, especially for large images, but
/// it's not the general default because it doesn't preserve aspect ratio.
/// NCSCALE_STRETCH throws away aspect ratio, and can safely use NCBLIT_3x2.
pub fn ncvisual_default_blitter(utf8: bool, scale: NcScale) -> NcBlitter {
    if utf8 {
        if scale == NCSCALE_STRETCH {
            return NCBLIT_2x2;
        }
        return NCBLIT_2x1;
    }
    NCBLIT_1x1
}
