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
// (+) implement : 24 / 0 / 21
// (#) unit tests: 0 / 0 / 45
// ------------------------------------------
//+cell_bchannel
// cell_bg
// cell_bg_alpha
// cell_bg_default_p
// cell_bg_palindex
// cell_bg_palindex_p
//+cell_bg_rgb
// cell_blend_bchannel
// cell_blend_fchannel
//+cellcmp
//+cell_double_wide_p
//+cell_extract
//+cell_fchannel
// cell_fg
// cell_fg_alpha
// cell_fg_default_p
// cell_fg_palindex
// cell_fg_palindex_p
//+cell_fg_rgb
//+cell_init
//+cell_load_simple
//+cell_prime
//+cell_set_bchannel
// cell_set_bg
//+cell_set_bg_alpha
// cell_set_bg_default
// cell_set_bg_palindex
// cell_set_bg_rgb
// cell_set_bg_rgb_clipped
//+cell_set_fchannel
// cell_set_fg
//+cell_set_fg_alpha
//+cell_set_fg_default
//+cell_set_fg_palindex
// cell_set_fg_rgb
// cell_set_fg_rgb_clipped
// cell_simple_p
//+cells_load_box            // FIXME
//+cell_strdup
//+cell_styles
//+cell_styles_off
//+cell_styles_on
//+cell_styles_set
//+cell_wide_left_p
//+cell_wide_right_p

use cstr_core::CString;

use crate as ffi;
use ffi::types::{AlphaBits, Channel, ChannelPair, Color, GraphemeCluster, IntResult, StyleMask};
use ffi::{cell, ncplane};

/// cell_load(), plus blast the styling with 'attr' and 'channels'.
// TODO: TEST
pub fn cell_prime(
    plane: &mut ffi::ncplane,
    cell: &mut ffi::cell,
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
    plane: &mut ncplane,
    style: u16,
    channels: ChannelPair,
    _ul: &mut cell,
    _ur: &mut cell,
    _ll: &mut cell,
    _lr: &mut cell,
    _hl: &mut cell,
    _vl: &mut cell,
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


// TODO: TEST
#[inline]
pub fn cell_init(cell: &mut cell) {
    *cell = unsafe { core::mem::zeroed() }
}

/// Set the specified style bits for the cell 'c', whether they're actively
/// supported or not. Only the lower 16 bits are meaningful.
/// static inline void
// TODO: TEST
#[inline]
pub fn cell_styles_set(cell: &mut cell, stylebits: StyleMask) {
    cell.stylemask = stylebits & ffi::NCSTYLE_MASK as u16;
}

/// Extract the style bits from the cell.
// TODO: TEST
#[inline]
pub fn cell_styles(cell: &cell) -> StyleMask {
    cell.stylemask
}

/// Add the specified styles (in the LSBs) to the cell's existing spec, whether
/// they're actively supported or not.
// TODO: TEST
#[inline]
pub fn cell_styles_on(cell: &mut cell, stylebits: StyleMask) {
    cell.stylemask |= stylebits & ffi::NCSTYLE_MASK as u16;
}

/// Remove the specified styles (in the LSBs) from the cell's existing spec.
// TODO: TEST
#[inline]
pub fn cell_styles_off(cell: &mut cell, stylebits: StyleMask) {
    cell.stylemask &= !(stylebits & ffi::NCSTYLE_MASK as u16);
}

/// Use the default color for the foreground.
// TODO: TEST
#[inline]
pub fn cell_set_fg_default(cell: &mut cell) {
    ffi::channels_set_fg_default(&mut cell.channels);
}

/// Use the default color for the background.
// TODO: TEST
#[inline]
pub fn cell_set_bg_default(cell: &mut cell) {
    ffi::channels_set_bg_default(&mut cell.channels);
}

/// Set the foreground alpha.
// TODO: TEST
#[inline]
pub fn cell_set_fg_alpha(cell: &mut cell, alpha: AlphaBits) {
    ffi::channels_set_fg_alpha(&mut cell.channels, alpha);
}

/// Set the background alpha.
// TODO: TEST
#[inline]
pub fn cell_set_bg_alpha(cell: &mut cell, alpha: AlphaBits) {
    ffi::channels_set_bg_alpha(&mut cell.channels, alpha);
}

/// Does the cell contain an East Asian Wide codepoint?
// TODO: TEST
// NOTE: remove casting when fixed: https://github.com/rust-lang/rust-bindgen/issues/1875
#[inline]
pub fn cell_double_wide_p(cell: &cell) -> bool {
    (cell.channels & ffi::CELL_WIDEASIAN_MASK as u64) != 0
}

/// Is this the right half of a wide character?
// TODO: TEST
#[inline]
pub fn cell_wide_right_p(cell: &cell) -> bool {
    cell_double_wide_p(cell) && cell.gcluster == 0
}

/// Is this the left half of a wide character?
// TODO: TEST
#[inline]
pub fn cell_wide_left_p(cell: &cell) -> bool {
    cell_double_wide_p(cell) && cell.gcluster != 0
}

/// copy the UTF8-encoded EGC out of the cell, whether simple or complex. the
/// result is not tied to the ncplane, and persists across erases / destruction.
// TODO: TEST
#[inline]
pub fn cell_strdup(plane: &ncplane, cell: &cell) -> *mut i8 {
    unsafe{
        libc::strdup(ffi::cell_extended_gcluster(plane, cell))
    }
}

/// Extract the three elements of a cell.
// TODO: TEST
#[inline]
pub fn cell_extract(plane: &ncplane, cell: &cell, stylemask: &mut StyleMask, channels: &mut ChannelPair) -> *mut i8 {
    if *stylemask != 0 {
        *stylemask = cell.stylemask;
    }
    if *channels != 0 {
        *channels = cell.channels;
    }
    cell_strdup(plane, cell)
}

/// Returns true if the two cells are distinct EGCs, attributes, or channels.
/// The actual egcpool index needn't be the same--indeed, the planes needn't even
/// be the same. Only the expanded EGC must be equal. The EGC must be bit-equal;
/// it would probably be better to test whether they're Unicode-equal FIXME.
// TODO: TEST
#[inline]
pub fn cellcmp(plane1: &ncplane, cell1: &cell, plane2: &ncplane, cell2: &cell) -> bool {
    if cell1.stylemask != cell2.stylemask {
        return true;
    }
    if cell1.channels != cell2.channels {
        return true;
    }
    unsafe {
        libc::strcmp(ffi::cell_extended_gcluster(plane1, cell1), ffi::cell_extended_gcluster(plane2, cell2)) != 0
    }
}

// TODO: TEST
// NOTE: remove casting when fixed: https://github.com/rust-lang/rust-bindgen/issues/1875
#[inline]
pub fn cell_load_simple(plane: &mut ncplane, cell: &mut cell, ch: char) -> i32 {
    unsafe { ffi::cell_release(plane, cell); }
    cell.channels &= !(ffi::CELL_WIDEASIAN_MASK as u64 | ffi::CELL_NOBACKGROUND_MASK);
    cell.gcluster = ch as GraphemeCluster;
    1
}

/// Extract the 32-bit background channel from a cell.
// TODO: TEST
#[inline]
pub fn cell_bchannel(cell: &cell) -> Channel {
    ffi::channels_bchannel(cell.channels)
}

/// Extract the 32-bit foreground channel from a cell.
// TODO: TEST
#[inline]
pub fn cell_fchannel(cell: &cell) -> Channel {
    ffi::channels_fchannel(cell.channels)
}

/// Set the 32-bit background channel of a cell.
// TODO: TEST
#[inline]
pub fn cell_set_bchannel(cell: &mut cell, channel: Channel) -> ChannelPair {
    ffi::channels_set_bchannel(&mut cell.channels, channel)
}

/// Set the 32-bit foreground channel of a cell.
// TODO: TEST
#[inline]
pub fn cell_set_fchannel(cell: &mut cell, channel: Channel) -> ChannelPair {
    ffi::channels_set_fchannel(&mut cell.channels, channel)
}

// NOTE: do not pass palette-indexed channels!
// static inline uint64_t
// cell_blend_fchannel(cell* cl, unsigned channel, unsigned* blends){
//     return cell_set_fchannel(cl, channels_blend(cell_fchannel(cl), channel, blends));
// }

// NOTE: do not pass palette-indexed channels!
// static inline uint64_t
// cell_blend_bchannel(cell* cl, unsigned channel, unsigned* blends){
//     return cell_set_bchannel(cl, channels_blend(cell_bchannel(cl), channel, blends));
// }

/// Extract 24 bits of foreground RGB from 'cell', shifted to LSBs.
// TODO: TEST
#[inline]
pub fn cell_fg(cell: &cell) -> Channel {
    ffi::channels_fg(cell.channels)
}

/// Extract 24 bits of background RGB from 'cell', shifted to LSBs.
// TODO: TEST
#[inline]
pub fn cell_bg(cell: &cell) -> Channel {
    ffi::channels_bg(cell.channels)
}

/// Extract 2 bits of foreground alpha from 'cell', shifted to LSBs.
// TODO: TEST
#[inline]
pub fn cell_fg_alpha(cell: &cell) -> AlphaBits {
    ffi::channels_fg_alpha(cell.channels)
}

/// Extract 2 bits of background alpha from 'cell', shifted to LSBs.
// TODO: TEST
#[inline]
pub fn cell_bg_alpha(cell: &cell) -> AlphaBits {
    ffi::channels_bg_alpha(cell.channels)
}

/// Extract 24 bits of foreground RGB from 'cell', split into components.
// TODO: TEST
#[inline]
pub fn cell_fg_rgb(cell: &cell, red: &mut Color, green: &mut Color, blue: &mut Color) -> Channel {
    ffi::channels_fg_rgb(cell.channels, red, green, blue)
}

/// Extract 24 bits of background RGB from 'cell', split into components.
// TODO: TEST
#[inline]
pub fn cell_bg_rgb(cell: &cell, red: &mut Color, green: &mut Color, blue: &mut Color) -> Channel {
    ffi::channels_bg_rgb(cell.channels, red, green, blue)
}

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

// static inline int
// cell_set_fg_palindex(cell* cl, int idx){
//     if(idx < 0 || idx >= NCPALETTESIZE){
//         return -1;
//     }
//     cl->channels |= CELL_FGDEFAULT_MASK;
//     cl->channels |= CELL_FG_PALETTE;
//     cell_set_fg_alpha(cl, CELL_ALPHA_OPAQUE);
//     cl->channels &= 0xff000000ffffffffull;
//     cl->channels |= ((uint64_t)idx << 32u);
//     return 0;
// }
//
// static inline unsigned
// cell_fg_palindex(const cell* cl){
//     return (cl->channels & 0xff00000000ull) >> 32u;
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
//     cl->channels &= 0xffffffffff000000;
//     cl->channels |= idx;
//     return 0;
// }
//
// static inline unsigned
// cell_bg_palindex(const cell* cl){
//     return (cl->channels & 0xff);
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
