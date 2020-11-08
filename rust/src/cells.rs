// functions already exported by bindgen : 6
// -----------------------------------------
// cell_duplicate
// cell_extended_gcluster
// cell_load
// cell_release
// cells_double_box
// cells_rounded_box
//
// static inline functions total: 42
// ------------------------------------------ (implement / remaining)
// (X) wont:  2
// (+) done: 40 /  0
// (#) test:  0 / 40
// ------------------------------------------
//+ cell_bchannel
//+ cell_bg_alpha
//+ cell_bg_default_p
//+ cell_bg_palindex
//+ cell_bg_palindex_p
//+ cell_bg_rgb
//+ cell_bg_rgb8
//+ cellcmp
//+ cell_double_wide_p
//+ cell_extract
//+ cell_fchannel
//+ cell_fg_alpha
//+ cell_fg_default_p
//+ cell_fg_palindex
//+ cell_fg_palindex_p
//+ cell_fg_rgb
//+ cell_fg_rgb8
//+ cell_init
//+ cell_load_char
//+ cell_prime
//+ cell_set_bchannel
//+ cell_set_bg_alpha
//+ cell_set_bg_default
//+ cell_set_bg_palindex
//+ cell_set_bg_rgb
//+ cell_set_bg_rgb8
//X cell_set_bg_rgb8_clipped   // unneeded
//+ cell_set_fchannel
//+ cell_set_fg_alpha
//+ cell_set_fg_default
//+ cell_set_fg_palindex
//+ cell_set_fg_rgb
//+ cell_set_fg_rgb8
//X cell_set_fg_rgb8_clipped   // unneeded
//+ cells_load_box
//+ cell_strdup
//+ cell_styles
//+ cell_styles_off
//+ cell_styles_on
//+ cell_styles_set
//+ cell_wide_left_p
//+ cell_wide_right_p

use libc::strcmp;

use crate::{
    cell_extended_gcluster, cell_load, cell_release, channels_bchannel, channels_bg_alpha,
    channels_bg_default_p, channels_bg_palindex_p, channels_bg_rgb, channels_bg_rgb8,
    channels_fchannel, channels_fg_alpha, channels_fg_default_p, channels_fg_palindex_p,
    channels_fg_rgb, channels_fg_rgb8, channels_set_bchannel, channels_set_bg_alpha,
    channels_set_bg_default, channels_set_bg_rgb, channels_set_bg_rgb8, channels_set_fchannel,
    channels_set_fg_alpha, channels_set_fg_default, channels_set_fg_rgb, channels_set_fg_rgb8,
    types::{
        AlphaBits, Cell, Channel, Channels, Color, Egc, IntResult, NcPlane, PaletteIndex, StyleMask,
    },
    CELL_ALPHA_OPAQUE, CELL_BGDEFAULT_MASK, CELL_BG_PALETTE, CELL_FGDEFAULT_MASK, CELL_FG_PALETTE,
    CELL_NOBACKGROUND_MASK, CELL_WIDEASIAN_MASK, NCSTYLE_MASK,
};

/// cell_load(), plus blast the styling with 'style' and 'channels'.
///
/// - Breaks the UTF-8 string in 'gcluster' down, setting up the cell 'cell'.
/// - Returns the number of bytes copied out of 'gcluster', or -1 on failure.
/// - The styling of the cell is left untouched, but any resources are released.
/// - blast the styling with 'style' and 'channels'
///
/// # Safety
///
/// Until we can change gcluster to a safer type, this function will remain unsafe
///
#[allow(unused_unsafe)]
pub unsafe fn cell_prime(
    plane: &mut NcPlane,
    cell: &mut Cell,
    gcluster: Egc,
    style: StyleMask,
    channels: Channels,
) -> IntResult {
    cell.stylemask = style;
    cell.channels = channels;
    unsafe { cell_load(plane, cell, gcluster as u32 as *const i8) }
}

/// load up six cells with the [Egc](type.Egc.html)s necessary to draw a box.
///
/// returns 0 on success, -1 on error.
///
/// on error, any cells this function might have loaded before the error
/// are cell_release()d. There must be at least six `Egc`s in gcluster.
///
/// # Safety
///
/// Until we can change gcluster to a safer type, this function will remain unsafe
///
#[allow(unused_unsafe)]
pub unsafe fn cells_load_box(
    plane: &mut NcPlane,
    style: StyleMask,
    channels: Channels,
    ul: &mut Cell,
    ur: &mut Cell,
    ll: &mut Cell,
    lr: &mut Cell,
    hl: &mut Cell,
    vl: &mut Cell,
    gcluster: Egc,
) -> IntResult {
    // mutable copy for pointer arithmetics:
    let mut gclu = gcluster as u32 as *const i8;
    let mut ulen: IntResult;

    ulen = unsafe { cell_prime(plane, ul, gcluster, style, channels) };

    if ulen > 0 {
        gclu = unsafe { gclu.offset(ulen as isize) };
        ulen = unsafe { cell_prime(plane, ur, gcluster, style, channels) };

        if ulen > 0 {
            gclu = unsafe { gclu.offset(ulen as isize) };
            ulen = unsafe { cell_prime(plane, ll, gcluster, style, channels) };

            if ulen > 0 {
                gclu = unsafe { gclu.offset(ulen as isize) };
                ulen = unsafe { cell_prime(plane, lr, gcluster, style, channels) };

                if ulen > 0 {
                    gclu = unsafe { gclu.offset(ulen as isize) };
                    ulen = unsafe { cell_prime(plane, hl, gcluster, style, channels) };

                    #[allow(unused_assignments)] // gclu is not read
                    if ulen > 0 {
                        gclu = unsafe { gclu.offset(ulen as isize) };
                        ulen = unsafe { cell_prime(plane, vl, gcluster, style, channels) };

                        if ulen > 0 {
                            return 0;
                        }
                        unsafe {
                            cell_release(plane, hl);
                        }
                    }
                    unsafe {
                        cell_release(plane, lr);
                    }
                }
                unsafe {
                    cell_release(plane, ll);
                }
            }
            unsafe {
                cell_release(plane, ur);
            }
        }
        unsafe {
            cell_release(plane, ul);
        }
    }
    -1
}

///
#[inline]
pub fn cell_init(cell: &mut Cell) {
    *cell = unsafe { core::mem::zeroed() }
}

/// Set the specified style bits for the cell 'c', whether they're actively
/// supported or not. Only the lower 16 bits are meaningful.
/// static inline void
#[inline]
pub fn cell_styles_set(cell: &mut Cell, stylebits: StyleMask) {
    cell.stylemask = stylebits & NCSTYLE_MASK as u16;
}

/// Extract the style bits from the cell.
#[inline]
pub fn cell_styles(cell: &Cell) -> StyleMask {
    cell.stylemask
}

/// Add the specified styles (in the LSBs) to the cell's existing spec, whether
/// they're actively supported or not.
#[inline]
pub fn cell_styles_on(cell: &mut Cell, stylebits: StyleMask) {
    cell.stylemask |= stylebits & NCSTYLE_MASK as u16;
}

/// Remove the specified styles (in the LSBs) from the cell's existing spec.
#[inline]
pub fn cell_styles_off(cell: &mut Cell, stylebits: StyleMask) {
    cell.stylemask &= !(stylebits & NCSTYLE_MASK as u16);
}

/// Use the default color for the foreground.
#[inline]
pub fn cell_set_fg_default(cell: &mut Cell) {
    channels_set_fg_default(&mut cell.channels);
}

/// Use the default color for the background.
#[inline]
pub fn cell_set_bg_default(cell: &mut Cell) {
    channels_set_bg_default(&mut cell.channels);
}

/// Set the foreground alpha.
#[inline]
pub fn cell_set_fg_alpha(cell: &mut Cell, alpha: AlphaBits) {
    channels_set_fg_alpha(&mut cell.channels, alpha);
}

/// Set the background alpha.
#[inline]
pub fn cell_set_bg_alpha(cell: &mut Cell, alpha: AlphaBits) {
    channels_set_bg_alpha(&mut cell.channels, alpha);
}

/// Does the cell contain an East Asian Wide codepoint?
// NOTE: remove casting when fixed: https://github.com/rust-lang/rust-bindgen/issues/1875
#[inline]
pub fn cell_double_wide_p(cell: &Cell) -> bool {
    (cell.channels & CELL_WIDEASIAN_MASK as Channels) != 0
}

/// Is this the right half of a wide character?
#[inline]
pub fn cell_wide_right_p(cell: &Cell) -> bool {
    cell_double_wide_p(cell) && cell.gcluster == 0
}

/// Is this the left half of a wide character?
#[inline]
pub fn cell_wide_left_p(cell: &Cell) -> bool {
    cell_double_wide_p(cell) && cell.gcluster != 0
}

/// copy the UTF8-encoded Egc out of the cell, whether simple or complex. the
/// result is not tied to the ncplane, and persists across erases / destruction.
#[inline]
pub fn cell_strdup(plane: &NcPlane, cell: &Cell) -> Egc {
    core::char::from_u32(unsafe { libc::strdup(cell_extended_gcluster(plane, cell)) } as i32 as u32)
        .expect("wrong char")

    // unsafer option B (maybe faster, TODO: bench)
    // unsafe {
    //     core::char::from_u32_unchecked(libc::strdup(cell_extended_gcluster(plane, cell)) as i32 as u32)
    // }
}

/// Extract the three elements of a cell.
#[inline]
pub fn cell_extract(
    plane: &NcPlane,
    cell: &Cell,
    stylemask: &mut StyleMask,
    channels: &mut Channels,
) -> Egc {
    if *stylemask != 0 {
        *stylemask = cell.stylemask;
    }
    if *channels != 0 {
        *channels = cell.channels;
    }
    cell_strdup(plane, cell)
}

/// Returns true if the two cells are distinct `Egc`s, attributes, or channels.
/// The actual egcpool index needn't be the same--indeed, the planes needn't even
/// be the same. Only the expanded Egc must be equal. The Egc must be bit-equal;
/// it would probably be better to test whether they're Unicode-equal FIXME.
#[inline]
pub fn cellcmp(plane1: &NcPlane, cell1: &Cell, plane2: &NcPlane, cell2: &Cell) -> bool {
    if cell1.stylemask != cell2.stylemask {
        return true;
    }
    if cell1.channels != cell2.channels {
        return true;
    }
    unsafe {
        strcmp(
            cell_extended_gcluster(plane1, cell1),
            cell_extended_gcluster(plane2, cell2),
        ) != 0
    }
}

///
// NOTE: remove casting for CELL_WIDEASIAN_MASK when fixed: https://github.com/rust-lang/rust-bindgen/issues/1875
#[inline]
pub fn cell_load_char(plane: &mut NcPlane, cell: &mut Cell, ch: Egc) -> i32 {
    unsafe {
        cell_release(plane, cell);
    }
    cell.channels &= !(CELL_WIDEASIAN_MASK as Channels | CELL_NOBACKGROUND_MASK);
    cell.gcluster = ch as u32;
    1
}

/// Extract the 32-bit background channel from a cell.
#[inline]
pub fn cell_bchannel(cell: &Cell) -> Channel {
    channels_bchannel(cell.channels)
}

/// Extract the 32-bit foreground channel from a cell.
#[inline]
pub fn cell_fchannel(cell: &Cell) -> Channel {
    channels_fchannel(cell.channels)
}

/// Set the 32-bit background channel of a cell.
#[inline]
pub fn cell_set_bchannel(cell: &mut Cell, channel: Channel) -> Channels {
    channels_set_bchannel(&mut cell.channels, channel)
}

/// Set the 32-bit foreground channel of a cell.
#[inline]
pub fn cell_set_fchannel(cell: &mut Cell, channel: Channel) -> Channels {
    channels_set_fchannel(&mut cell.channels, channel)
}

/// Extract 24 bits of foreground RGB from 'cell', shifted to LSBs.
#[inline]
pub fn cell_fg_rgb(cell: &Cell) -> Channel {
    channels_fg_rgb(cell.channels)
}

/// Extract 24 bits of background RGB from 'cell', shifted to LSBs.
#[inline]
pub fn cell_bg_rgb(cell: &Cell) -> Channel {
    channels_bg_rgb(cell.channels)
}

/// Extract 2 bits of foreground alpha from 'cell', shifted to LSBs.
#[inline]
pub fn cell_fg_alpha(cell: &Cell) -> AlphaBits {
    channels_fg_alpha(cell.channels)
}

/// Extract 2 bits of background alpha from 'cell', shifted to LSBs.
#[inline]
pub fn cell_bg_alpha(cell: &Cell) -> AlphaBits {
    channels_bg_alpha(cell.channels)
}

/// Extract 24 bits of foreground RGB from 'cell', split into components.
#[inline]
pub fn cell_fg_rgb8(cell: &Cell, red: &mut Color, green: &mut Color, blue: &mut Color) -> Channel {
    channels_fg_rgb8(cell.channels, red, green, blue)
}

/// Extract 24 bits of background RGB from 'cell', split into components.
#[inline]
pub fn cell_bg_rgb8(cell: &Cell, red: &mut Color, green: &mut Color, blue: &mut Color) -> Channel {
    channels_bg_rgb8(cell.channels, red, green, blue)
}

/// Set the r, g, and b cell for the foreground component of this 64-bit
/// 'cell' variable, and mark it as not using the default color.
#[inline]
pub fn cell_set_fg_rgb8(cell: &mut Cell, red: Color, green: Color, blue: Color) {
    channels_set_fg_rgb8(&mut cell.channels, red, green, blue);
}

/// Same as `cell_set_fg_rgb8()` but with an assembled 24-bit RGB value.
#[inline]
pub fn cell_set_fg_rgb(cell: &mut Cell, channel: Channel) {
    channels_set_fg_rgb(&mut cell.channels, channel);
}

/// Set the cell's foreground palette index, set the foreground palette index
/// bit, set it foreground-opaque, and clear the foreground default color bit.
///
// NOTE: this function now can't fail
#[inline]
pub fn cell_set_fg_palindex(cell: &mut Cell, index: PaletteIndex) {
    cell.channels |= CELL_FGDEFAULT_MASK;
    cell.channels |= CELL_FG_PALETTE;
    cell_set_fg_alpha(cell, CELL_ALPHA_OPAQUE);
    cell.channels &= 0xff000000ffffffff as Channels;
    cell.channels |= (index as Channels) << 32;
}

///
#[inline]
pub fn cell_fg_palindex(cell: &Cell) -> PaletteIndex {
    ((cell.channels & 0xff00000000 as Channels) >> 32) as PaletteIndex
}

/// Set the r, g, and b cell for the background component of this 64-bit
/// 'cell' variable, and mark it as not using the default color.
///
#[inline]
pub fn cell_set_bg_rgb8(cell: &mut Cell, red: Color, green: Color, blue: Color) {
    channels_set_bg_rgb8(&mut cell.channels, red, green, blue);
}

/// Same as `cell_set_fg_rgb8()` but with an assembled 24-bit RGB value.
///
#[inline]
pub fn cell_set_bg_rgb(cell: &mut Cell, channel: Channel) {
    channels_set_bg_rgb(&mut cell.channels, channel);
}

/// Set the cell's background palette index, set the background palette index
/// bit, set it background-opaque, and clear the background default color bit.
///
// NOTE: this function now can't fail
#[inline]
pub fn cell_set_bg_palindex(cell: &mut Cell, index: PaletteIndex) {
    cell.channels |= CELL_BGDEFAULT_MASK as Channels;
    cell.channels |= CELL_BG_PALETTE as Channels;
    cell_set_bg_alpha(cell, CELL_ALPHA_OPAQUE);
    cell.channels &= 0xffffffffff000000;
    cell.channels |= index as Channels;
}

///
#[inline]
pub fn cell_bg_palindex(cell: &Cell) -> PaletteIndex {
    (cell.channels & 0xff) as PaletteIndex
}

/// Is the foreground using the "default foreground color"?
#[inline]
pub fn cell_fg_default_p(cell: &Cell) -> bool {
    channels_fg_default_p(cell.channels)
}

///
#[inline]
pub fn cell_fg_palindex_p(cell: &Cell) -> bool {
    channels_fg_palindex_p(cell.channels)
}

/// Is the background using the "default background color"? The "default
/// background color" must generally be used to take advantage of
/// terminal-effected transparency.
#[inline]
pub fn cell_bg_default_p(cell: &Cell) -> bool {
    channels_bg_default_p(cell.channels)
}

///
#[inline]
pub fn cell_bg_palindex_p(cell: &Cell) -> bool {
    channels_bg_palindex_p(cell.channels)
}

#[cfg(test)]
mod test {
    // use super::nc;
    // use serial_test::serial;
    /*
    #[test]
    #[serial]
    fn () {
    }
    */
}
