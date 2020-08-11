// ---------------------------------------------------------------------------------------
// NOTE: These functions now can't fail and dont't have to return an error:
// - `palette256_set_rgb()`
// ---------------------------------------------------------------------------------------
//
// functions already exported by bindgen : 3
// -----------------------------------------
// palette256_free
// palette256_new
// palette256_use
//
// static inline functions to reimplement: 3
// -----------------------------------------
// - finished : 1
// - remaining: 2
// --------------- (+) implemented (#) + unit test (x) wont implement
//+palette256_get_rgb
// palette256_set
// palette256_set_rgb

use crate as ffi;
use crate::types::Color;

/// Manipulate entries in the palette store 'p'. These are *not* locked.
pub fn palette256_set_rgb(palette: &mut ffi::palette256, idx: usize, r: Color, g: Color, b: Color) {
    ffi::channel_set_rgb(&mut palette.chans[idx], r, g, b)
}

// XXX: what is the return type?
// static inline int
// palette256_set(palette256* p, int idx, unsigned rgb){
//   if(idx < 0 || (size_t)idx > sizeof(p->chans) / sizeof(*p->chans)){
//     return -1;
//   }
//   return channel_set(&p->chans[idx], rgb);
// }

// static inline int
// palette256_get_rgb(const palette256* p, int idx, unsigned* RESTRICT r, unsigned* RESTRICT g, unsigned* RESTRICT b){
//   if(idx < 0 || (size_t)idx > sizeof(p->chans) / sizeof(*p->chans)){
//     return -1;
//   }
//   return channel_rgb(p->chans[idx], r, g, b);
// }
