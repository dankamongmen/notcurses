//! [`NcCell`] `cell*_*` static functions reimplementations

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
// (#) test:  26 / 14
// ------------------------------------------
//# cell_bchannel
//# cell_bg_alpha
//# cell_bg_default_p
//# cell_bg_palindex
//# cell_bg_palindex_p
//# cell_bg_rgb
//# cell_bg_rgb8
//+ cellcmp
//+ cell_double_wide_p
//+ cell_extract
//# cell_fchannel
//# cell_fg_alpha
//# cell_fg_default_p
//# cell_fg_palindex
//# cell_fg_palindex_p
//# cell_fg_rgb
//# cell_fg_rgb8
//+ cell_init
//+ cell_load_char
//+ cell_prime
//# cell_set_bchannel
//# cell_set_bg_alpha
//# cell_set_bg_default
//# cell_set_bg_palindex
//# cell_set_bg_rgb
//# cell_set_bg_rgb8
//X cell_set_bg_rgb8_clipped   // unneeded
//# cell_set_fchannel
//# cell_set_fg_alpha
//# cell_set_fg_default
//# cell_set_fg_palindex
//# cell_set_fg_rgb
//# cell_set_fg_rgb8
//X cell_set_fg_rgb8_clipped   // unneeded
//+ cells_load_box
//+ cell_strdup
//+ cell_styles
//+ cell_styles_off
//+ cell_styles_on
//+ cell_styles_set
//+ cell_wide_left_p
//+ cell_wide_right_p

#[cfg(test)]
mod tests;

mod constructors;
pub use constructors::*;

use libc::strcmp;

use crate::{
    cell_extended_gcluster, cell_load, cell_release, channels_bchannel, channels_bg_alpha,
    channels_bg_default_p, channels_bg_palindex_p, channels_bg_rgb, channels_bg_rgb8,
    channels_fchannel, channels_fg_alpha, channels_fg_default_p, channels_fg_palindex_p,
    channels_fg_rgb, channels_fg_rgb8, channels_set_bchannel, channels_set_bg_alpha,
    channels_set_bg_default, channels_set_bg_rgb, channels_set_bg_rgb8, channels_set_fchannel,
    channels_set_fg_alpha, channels_set_fg_default, channels_set_fg_rgb, channels_set_fg_rgb8,
    types::{
        NcAlphaBits, NcCell, NcChannel, NcChannels, NcChar, NcColor, NcPaletteIndex, NcPlane,
        NcResult, NcRgb, NcStyleMask, NCCELL_ALPHA_OPAQUE, NCCELL_BGDEFAULT_MASK,
        NCCELL_BG_PALETTE, NCCELL_FGDEFAULT_MASK, NCCELL_FG_PALETTE, NCCELL_NOBACKGROUND_MASK,
        NCCELL_WIDEASIAN_MASK, NCRESULT_ERR, NCRESULT_OK,
    },
    NCSTYLE_MASK,
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
    cell: &mut NcCell,
    gcluster: NcChar,
    style: NcStyleMask,
    channels: NcChannels,
) -> NcResult {
    cell.stylemask = style;
    cell.channels = channels;
    unsafe { cell_load(plane, cell, gcluster as u32 as *const i8) }
}

/// load up six cells with the [`NcChar`]s necessary to draw a box.
///
/// returns 0 on success, -1 on error.
///
/// on error, any cells this function might have loaded before the error
/// are cell_release()d. There must be at least six `NcChar`s in gcluster.
///
/// # Safety
///
/// Until we can change gcluster to a safer type, this function will remain unsafe
///
#[allow(unused_unsafe)]
pub unsafe fn cells_load_box(
    plane: &mut NcPlane,
    style: NcStyleMask,
    channels: NcChannels,
    ul: &mut NcCell,
    ur: &mut NcCell,
    ll: &mut NcCell,
    lr: &mut NcCell,
    hl: &mut NcCell,
    vl: &mut NcCell,
    gcluster: NcChar,
) -> NcResult {
    // mutable copy for pointer arithmetics:
    let mut gclu = gcluster as u32 as *const i8;
    let mut ulen: NcResult;

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

                    if ulen > 0 {
                        let _gclu = unsafe { gclu.offset(ulen as isize) };
                        ulen = unsafe { cell_prime(plane, vl, gcluster, style, channels) };

                        if ulen > 0 {
                            return NCRESULT_OK;
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
    NCRESULT_ERR
}

/// Initialize (zero out) the [`NcCell`].
#[inline]
pub fn cell_init(cell: &mut NcCell) {
    *cell = unsafe { core::mem::zeroed() }
}

/// Set *only* the specified [`NcStyleMask`] bits for the [`NcCell`],
/// whether they're actively supported or not.
#[inline]
pub fn cell_styles_set(cell: &mut NcCell, stylebits: NcStyleMask) {
    cell.stylemask = stylebits & NCSTYLE_MASK as u16;
}

/// Extract the [`NcStyleMask`] bits from the [`NcCell`].
#[inline]
pub fn cell_styles(cell: &NcCell) -> NcStyleMask {
    cell.stylemask
}

/// Add the specified [`NcStyleMask`] bits to the [`NcCell`]'s existing spec,
/// whether they're actively supported or not.
#[inline]
pub fn cell_styles_on(cell: &mut NcCell, stylebits: NcStyleMask) {
    cell.stylemask |= stylebits & NCSTYLE_MASK as u16;
}

/// Remove the specified [`NcStyleMask`] bits from the cell's existing spec.
#[inline]
pub fn cell_styles_off(cell: &mut NcCell, stylebits: NcStyleMask) {
    cell.stylemask &= !(stylebits & NCSTYLE_MASK as u16);
}

/// Use the default color for the foreground [`NcChannel`] of this [`NcCell`].
#[inline]
pub fn cell_set_fg_default(cell: &mut NcCell) {
    channels_set_fg_default(&mut cell.channels);
}

/// Use the default color for the background [`NcChannel`] of this [`NcCell`].
#[inline]
pub fn cell_set_bg_default(cell: &mut NcCell) {
    channels_set_bg_default(&mut cell.channels);
}

/// Set the foreground [`NcAlphaBits`].
#[inline]
pub fn cell_set_fg_alpha(cell: &mut NcCell, alpha: NcAlphaBits) {
    channels_set_fg_alpha(&mut cell.channels, alpha);
}

/// Set the background [`NcAlphaBits`].
#[inline]
pub fn cell_set_bg_alpha(cell: &mut NcCell, alpha: NcAlphaBits) {
    channels_set_bg_alpha(&mut cell.channels, alpha);
}

/// Does the [`NcCell`] contain an East Asian Wide codepoint?
// NOTE: remove casting when fixed: https://github.com/rust-lang/rust-bindgen/issues/1875
#[inline]
pub fn cell_double_wide_p(cell: &NcCell) -> bool {
    (cell.channels & NCCELL_WIDEASIAN_MASK as NcChannels) != 0
}

/// Is this the right half of a wide character?
#[inline]
pub fn cell_wide_right_p(cell: &NcCell) -> bool {
    cell_double_wide_p(cell) && cell.gcluster == 0
}

/// Is this the left half of a wide character?
#[inline]
pub fn cell_wide_left_p(cell: &NcCell) -> bool {
    cell_double_wide_p(cell) && cell.gcluster != 0
}

/// copy the UTF8-encoded NcChar out of the cell, whether simple or complex. the
/// result is not tied to the ncplane, and persists across erases / destruction.
#[inline]
pub fn cell_strdup(plane: &NcPlane, cell: &NcCell) -> NcChar {
    core::char::from_u32(unsafe { libc::strdup(cell_extended_gcluster(plane, cell)) } as i32 as u32)
        .expect("wrong char")

    // unsafer option B (maybe faster, TODO: bench)
    // unsafe {
    //     core::char::from_u32_unchecked(libc::strdup(cell_extended_gcluster(plane, cell)) as i32 as u32)
    // }
}

/// Extract the three elements of a cell: save the [`NcStyleMask`] and
/// [`NcChannels`], and return the [`NcChar`].
#[inline]
pub fn cell_extract(
    plane: &NcPlane,
    cell: &NcCell,
    stylemask: &mut NcStyleMask,
    channels: &mut NcChannels,
) -> NcChar {
    if *stylemask != 0 {
        *stylemask = cell.stylemask;
    }
    if *channels != 0 {
        *channels = cell.channels;
    }
    cell_strdup(plane, cell)
}

/// Returns true if the two cells are distinct [`NcChar`]s, attributes, or channels
///
/// The actual egcpool index needn't be the same--indeed, the planes needn't even
/// be the same. Only the expanded NcChar must be equal. The NcChar must be bit-equal;
// NOTE: FIXME: it would probably be better to test whether they're Unicode-equal
#[inline]
pub fn cellcmp(plane1: &NcPlane, cell1: &NcCell, plane2: &NcPlane, cell2: &NcCell) -> bool {
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
// NOTE: remove casting for NCCELL_WIDEASIAN_MASK when fixed: https://github.com/rust-lang/rust-bindgen/issues/1875
#[inline]
pub fn cell_load_char(plane: &mut NcPlane, cell: &mut NcCell, ch: NcChar) -> i32 {
    unsafe {
        cell_release(plane, cell);
    }
    cell.channels &= !(NCCELL_WIDEASIAN_MASK as NcChannels | NCCELL_NOBACKGROUND_MASK);
    cell.gcluster = ch as u32;
    1
}

/// Extract the 32-bit background channel from an [`NcCell`].
#[inline]
pub fn cell_bchannel(cell: &NcCell) -> NcChannel {
    channels_bchannel(cell.channels)
}

/// Extract the 32-bit foreground channel from an [`NcCell`].
#[inline]
pub fn cell_fchannel(cell: &NcCell) -> NcChannel {
    channels_fchannel(cell.channels)
}

/// Set the 32-bit background [`NcChannel`] of an [`NcCell`] and return the
/// [`NcChannels`].
#[inline]
pub fn cell_set_bchannel(cell: &mut NcCell, channel: NcChannel) -> NcChannels {
    channels_set_bchannel(&mut cell.channels, channel)
}

/// Set the 32-bit foreground [`NcChannel`] of an [`NcCell`] and return the
/// [`NcChannels`].
#[inline]
pub fn cell_set_fchannel(cell: &mut NcCell, channel: NcChannel) -> NcChannels {
    channels_set_fchannel(&mut cell.channels, channel)
}

/// Extract 24 bits of foreground [`NcRgb`] from the [`NcCell`] (shifted to LSBs).
#[inline]
pub fn cell_fg_rgb(cell: &NcCell) -> NcRgb {
    channels_fg_rgb(cell.channels)
}

/// Extract 24 bits of background RGB from the [`NcCell`] (shifted to LSBs).
#[inline]
pub fn cell_bg_rgb(cell: &NcCell) -> NcRgb {
    channels_bg_rgb(cell.channels)
}

/// Extract 2 bits of foreground alpha from the [`NcCell`] (shifted to LSBs).
#[inline]
pub fn cell_fg_alpha(cell: &NcCell) -> NcAlphaBits {
    channels_fg_alpha(cell.channels)
}

/// Extract 2 bits of background alpha from the [`NcCell`] (shifted to LSBs).
#[inline]
pub fn cell_bg_alpha(cell: &NcCell) -> NcAlphaBits {
    channels_bg_alpha(cell.channels)
}

/// Extract 24 bits of foreground [`NcRgb`] from the [`NcCell`] and save split
/// into [`NcColor`] components. Also return the corresponding [`NcChannel`]
/// (which can have some extra bits set).
#[inline]
pub fn cell_fg_rgb8(
    cell: &NcCell,
    red: &mut NcColor,
    green: &mut NcColor,
    blue: &mut NcColor,
) -> NcChannel {
    channels_fg_rgb8(cell.channels, red, green, blue)
}

/// Extract 24 bits of background [`NcRgb`] from the [`NcCell`] and save split
/// into [`NcColor`] components. Also return the corresponding [`NcChannel`]
/// (which can have some extra bits set).
#[inline]
pub fn cell_bg_rgb8(
    cell: &NcCell,
    red: &mut NcColor,
    green: &mut NcColor,
    blue: &mut NcColor,
) -> NcChannel {
    channels_bg_rgb8(cell.channels, red, green, blue)
}

/// Set the RGB [`NcColor`] components for the foreground [`NcChannel`] of this
/// [`NcCell`], and mark it as not using the default color.
#[inline]
pub fn cell_set_fg_rgb8(cell: &mut NcCell, red: NcColor, green: NcColor, blue: NcColor) {
    channels_set_fg_rgb8(&mut cell.channels, red, green, blue);
}

/// Set the [`NcRgb`] 24-bit value for the foreground [`NcChannel`] of this
/// [`NcCell`], and mark it as not using the default color.
#[inline]
pub fn cell_set_fg_rgb(cell: &mut NcCell, rgb: NcRgb) {
    channels_set_fg_rgb(&mut cell.channels, rgb);
}

/// Set the cell's foreground [`NcPaletteIndex`], set the foreground palette index
/// bit ([`NCCELL_FG_PALETTE`]), set it foreground-opaque ([`NCCELL_ALPHA_OPAQUE`]),
/// and clear the foreground default color bit ([`NCCELL_FGDEFAULT_MASK`]).
///
// NOTE: unlike the original C function, this one can't fail
#[inline]
pub fn cell_set_fg_palindex(cell: &mut NcCell, index: NcPaletteIndex) {
    cell.channels |= NCCELL_FGDEFAULT_MASK;
    cell.channels |= NCCELL_FG_PALETTE;
    cell_set_fg_alpha(cell, NCCELL_ALPHA_OPAQUE);
    cell.channels &= 0xff000000ffffffff as NcChannels;
    cell.channels |= (index as NcChannels) << 32;
}

/// Return the [`NcPaletteIndex`] of the foreground [`NcChannel`] of the
/// [`NcCell`]
#[inline]
pub fn cell_fg_palindex(cell: &NcCell) -> NcPaletteIndex {
    ((cell.channels & 0xff00000000 as NcChannels) >> 32) as NcPaletteIndex
}

/// Set the RGB [`NcColor`] components for the background [`NcChannel`] of this
/// [`NcCell`], and mark it as not using the default color.
#[inline]
pub fn cell_set_bg_rgb8(cell: &mut NcCell, red: NcColor, green: NcColor, blue: NcColor) {
    channels_set_bg_rgb8(&mut cell.channels, red, green, blue);
}

/// Set the [`NcRgb`] 24-bit value for the background [`NcChannel`] of this
/// [`NcCell`], and mark it as not using the default color.
#[inline]
pub fn cell_set_bg_rgb(cell: &mut NcCell, rgb: NcRgb) {
    channels_set_bg_rgb(&mut cell.channels, rgb);
}

/// Set the cell's background [`NcPaletteIndex`], set the background palette index
/// bit ([`NCCELL_BG_PALETTE`]), set it background-opaque ([`NCCELL_ALPHA_OPAQUE`]),
/// and clear the background default color bit ([`NCCELL_BGDEFAULT_MASK`]).
///
// NOTE: unlike the original C function, this one can't fail
#[inline]
pub fn cell_set_bg_palindex(cell: &mut NcCell, index: NcPaletteIndex) {
    cell.channels |= NCCELL_BGDEFAULT_MASK as NcChannels;
    cell.channels |= NCCELL_BG_PALETTE as NcChannels;
    cell_set_bg_alpha(cell, NCCELL_ALPHA_OPAQUE);
    cell.channels &= 0xffffffffff000000;
    cell.channels |= index as NcChannels;
}

/// Return the [`NcPaletteIndex`] of the background [`NcChannel`] of the
/// [`NcCell`]
#[inline]
pub fn cell_bg_palindex(cell: &NcCell) -> NcPaletteIndex {
    (cell.channels & 0xff) as NcPaletteIndex
}

/// Is the foreground [`NcChannel`] of this [`NcCell`] using the
/// "default foreground color"?
#[inline]
pub fn cell_fg_default_p(cell: &NcCell) -> bool {
    channels_fg_default_p(cell.channels)
}

/// Is the foreground [`NcChannel`] of this [`NcCell`] using an
/// [`NcPaletteIndex`] indexed [`NcPalette`][crate::NcPalette] color?
#[inline]
pub fn cell_fg_palindex_p(cell: &NcCell) -> bool {
    channels_fg_palindex_p(cell.channels)
}

/// Is the background [`NcChannel`] of this [`NcCell`] using the
/// "default background color"?
///
/// The "default background color" must generally be used to take advantage of
/// terminal-effected transparency.
#[inline]
pub fn cell_bg_default_p(cell: &NcCell) -> bool {
    channels_bg_default_p(cell.channels)
}

/// Is the background [`NcChannel`] of this [`NcCell`] using an
/// [`NcPaletteIndex`] indexed [`NcPalette`][`crate::NcPalette] color?
#[inline]
pub fn cell_bg_palindex_p(cell: &NcCell) -> bool {
    channels_bg_palindex_p(cell.channels)
}
