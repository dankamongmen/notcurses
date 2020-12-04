//! `cell*_*` reimplemented functions.

use libc::strcmp;

use crate::{
    cell_extended_gcluster, cell_load, cell_release, channels_bchannel, channels_bg_alpha,
    channels_bg_default_p, channels_bg_palindex_p, channels_bg_rgb, channels_bg_rgb8,
    channels_fchannel, channels_fg_alpha, channels_fg_default_p, channels_fg_palindex_p,
    channels_fg_rgb, channels_fg_rgb8, channels_set_bchannel, channels_set_bg_alpha,
    channels_set_bg_default, channels_set_bg_rgb, channels_set_bg_rgb8, channels_set_fchannel,
    channels_set_fg_alpha, channels_set_fg_default, channels_set_fg_rgb, channels_set_fg_rgb8,
    NcAlphaBits, NcCell, NcChannel, NcChannelPair, NcColor, NcEgc, NcPaletteIndex, NcPlane,
    NcResult, NcRgb, NcStyleMask, NCCELL_ALPHA_OPAQUE, NCCELL_BGDEFAULT_MASK, NCCELL_BG_PALETTE,
    NCCELL_FGDEFAULT_MASK, NCCELL_FG_PALETTE, NCCELL_NOBACKGROUND_MASK, NCCELL_WIDEASIAN_MASK,
    NCRESULT_ERR, NCRESULT_OK, NCSTYLE_MASK,
};

/// Same as [cell_load], plus blasts the styling with 'style' and 'channels'.
///
/// - Breaks the UTF-8 string in 'gcluster' down, setting up the cell 'cell'.
/// - Returns the number of bytes copied out of 'gcluster', or -1 on failure.
/// - The styling of the cell is left untouched, but any resources are released.
/// - Blasts the styling with 'style' and 'channels'.
///
#[allow(unused_unsafe)]
pub unsafe fn cell_prime(
    plane: &mut NcPlane,
    cell: &mut NcCell,
    gcluster: NcEgc,
    style: NcStyleMask,
    channels: NcChannelPair,
) -> NcResult {
    cell.stylemask = style;
    cell.channels = channels;
    unsafe { cell_load(plane, cell, gcluster as u32 as *const i8) }
}

/// Loads up six cells with the [NcEgc]s necessary to draw a box.
///
/// Returns [NCRESULT_OK] on success, [NCRESULT_ERR] on error.
///
/// On error, any [NcCell]s this function might have loaded before the error
/// are [cell_release]d. There must be at least six [NcEgc]s in gcluster.
///
#[allow(unused_unsafe)]
pub unsafe fn cells_load_box(
    plane: &mut NcPlane,
    style: NcStyleMask,
    channels: NcChannelPair,
    ul: &mut NcCell,
    ur: &mut NcCell,
    ll: &mut NcCell,
    lr: &mut NcCell,
    hl: &mut NcCell,
    vl: &mut NcCell,
    gcluster: NcEgc,
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

/// Initializes (zeroes out) an [NcCell].
#[inline]
pub fn cell_init(cell: &mut NcCell) {
    *cell = unsafe { core::mem::zeroed() }
}

/// Sets *just* the specified [NcStyleMask] bits for an [NcCell],
/// whether they're actively supported or not.
#[inline]
pub fn cell_styles_set(cell: &mut NcCell, stylebits: NcStyleMask) {
    cell.stylemask = stylebits & NCSTYLE_MASK as u16;
}

/// Extracts the [NcStyleMask] bits from an [NcCell].
#[inline]
pub fn cell_styles(cell: &NcCell) -> NcStyleMask {
    cell.stylemask
}

/// Adds the specified [NcStyleMask] bits to an [NcCell]'s existing spec.,
/// whether they're actively supported or not.
#[inline]
pub fn cell_styles_on(cell: &mut NcCell, stylebits: NcStyleMask) {
    cell.stylemask |= stylebits & NCSTYLE_MASK as u16;
}

/// Removes the specified [NcStyleMask] bits from an [NcCell]'s existing spec.
#[inline]
pub fn cell_styles_off(cell: &mut NcCell, stylebits: NcStyleMask) {
    cell.stylemask &= !(stylebits & NCSTYLE_MASK as u16);
}

/// Indicates to use the "default color" for the **foreground** [NcChannel]
/// of an [NcCell].
#[inline]
pub fn cell_set_fg_default(cell: &mut NcCell) {
    channels_set_fg_default(&mut cell.channels);
}

/// Indicates to use the "default color" for the **background** [NcChannel]
/// of an [NcCell].
#[inline]
pub fn cell_set_bg_default(cell: &mut NcCell) {
    channels_set_bg_default(&mut cell.channels);
}

/// Sets the foreground [NcAlphaBits] of an [NcCell].
#[inline]
pub fn cell_set_fg_alpha(cell: &mut NcCell, alpha: NcAlphaBits) {
    channels_set_fg_alpha(&mut cell.channels, alpha);
}

/// Sets the background [NcAlphaBits] of an [NcCell].
#[inline]
pub fn cell_set_bg_alpha(cell: &mut NcCell, alpha: NcAlphaBits) {
    channels_set_bg_alpha(&mut cell.channels, alpha);
}

/// Does the [NcCell] contain an East Asian Wide codepoint?
// NOTE: remove casting when fixed:
// https://github.com/rust-lang/rust-bindgen/issues/1875
#[inline]
pub fn cell_double_wide_p(cell: &NcCell) -> bool {
    (cell.channels & NCCELL_WIDEASIAN_MASK as NcChannelPair) != 0
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

/// Copies the UTF8-encoded [NcEgc] out of the cell, whether simple
/// or complex.
///
/// The result is not tied to the [NcPlane], and persists
/// across erases and destruction.
#[inline]
pub fn cell_strdup(plane: &NcPlane, cell: &NcCell) -> NcEgc {
    core::char::from_u32(unsafe { libc::strdup(cell_extended_gcluster(plane, cell)) } as i32 as u32)
        .expect("wrong char")

    // unsafer option B (maybe faster, TODO: bench)
    // unsafe {
    //     core::char::from_u32_unchecked(libc::strdup(cell_extended_gcluster(plane, cell)) as i32 as u32)
    // }
}

/// Saves the [NcStyleMask] and the [NcChannelPair], and returns the [NcEgc]
/// (the three elements of an [NcCell]).
#[inline]
pub fn cell_extract(
    plane: &NcPlane,
    cell: &NcCell,
    stylemask: &mut NcStyleMask,
    channels: &mut NcChannelPair,
) -> NcEgc {
    if *stylemask != 0 {
        *stylemask = cell.stylemask;
    }
    if *channels != 0 {
        *channels = cell.channels;
    }
    cell_strdup(plane, cell)
}

/// Returns true if the two cells are distinct [NcEgc]s, attributes, or channels
///
/// The actual egcpool index needn't be the same--indeed, the planes needn't even
/// be the same. Only the expanded NcEgc must be equal. The NcEgc must be bit-equal;
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

/// Loads a 7-bit char into the [NcCell].
// NOTE: Unlike the original C function this doesn't return anything.
// REMINDER: remove casting for NCCELL_WIDEASIAN_MASK when fixed: https://github.com/rust-lang/rust-bindgen/issues/1875
#[inline]
pub fn cell_load_char(plane: &mut NcPlane, cell: &mut NcCell, ch: NcEgc) /* -> i32 */
{
    unsafe {
        cell_release(plane, cell);
    }
    cell.channels &= !(NCCELL_WIDEASIAN_MASK as NcChannelPair | NCCELL_NOBACKGROUND_MASK);
    cell.gcluster = ch as u32;
}

/// Extracts the 32-bit background [NcChannel] from an [NcCell].
#[inline]
pub fn cell_bchannel(cell: &NcCell) -> NcChannel {
    channels_bchannel(cell.channels)
}

/// Extracts the 32-bit foreground [NcChannel] from an [NcCell].
#[inline]
pub fn cell_fchannel(cell: &NcCell) -> NcChannel {
    channels_fchannel(cell.channels)
}

/// Sets the 32-bit background [NcChannel] of an [NcCell] and returns its new
/// [NcChannelPair].
#[inline]
pub fn cell_set_bchannel(cell: &mut NcCell, channel: NcChannel) -> NcChannelPair {
    channels_set_bchannel(&mut cell.channels, channel)
}

/// Sets the 32-bit foreground [NcChannel] of an [NcCell] and returns its new
/// [NcChannelPair].
#[inline]
pub fn cell_set_fchannel(cell: &mut NcCell, channel: NcChannel) -> NcChannelPair {
    channels_set_fchannel(&mut cell.channels, channel)
}

/// Extracts the foreground [NcRgb] 24-bit value from an [NcCell]
/// (shifted to LSBs).
#[inline]
pub fn cell_fg_rgb(cell: &NcCell) -> NcRgb {
    channels_fg_rgb(cell.channels)
}

/// Extracts the background [NcRgb] 24-bit value from an [NcCell]
/// (shifted to LSBs).
#[inline]
pub fn cell_bg_rgb(cell: &NcCell) -> NcRgb {
    channels_bg_rgb(cell.channels)
}

/// Extracts the foreground [NcAlphaBits] from an [NcCell] (shifted to LSBs).
#[inline]
pub fn cell_fg_alpha(cell: &NcCell) -> NcAlphaBits {
    channels_fg_alpha(cell.channels)
}

/// Extracts the background [NcAlphaBits] from an [NcCell] (shifted to LSBs).
#[inline]
pub fn cell_bg_alpha(cell: &NcCell) -> NcAlphaBits {
    channels_bg_alpha(cell.channels)
}

/// Extracts the foreground [NcRgb] 24-bit value from an [NcCell] and saves it
/// split into three [NcColor] 8-bit components. Also returns the corresponding
/// [NcChannel] (which can have some extra bits set).
#[inline]
pub fn cell_fg_rgb8(
    cell: &NcCell,
    red: &mut NcColor,
    green: &mut NcColor,
    blue: &mut NcColor,
) -> NcChannel {
    channels_fg_rgb8(cell.channels, red, green, blue)
}

/// Extracts the background [NcRgb] 24-bit value from an [NcCell] and saves it
/// split into three [NcColor] 8-bit components. Also returns the corresponding
/// [NcChannel] (which can have some extra bits set).
#[inline]
pub fn cell_bg_rgb8(
    cell: &NcCell,
    red: &mut NcColor,
    green: &mut NcColor,
    blue: &mut NcColor,
) -> NcChannel {
    channels_bg_rgb8(cell.channels, red, green, blue)
}

/// Sets the RGB [NcColor] components for the foreground [NcChannel] of an
/// [NcCell], and marks it as not using the default color.
#[inline]
pub fn cell_set_fg_rgb8(cell: &mut NcCell, red: NcColor, green: NcColor, blue: NcColor) {
    channels_set_fg_rgb8(&mut cell.channels, red, green, blue);
}

/// Sets the 24-bit [NcRgb] value for the foreground [NcChannel] of an
/// [NcCell], and marks it as not using the default color.
#[inline]
pub fn cell_set_fg_rgb(cell: &mut NcCell, rgb: NcRgb) {
    channels_set_fg_rgb(&mut cell.channels, rgb);
}

/// Sets an [NcCell]'s foreground [NcPaletteIndex].
///
/// Also sets [NCCELL_FG_PALETTE] and [NCCELL_ALPHA_OPAQUE],
/// and clears out [NCCELL_FGDEFAULT_MASK].
///
// NOTE: unlike the original C function, this one can't fail
#[inline]
pub fn cell_set_fg_palindex(cell: &mut NcCell, index: NcPaletteIndex) {
    cell.channels |= NCCELL_FGDEFAULT_MASK;
    cell.channels |= NCCELL_FG_PALETTE;
    cell_set_fg_alpha(cell, NCCELL_ALPHA_OPAQUE);
    cell.channels &= 0xff000000ffffffff as NcChannelPair;
    cell.channels |= (index as NcChannelPair) << 32;
}

/// Returns the [NcPaletteIndex] of the foreground [NcChannel] of the
/// [NcCell]
#[inline]
pub fn cell_fg_palindex(cell: &NcCell) -> NcPaletteIndex {
    ((cell.channels & 0xff00000000 as NcChannelPair) >> 32) as NcPaletteIndex
}

/// Sets the [NcColor] 8-bit RGB components of the background [NcChannel]
/// of the [NcCell], and marks it as not using the "default color".
#[inline]
pub fn cell_set_bg_rgb8(cell: &mut NcCell, red: NcColor, green: NcColor, blue: NcColor) {
    channels_set_bg_rgb8(&mut cell.channels, red, green, blue);
}

/// Sets the [NcRgb] 24-bit value for the background [NcChannel] of this
/// [NcCell], and marks it as not using the default color.
#[inline]
pub fn cell_set_bg_rgb(cell: &mut NcCell, rgb: NcRgb) {
    channels_set_bg_rgb(&mut cell.channels, rgb);
}

/// Sets an [NcCell]'s background [NcPaletteIndex].
///
/// Also sets [NCCELL_BG_PALETTE] and [NCCELL_ALPHA_OPAQUE],
/// and clears out [NCCELL_BGDEFAULT_MASK].
///
// NOTE: unlike the original C function, this one can't fail
#[inline]
pub fn cell_set_bg_palindex(cell: &mut NcCell, index: NcPaletteIndex) {
    cell.channels |= NCCELL_BGDEFAULT_MASK as NcChannelPair;
    cell.channels |= NCCELL_BG_PALETTE as NcChannelPair;
    cell_set_bg_alpha(cell, NCCELL_ALPHA_OPAQUE);
    cell.channels &= 0xffffffffff000000;
    cell.channels |= index as NcChannelPair;
}

/// Returns the [NcPaletteIndex] of the background [NcChannel] of the [NcCell]
#[inline]
pub fn cell_bg_palindex(cell: &NcCell) -> NcPaletteIndex {
    (cell.channels & 0xff) as NcPaletteIndex
}

/// Is the foreground [NcChannel] of this [NcCell] using the
/// "default foreground color"?
#[inline]
pub fn cell_fg_default_p(cell: &NcCell) -> bool {
    channels_fg_default_p(cell.channels)
}

/// Is the foreground [NcChannel] of this [NcCell] using an
/// [NcPaletteIndex] [indexed][NcPaletteIndex] [NcPalette][crate::NcPalette] color?
#[inline]
pub fn cell_fg_palindex_p(cell: &NcCell) -> bool {
    channels_fg_palindex_p(cell.channels)
}

/// Is the background [NcChannel] of this [NcCell] using the
/// "default background color"?
///
/// The "default background color" must generally be used to take advantage of
/// terminal-effected transparency.
#[inline]
pub fn cell_bg_default_p(cell: &NcCell) -> bool {
    channels_bg_default_p(cell.channels)
}

/// Is the background [NcChannel] of this [NcCell] using an
/// [NcPaletteIndex] [indexed][NcPaletteIndex] [NcPalette][crate::NcPalette] color?
#[inline]
pub fn cell_bg_palindex_p(cell: &NcCell) -> bool {
    channels_bg_palindex_p(cell.channels)
}
