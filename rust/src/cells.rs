// functions already exported by bindgen : 6
// -----------------------------------------
// - cell_duplicate
// - cell_extended_gcluster
// - cell_load
// - cell_release
// - cells_double_box
// - cells_rounded_box
//
// static inline functions to reimplement: 42
// ------------------------------------------ (done / (x) wont / remaining)
// (+) implement : 38 / 2 /  2
// (#) unit tests:  0 / 2 / 40
// ------------------------------------------
//+cell_bchannel
//+cell_bg
//+cell_bg_alpha
//+cell_bg_default_p
//+cell_bg_palindex
//+cell_bg_palindex_p
//+cell_bg_rgb
//+cellcmp
//+cell_double_wide_p
//+cell_extract
//+cell_fchannel
//+cell_fg
//+cell_fg_alpha
//+cell_fg_default_p
//+cell_fg_palindex
//+cell_fg_palindex_p
//+cell_fg_rgb
//+cell_init
//+cell_load_simple
// cell_prime                // FIXME
//+cell_set_bchannel
//+cell_set_bg
//+cell_set_bg_alpha
//+cell_set_bg_default
//+cell_set_bg_palindex
//+cell_set_bg_rgb
//xcell_set_bg_rgb_clipped   // unneeded
//+cell_set_fchannel
//+cell_set_fg
//+cell_set_fg_alpha
//+cell_set_fg_default
//+cell_set_fg_palindex
//+cell_set_fg_rgb
//xcell_set_fg_rgb_clipped   // unneeded
// cells_load_box            // FIXME
//+cell_strdup
//+cell_styles
//+cell_styles_off
//+cell_styles_on
//+cell_styles_set
//+cell_wide_left_p
//+cell_wide_right_p

use cstr_core::CString;

use crate as ffi;
use ffi::types::{AlphaBits, Channel, ChannelPair, Color, GraphemeCluster, IntResult, PaletteIndex, StyleMask};
use ffi::{cell, ncplane};

/*
/// cell_load(), plus blast the styling with 'style' and 'channels'.
// TODO: TEST
pub fn cell_prime(
    plane: &mut ffi::ncplane,
    cell: &mut ffi::cell,
    gcluster: GraphemeCluster,
    style: u16,
    channels: ChannelPair,
) -> IntResult {
    unsafe {
        (*cell).stylemask = style;
        (*cell).channels = channels;
        ffi::cell_load(plane, cell, CString::new(gcluster).unwrap().as_ptr())
    }
}
*/
/*
/// load up six cells with the EGCs necessary to draw a box.
///
/// returns 0 on success, -1 on error.
///
/// on error, any cells this function might have loaded before the error
/// are cell_release()d. There must be at least six EGCs in gcluster.
///
// TODO: need clarification: https://github.com/dankamongmen/notcurses/issues/918
// TODO: TEST
pub fn cells_load_box(
    plane: &mut ncplane,
    style: StyleMask,
    channels: ChannelPair,
    ul: &mut cell,
    ur: &mut cell,
    ll: &mut cell,
    lr: &mut cell,
    hl: &mut cell,
    vl: &mut cell,
    gcluster: GraphemeCluster,
) -> IntResult {

    let mut ulen;

    ulen = cell_prime(plane, ul, gcluster, style, channels);
    if ulen > 0 {

        ulen = cell_prime(plane, ur, gcluster += ulen, style, channels);
        if ulen > 0 {
        }

    }

}
*/

/*
    int ulen;
    if((ulen = cell_prime(n, ul, gclusters, style, channels)) > 0){ // done
        if((ulen = cell_prime(n, ur, gclusters += ulen, style, channels)) > 0){

            if((ulen = cell_prime(n, ll, gclusters += ulen, style, channels)) > 0){
                if((ulen = cell_prime(n, lr, gclusters += ulen, style, channels)) > 0){
                    if((ulen = cell_prime(n, hl, gclusters += ulen, style, channels)) > 0){
                        if((ulen = cell_prime(n, vl, gclusters += ulen, style, channels)) > 0){
                            return 0;
                        }
                        cell_release(n, hl);
                    }
                    cell_release(n, lr);
                }
                cell_release(n, ll);
            }
            cell_release(n, ur);
        }
        cell_release(n, ul);
    }
    return -1;
*/


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

/// Set the r, g, and b cell for the foreground component of this 64-bit
/// 'cell' variable, and mark it as not using the default color.
// TODO: TEST
#[inline]
pub fn cell_set_fg_rgb(cell: &mut cell, red: Color, green: Color, blue: Color) {
    ffi::channels_set_fg_rgb(&mut cell.channels, red, green, blue);
}

/// Same as `cell_set_fg_rgb()` but with an assembled 24-bit RGB value.
// TODO: TEST
#[inline]
pub fn cell_set_fg(cell: &mut cell, channel: Channel) {
    ffi::channels_set_fg(&mut cell.channels, channel);
}

/// Set the cell's foreground palette index, set the foreground palette index
/// bit, set it foreground-opaque, and clear the foreground default color bit.
///
// TODO: TEST
// NOTE: this function now can't fail
#[inline]
pub fn cell_set_fg_palindex(cell: &mut cell, index: PaletteIndex) {
    cell.channels |= ffi::CELL_FGDEFAULT_MASK;
    cell.channels |= ffi::CELL_FG_PALETTE;
    cell_set_fg_alpha(cell, ffi::CELL_ALPHA_OPAQUE);
    cell.channels &= 0xff000000ffffffff_u64;
    cell.channels |= (index as u64) << 32;
}

// TODO: TEST
#[inline]
pub fn cell_fg_palindex(cell: &cell) -> PaletteIndex {
    ((cell.channels & 0xff00000000_u64) >> 32) as PaletteIndex
}

/// Set the r, g, and b cell for the background component of this 64-bit
/// 'cell' variable, and mark it as not using the default color.
///
// TODO: TEST
#[inline]
pub fn cell_set_bg_rgb(cell: &mut cell, red: Color, green: Color, blue: Color) {
    ffi::channels_set_bg_rgb(&mut cell.channels, red, green, blue);
}

/// Same as `cell_set_fg_rgb()` but with an assembled 24-bit RGB value.
///
// TODO: TEST
#[inline]
pub fn cell_set_bg(cell: &mut cell, channel: Channel) {
    ffi::channels_set_bg(&mut cell.channels, channel);
}

/// Set the cell's background palette index, set the background palette index
/// bit, set it background-opaque, and clear the background default color bit.
///
// TODO: TEST
// NOTE: this function now can't fail
#[inline]
pub fn cell_set_bg_palindex(cell: &mut cell, index: PaletteIndex) {
    cell.channels |= ffi::CELL_BGDEFAULT_MASK as u64 ;
    cell.channels |= ffi::CELL_BG_PALETTE as u64;
    cell_set_bg_alpha(cell, ffi::CELL_ALPHA_OPAQUE);
    cell.channels &= 0xffffffffff000000;
    cell.channels |= index as u64;
}

// TODO: TEST
#[inline]
pub fn cell_bg_palindex(cell: &cell) -> PaletteIndex {
    (cell.channels & 0xff) as PaletteIndex
}

/// Is the foreground using the "default foreground color"?
// TODO: TEST
#[inline]
pub fn cell_fg_default_p(cell: &cell) -> bool {
    ffi::channels_fg_default_p(cell.channels)
}
// TODO: TEST
#[inline]
pub fn cell_fg_palindex_p(cell: &cell) -> bool {
    ffi::channels_fg_palindex_p(cell.channels)
}

/// Is the background using the "default background color"? The "default
/// background color" must generally be used to take advantage of
/// terminal-effected transparency.
// TODO: TEST
#[inline]
pub fn cell_bg_default_p(cell: &cell) -> bool {
    ffi::channels_bg_default_p(cell.channels)
}

// TODO: TEST
#[inline]
pub fn cell_bg_palindex_p(cell: &cell) -> bool {
    ffi::channels_bg_palindex_p(cell.channels)
}

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
