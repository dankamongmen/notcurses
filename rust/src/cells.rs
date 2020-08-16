// functions already exported by bindgen : 6
// -----------------------------------------
// - cell_duplicate
// - cell_extended_gcluster
// - cell_load
// - cell_release
// - cells_double_box
// - cells_rounded_box
//
// static inline functions to reimplement: 45
// ------------------------------------------ (done / (x) wont / remaining)
// (+) implement : 2 / 0 / 43
// (#) unit tests: 0 / 0 / 45
// ------------------------------------------
// cell_bchannel
// cell_bg
// cell_bg_alpha
// cell_bg_default_p
// cell_bg_palindex
// cell_bg_palindex_p
// cell_bg_rgb
// cell_blend_bchannel
// cell_blend_fchannel
// cellcmp
// cell_double_wide_p
// cell_extract
// cell_fchannel
// cell_fg
// cell_fg_alpha
// cell_fg_default_p
// cell_fg_palindex
// cell_fg_palindex_p
// cell_fg_rgb
// cell_init
// cell_load_simple
//+cell_prime
// cell_set_bchannel
// cell_set_bg
// cell_set_bg_alpha
// cell_set_bg_default
// cell_set_bg_palindex
// cell_set_bg_rgb
// cell_set_bg_rgb_clipped
// cell_set_fchannel
// cell_set_fg
// cell_set_fg_alpha
// cell_set_fg_default
// cell_set_fg_palindex
// cell_set_fg_rgb
// cell_set_fg_rgb_clipped
// cell_simple_p
//+cells_load_box            // FIXME
// cell_strdup
// cell_styles
// cell_styles_off
// cell_styles_on
// cell_styles_set
// cell_wide_left_p
// cell_wide_right_p

use cstr_core::CString;

use crate as ffi;
use crate::types::{ChannelPair, IntResult};

/// cell_load(), plus blast the styling with 'attr' and 'channels'.
// TODO: TEST
pub fn cell_prime(
    plane: *mut ffi::ncplane,
    cell: *mut ffi::cell,
    gcluster: &str,
    style: u16,
    channels: ChannelPair,
) -> IntResult {
    unsafe {
        (*cell).stylemask = style;
        (*cell).channels = channels;
        ffi::cell_load(plane, cell, CString::new(gcluster).unwrap().as_ptr())
    }
}
// static inline int
// cell_prime(struct ncplane* n, cell* c, const char* gcluster,
//            uint32_t attr, uint64_t channels){
//   c->stylemask = attr;
//   c->channels = channels;
//   int ret = cell_load(n, c, gcluster);
//   return ret;
// }

/// load up six cells with the EGCs necessary to draw a box. returns 0 on
/// success, -1 on error. on error, any cells this function might
/// have loaded before the error are cell_release()d. There must be at least
/// six EGCs in gcluster.
// TODO: TEST
// FIXME missing cell_prime()s
pub fn cells_load_box(
    plane: *mut ffi::ncplane,
    style: u16,
    channels: ChannelPair,
    _ul: *mut ffi::cell,
    _ur: *mut ffi::cell,
    _ll: *mut ffi::cell,
    _lr: *mut ffi::cell,
    _hl: *mut ffi::cell,
    _vl: *mut ffi::cell,
    gcluster: &str,
) -> IntResult {
    cell_prime(plane, _ul, gcluster, style, channels)
}
// static inline int
// cells_load_box(struct ncplane* n, uint32_t style, uint64_t channels,
//                cell* ul, cell* ur, cell* ll, cell* lr,
//                cell* hl, cell* vl, const char* gclusters){
//     int ulen;
//     if((ulen = cell_prime(n, ul, gclusters, style, channels)) > 0){
//         if((ulen = cell_prime(n, ur, gclusters += ulen, style, channels)) > 0){
//             if((ulen = cell_prime(n, ll, gclusters += ulen, style, channels)) > 0){
//                 if((ulen = cell_prime(n, lr, gclusters += ulen, style, channels)) > 0){
//                     if((ulen = cell_prime(n, hl, gclusters += ulen, style, channels)) > 0){
//                         if((ulen = cell_prime(n, vl, gclusters += ulen, style, channels)) > 0){
//                             return 0;
//                         }
//                         cell_release(n, hl);
//                     }
//                     cell_release(n, lr);
//                 }
//                 cell_release(n, ll);
//             }
//             cell_release(n, ur);
//         }
//         cell_release(n, ul);
//     }
//     return -1;
// }

// static inline void
// cell_init(cell* c){
//     memset(c, 0, sizeof(*c));
// }
//
// // Set the specified style bits for the cell 'c', whether they're actively
// // supported or not.
// static inline void
// cell_styles_set(cell* c, unsigned stylebits){
//     c->stylemask = (c->stylemask & ~NCSTYLE_MASK) | ((stylebits & NCSTYLE_MASK));
// }
//
// // Extract the style bits from the cell's stylemask.
// static inline unsigned
// cell_styles(const cell* c){
//     return c->stylemask & NCSTYLE_MASK;
// }
//
// // Add the specified styles (in the LSBs) to the cell's existing spec, whether
// // they're actively supported or not.
// static inline void
// cell_styles_on(cell* c, unsigned stylebits){
//     c->stylemask |= (stylebits & NCSTYLE_MASK);
// }
//
// // Remove the specified styles (in the LSBs) from the cell's existing spec.
// static inline void
// cell_styles_off(cell* c, unsigned stylebits){
//     c->stylemask &= ~(stylebits & NCSTYLE_MASK);
// }
//
// // Use the default color for the foreground.
// static inline void
// cell_set_fg_default(cell* c){
//     channels_set_fg_default(&c->channels);
// }
//
// // Use the default color for the background.
// static inline void
// cell_set_bg_default(cell* c){
//     channels_set_bg_default(&c->channels);
// }
//
// static inline int
// cell_set_fg_alpha(cell* c, int alpha){
//     return channels_set_fg_alpha(&c->channels, alpha);
// }
//
// static inline int
// cell_set_bg_alpha(cell* c, int alpha){
//     return channels_set_bg_alpha(&c->channels, alpha);
// }
//
// // Does the cell contain an East Asian Wide codepoint?
// static inline bool
// cell_double_wide_p(const cell* c){
//     return (c->channels & CELL_WIDEASIAN_MASK);
// }
//
// // Is this the right half of a wide character?
// static inline bool
// cell_wide_right_p(const cell* c){
//     return cell_double_wide_p(c) && c->gcluster == 0;
// }
//
// // Is this the left half of a wide character?
// static inline bool
// cell_wide_left_p(const cell* c){
//     return cell_double_wide_p(c) && c->gcluster;
// }
//
// // Is the cell simple (a lone ASCII character, encoded as such)?
// static inline bool
// cell_simple_p(const cell* c){
//     return c->gcluster < 0x80;
// }
//
// // copy the UTF8-encoded EGC out of the cell, whether simple or complex. the
// // result is not tied to the ncplane, and persists across erases / destruction.
// static inline char*
// cell_strdup(const struct ncplane* n, const cell* c){
//     char* ret;
//     if(cell_simple_p(c)){
//         if( (ret = (char*)malloc(2)) ){ // cast is here for C++ clients
//             ret[0] = c->gcluster;
//             ret[1] = '\0';
//         }
//     }else{
//         ret = strdup(cell_extended_gcluster(n, c));
//     }
//     return ret;
// }
//
// // Extract the three elements of a cell.
// static inline char*
// cell_extract(const struct ncplane* n, const cell* c,
//                            uint32_t* stylemask, uint64_t* channels){
//     if(stylemask){
//         *stylemask = c->stylemask;
//     }
//     if(channels){
//         *channels = c->channels;
//     }
//     return cell_strdup(n, c);
// }
//
// // Returns true if the two cells are distinct EGCs, attributes, or channels.
// // The actual egcpool index needn't be the same--indeed, the planes needn't even
// // be the same. Only the expanded EGC must be equal. The EGC must be bit-equal;
// // it would probably be better to test whether they're Unicode-equal FIXME.
// static inline bool
// cellcmp(const struct ncplane* n1, const cell* RESTRICT c1,
//                 const struct ncplane* n2, const cell* RESTRICT c2){
//     if(c1->stylemask != c2->stylemask){
//         return true;
//     }
//     if(c1->channels != c2->channels){
//         return true;
//     }
//     if(cell_simple_p(c1) && cell_simple_p(c2)){
//         return c1->gcluster != c2->gcluster;
//     }
//     if(cell_simple_p(c1) || cell_simple_p(c2)){
//         return true;
//     }
//     return strcmp(cell_extended_gcluster(n1, c1), cell_extended_gcluster(n2, c2));
// }
//
// static inline int
// cell_load_simple(struct ncplane* n, cell* c, char ch){
//     cell_release(n, c);
//     c->channels &= ~CELL_WIDEASIAN_MASK;
//     c->gcluster = ch;
//     if(cell_simple_p(c)){
//         return 1;
//     }
//     return -1;
// }
//
// // Extract the 32-bit background channel from a cell.
// static inline unsigned
// cell_bchannel(const cell* cl){
//     return channels_bchannel(cl->channels);
// }
//
// // Extract the 32-bit foreground channel from a cell.
// static inline unsigned
// cell_fchannel(const cell* cl){
//     return channels_fchannel(cl->channels);
// }
//
// // Set the 32-bit background channel of a cell.
// static inline uint64_t
// cell_set_bchannel(cell* cl, uint32_t channel){
//     return channels_set_bchannel(&cl->channels, channel);
// }
//
// // Set the 32-bit foreground channel of a cell.
// static inline uint64_t
// cell_set_fchannel(cell* cl, uint32_t channel){
//     return channels_set_fchannel(&cl->channels, channel);
// }
//
// // do not pass palette-indexed channels!
// static inline uint64_t
// cell_blend_fchannel(cell* cl, unsigned channel, unsigned* blends){
//     return cell_set_fchannel(cl, channels_blend(cell_fchannel(cl), channel, blends));
// }
//
// static inline uint64_t
// cell_blend_bchannel(cell* cl, unsigned channel, unsigned* blends){
//     return cell_set_bchannel(cl, channels_blend(cell_bchannel(cl), channel, blends));
// }
//
// // Extract 24 bits of foreground RGB from 'cell', shifted to LSBs.
// static inline unsigned
// cell_fg(const cell* cl){
//     return channels_fg(cl->channels);
// }
//
// // Extract 24 bits of background RGB from 'cell', shifted to LSBs.
// static inline unsigned
// cell_bg(const cell* cl){
//     return channels_bg(cl->channels);
// }
//
// // Extract 2 bits of foreground alpha from 'cell', shifted to LSBs.
// static inline unsigned
// cell_fg_alpha(const cell* cl){
//     return channels_fg_alpha(cl->channels);
// }
//
// // Extract 2 bits of background alpha from 'cell', shifted to LSBs.
// static inline unsigned
// cell_bg_alpha(const cell* cl){
//     return channels_bg_alpha(cl->channels);
// }
//
// // Extract 24 bits of foreground RGB from 'cell', split into components.
// static inline unsigned
// cell_fg_rgb(const cell* cl, unsigned* r, unsigned* g, unsigned* b){
//     return channels_fg_rgb(cl->channels, r, g, b);
// }
//
// // Extract 24 bits of background RGB from 'cell', split into components.
// static inline unsigned
// cell_bg_rgb(const cell* cl, unsigned* r, unsigned* g, unsigned* b){
//     return channels_bg_rgb(cl->channels, r, g, b);
// }
//
// // Set the r, g, and b cell for the foreground component of this 64-bit
// // 'cell' variable, and mark it as not using the default color.
// static inline int
// cell_set_fg_rgb(cell* cl, int r, int g, int b){
//     return channels_set_fg_rgb(&cl->channels, r, g, b);
// }
//
// // Same, but clipped to [0..255].
// static inline void
// cell_set_fg_rgb_clipped(cell* cl, int r, int g, int b){
//     channels_set_fg_rgb_clipped(&cl->channels, r, g, b);
// }
//
// // Same, but with an assembled 24-bit RGB value.
// static inline int
// cell_set_fg(cell* c, uint32_t channel){
//     return channels_set_fg(&c->channels, channel);
// }
//
// // Set the cell's foreground palette index, set the foreground palette index
// // bit, set it foreground-opaque, and clear the foreground default color bit.
// static inline int
// cell_set_fg_palindex(cell* cl, int idx){
//     if(idx < 0 || idx >= NCPALETTESIZE){
//         return -1;
//     }
//     cl->channels |= CELL_FGDEFAULT_MASK;
//     cl->channels |= CELL_FG_PALETTE;
//     cell_set_fg_alpha(cl, CELL_ALPHA_OPAQUE);
//     cl->stylemask &= 0xffff00ff;
//     cl->stylemask |= (idx << 8u);
//     return 0;
// }
//
// static inline unsigned
// cell_fg_palindex(const cell* cl){
//     return (cl->stylemask & 0x0000ff00) >> 8u;
// }
//
// // Set the r, g, and b cell for the background component of this 64-bit
// // 'cell' variable, and mark it as not using the default color.
// static inline int
// cell_set_bg_rgb(cell* cl, int r, int g, int b){
//     return channels_set_bg_rgb(&cl->channels, r, g, b);
// }
//
// // Same, but clipped to [0..255].
// static inline void
// cell_set_bg_rgb_clipped(cell* cl, int r, int g, int b){
//     channels_set_bg_rgb_clipped(&cl->channels, r, g, b);
// }
//
// // Same, but with an assembled 24-bit RGB value. A value over 0xffffff
// // will be rejected, with a non-zero return value.
// static inline int
// cell_set_bg(cell* c, uint32_t channel){
//     return channels_set_bg(&c->channels, channel);
// }
//
// // Set the cell's background palette index, set the background palette index
// // bit, set it background-opaque, and clear the background default color bit.
// static inline int
// cell_set_bg_palindex(cell* cl, int idx){
//     if(idx < 0 || idx >= NCPALETTESIZE){
//         return -1;
//     }
//     cl->channels |= CELL_BGDEFAULT_MASK;
//     cl->channels |= CELL_BG_PALETTE;
//     cell_set_bg_alpha(cl, CELL_ALPHA_OPAQUE);
//     cl->stylemask &= 0xffffff00;
//     cl->stylemask |= idx;
//     return 0;
// }
//
// static inline unsigned
// cell_bg_palindex(const cell* cl){
//     return cl->stylemask & 0x000000ff;
// }
//
// // Is the foreground using the "default foreground color"?
// static inline bool
// cell_fg_default_p(const cell* cl){
//     return channels_fg_default_p(cl->channels);
// }
//
// static inline bool
// cell_fg_palindex_p(const cell* cl){
//     return channels_fg_palindex_p(cl->channels);
// }
//
// // Is the background using the "default background color"? The "default
// // background color" must generally be used to take advantage of
// // terminal-effected transparency.
// static inline bool
// cell_bg_default_p(const cell* cl){
//     return channels_bg_default_p(cl->channels);
// }
//
// static inline bool
// cell_bg_palindex_p(const cell* cl){
//     return channels_bg_palindex_p(cl->channels);
// }

#[cfg(test)]
mod test {
    // use super::ffi;
    // use serial_test::serial;
    /*
    #[test]
    #[serial]
    fn () {
    }
    */
}
