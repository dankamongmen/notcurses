//! `cell*_*` reimplemented functions.

use libc::strcmp;

use crate::{
    cell_release, cstring, NcAlphaBits, NcCell, NcChannel, NcChannelPair, NcColor, NcEgc,
    NcIntResult, NcPaletteIndex, NcPlane, NcRgb, NcStyleMask, NCCELL_ALPHA_OPAQUE,
    NCCELL_BGDEFAULT_MASK, NCCELL_BG_PALETTE, NCCELL_FGDEFAULT_MASK, NCCELL_FG_PALETTE,
    NCCELL_NOBACKGROUND_MASK, NCRESULT_ERR, NCRESULT_OK, NCSTYLE_MASK,
};

// Alpha -----------------------------------------------------------------------

/// Extracts the foreground [NcAlphaBits] from an [NcCell] (shifted to LSBs).
///
/// *Method: NcCell.[fg_alpha()][NcCell#method.fg_alpha].*
#[inline]
pub fn cell_fg_alpha(cell: &NcCell) -> NcAlphaBits {
    crate::channels_fg_alpha(cell.channels)
}

/// Extracts the background [NcAlphaBits] from an [NcCell] (shifted to LSBs).
///
/// *Method: NcCell.[bg_alpha()][NcCell#method.bg_alpha].*
#[inline]
pub fn cell_bg_alpha(cell: &NcCell) -> NcAlphaBits {
    crate::channels_bg_alpha(cell.channels)
}

/// Sets the foreground [NcAlphaBits] of an [NcCell].
///
/// *Method: NcCell.[set_fg_alpha()][NcCell#method.set_fg_alpha].*
#[inline]
pub fn cell_set_fg_alpha(cell: &mut NcCell, alpha: NcAlphaBits) {
    crate::channels_set_fg_alpha(&mut cell.channels, alpha);
}

/// Sets the background [NcAlphaBits] of an [NcCell].
///
/// *Method: NcCell.[set_bg_alpha()][NcCell#method.set_bg_alpha].*
#[inline]
pub fn cell_set_bg_alpha(cell: &mut NcCell, alpha: NcAlphaBits) {
    crate::channels_set_bg_alpha(&mut cell.channels, alpha);
}

// Channels --------------------------------------------------------------------

/// Gets the foreground [NcChannel] from an [NcCell].
///
/// *Method: NcCell.[fchannel()][NcCell#method.fchannel].*
#[inline]
pub fn cell_fchannel(cell: &NcCell) -> NcChannel {
    crate::channels_fchannel(cell.channels)
}

/// Gets the background [NcChannel] from an [NcCell].
///
/// *Method: NcCell.[bchannel()][NcCell#method.bchannel].*
#[inline]
pub fn cell_bchannel(cell: &NcCell) -> NcChannel {
    crate::channels_bchannel(cell.channels)
}

/// Sets the foreground [NcChannel] of an [NcCell] and returns the new
/// [NcChannelPair].
///
/// *Method: NcCell.[set_fchannel()][NcCell#method.set_fchannel].*
#[inline]
pub fn cell_set_fchannel(cell: &mut NcCell, channel: NcChannel) -> NcChannelPair {
    crate::channels_set_fchannel(&mut cell.channels, channel)
}

/// Sets the background [NcChannel] of an [NcCell] and returns the new
/// [NcChannelPair].
///
/// *Method: NcCell.[set_bchannel()][NcCell#method.set_bchannel].*
#[inline]
pub fn cell_set_bchannel(cell: &mut NcCell, channel: NcChannel) -> NcChannelPair {
    crate::channels_set_bchannel(&mut cell.channels, channel)
}

// NcColor ---------------------------------------------------------------------

/// Gets the foreground [NcColor] RGB components of an [NcCell],
/// and returns the [NcChannel] (which can have some extra bits set).
///
/// *Method: NcCell.[fg_rgb8()][NcCell#method.fg_rgb8].*
#[inline]
pub fn cell_fg_rgb8(
    cell: &NcCell,
    red: &mut NcColor,
    green: &mut NcColor,
    blue: &mut NcColor,
) -> NcChannel {
    crate::channels_fg_rgb8(cell.channels, red, green, blue)
}

/// Gets the background [NcColor] RGB components of an [NcCell],
/// and returns the [NcChannel] (which can have some extra bits set).
///
/// *Method: NcCell.[bg_rgb8()][NcCell#method.bg_rgb8].*
#[inline]
pub fn cell_bg_rgb8(
    cell: &NcCell,
    red: &mut NcColor,
    green: &mut NcColor,
    blue: &mut NcColor,
) -> NcChannel {
    crate::channels_bg_rgb8(cell.channels, red, green, blue)
}

/// Sets the foreground [NcColor] RGB components of the [NcCell],
/// and marks it as not using the "default color".
///
/// *Method: NcCell.[set_fg_rgb8()][NcCell#method.set_fg_rgb8].*
#[inline]
pub fn cell_set_fg_rgb8(cell: &mut NcCell, red: NcColor, green: NcColor, blue: NcColor) {
    crate::channels_set_fg_rgb8(&mut cell.channels, red, green, blue);
}

/// Sets the background [NcColor] RGB components of the [NcCell],
/// and marks it as not using the "default color".
///
/// *Method: NcCell.[set_bg_rgb8()][NcCell#method.set_bg_rgb8].*
#[inline]
pub fn cell_set_bg_rgb8(cell: &mut NcCell, red: NcColor, green: NcColor, blue: NcColor) {
    crate::channels_set_bg_rgb8(&mut cell.channels, red, green, blue);
}

// NcRgb -----------------------------------------------------------------------

/// Gets the foreground [NcRgb] from an [NcCell] (shifted to LSBs).
///
/// *Method: NcCell.[fg_rgb()][NcCell#method.fg_rgb].*
#[inline]
pub fn cell_fg_rgb(cell: &NcCell) -> NcRgb {
    crate::channels_fg_rgb(cell.channels)
}

/// Gets the background [NcRgb] from an [NcCell] (shifted to LSBs).
///
/// *Method: NcCell.[bg_rgb()][NcCell#method.bg_rgb].*
#[inline]
pub fn cell_bg_rgb(cell: &NcCell) -> NcRgb {
    crate::channels_bg_rgb(cell.channels)
}

/// Sets the foreground [NcRgb] of an [NcCell],
/// and marks it as not using the default color.
///
/// *Method: NcCell.[set_fg_rgb()][NcCell#method.set_fg_rgb].*
#[inline]
pub fn cell_set_fg_rgb(cell: &mut NcCell, rgb: NcRgb) {
    crate::channels_set_fg_rgb(&mut cell.channels, rgb);
}

/// Sets the background [NcRgb] of an [NcCell],
/// and marks it as not using the default color.
///
/// *Method: NcCell.[set_bg_rgb()][NcCell#method.set_bg_rgb].*
#[inline]
pub fn cell_set_bg_rgb(cell: &mut NcCell, rgb: NcRgb) {
    crate::channels_set_bg_rgb(&mut cell.channels, rgb);
}

// Default ---------------------------------------------------------------------

/// Indicates to use the "default color" for the foreground [NcChannel]
/// of an [NcCell].
///
/// *Method: NcCell.[set_fg_default()][NcCell#method.set_fg_default].*
#[inline]
pub fn cell_set_fg_default(cell: &mut NcCell) {
    crate::channels_set_fg_default(&mut cell.channels);
}

/// Indicates to use the "default color" for the background [NcChannel]
/// of an [NcCell].
///
/// *Method: NcCell.[set_bg_default()][NcCell#method.set_bg_default].*
#[inline]
pub fn cell_set_bg_default(cell: &mut NcCell) {
    crate::channels_set_bg_default(&mut cell.channels);
}

/// Is the foreground [NcChannel] of this [NcCell] using the
/// "default foreground color"?
///
/// *Method: NcCell.[fg_default_p()][NcCell#method.fg_default_p].*
#[inline]
pub fn cell_fg_default_p(cell: &NcCell) -> bool {
    crate::channels_fg_default_p(cell.channels)
}

/// Is the background [NcChannel] of this [NcCell] using the
/// "default background color"?
///
/// The "default background color" must generally be used to take advantage of
/// terminal-effected transparency.
///
/// *Method: NcCell.[bg_default_p()][NcCell#method.bg_default_p].*
#[inline]
pub fn cell_bg_default_p(cell: &NcCell) -> bool {
    crate::channels_bg_default_p(cell.channels)
}

// Palette ---------------------------------------------------------------------

/// Is the foreground [NcChannel] of this [NcCell] using an
/// [NcPaletteIndex] [indexed][NcPaletteIndex] [NcPalette][crate::NcPalette] color?
///
/// *Method: NcCell.[fg_palindex_p()][NcCell#method.fg_palindex_p].*
#[inline]
pub fn cell_fg_palindex_p(cell: &NcCell) -> bool {
    crate::channels_fg_palindex_p(cell.channels)
}

/// Is the background [NcChannel] of this [NcCell] using an
/// [NcPaletteIndex] [indexed][NcPaletteIndex] [NcPalette][crate::NcPalette] color?
///
/// *Method: NcCell.[bg_palindex_p()][NcCell#method.bg_palindex_p].*
#[inline]
pub fn cell_bg_palindex_p(cell: &NcCell) -> bool {
    crate::channels_bg_palindex_p(cell.channels)
}

/// Gets the [NcPaletteIndex] of the foreground [NcChannel] of the [NcCell].
///
/// *Method: NcCell.[fg_palindex()][NcCell#method.fg_palindex].*
#[inline]
pub const fn cell_fg_palindex(cell: &NcCell) -> NcPaletteIndex {
    ((cell.channels & 0xff00000000 as NcChannelPair) >> 32) as NcPaletteIndex
}

/// Gets the [NcPaletteIndex] of the background [NcChannel] of the [NcCell].
///
/// *Method: NcCell.[bg_palindex()][NcCell#method.bg_palindex].*
#[inline]
pub const fn cell_bg_palindex(cell: &NcCell) -> NcPaletteIndex {
    (cell.channels & 0xff) as NcPaletteIndex
}

/// Sets an [NcCell]'s foreground [NcPaletteIndex].
///
/// Also sets [NCCELL_FG_PALETTE] and [NCCELL_ALPHA_OPAQUE],
/// and clears out [NCCELL_FGDEFAULT_MASK].
///
/// *Method: NcCell.[set_fg_palindex()][NcCell#method.set_fg_palindex].*
//
// NOTE: unlike the original C function, this one can't fail
#[inline]
pub fn cell_set_fg_palindex(cell: &mut NcCell, index: NcPaletteIndex) {
    cell.channels |= NCCELL_FGDEFAULT_MASK;
    cell.channels |= NCCELL_FG_PALETTE;
    cell_set_fg_alpha(cell, NCCELL_ALPHA_OPAQUE);
    cell.channels &= 0xff000000ffffffff as NcChannelPair;
    cell.channels |= (index as NcChannelPair) << 32;
}

/// Sets an [NcCell]'s background [NcPaletteIndex].
///
/// Also sets [NCCELL_BG_PALETTE] and [NCCELL_ALPHA_OPAQUE],
/// and clears out [NCCELL_BGDEFAULT_MASK].
///
/// *Method: NcCell.[set_bg_palindex()][NcCell#method.set_bg_palindex].*
//
// NOTE: unlike the original C function, this one can't fail
#[inline]
pub fn cell_set_bg_palindex(cell: &mut NcCell, index: NcPaletteIndex) {
    cell.channels |= NCCELL_BGDEFAULT_MASK as NcChannelPair;
    cell.channels |= NCCELL_BG_PALETTE as NcChannelPair;
    cell_set_bg_alpha(cell, NCCELL_ALPHA_OPAQUE);
    cell.channels &= 0xffffffffff000000;
    cell.channels |= index as NcChannelPair;
}

// Styles ----------------------------------------------------------------------

/// Extracts the [NcStyleMask] bits from an [NcCell].
///
/// *Method: NcCell.[cell_styles()][NcCell#method.cell_styles].*
#[inline]
pub const fn cell_styles(cell: &NcCell) -> NcStyleMask {
    cell.stylemask
}

/// Adds the specified [NcStyleMask] bits to an [NcCell]'s existing spec.,
/// whether they're actively supported or not.
///
/// *Method: NcCell.[styles_on()][NcCell#method.styles_on].*
#[inline]
pub fn cell_on_styles(cell: &mut NcCell, stylebits: NcStyleMask) {
    cell.stylemask |= stylebits & NCSTYLE_MASK as u16;
}

/// Removes the specified [NcStyleMask] bits from an [NcCell]'s existing spec.
///
/// *Method: NcCell.[styles_off()][NcCell#method.styles_off].*
#[inline]
pub fn cell_off_styles(cell: &mut NcCell, stylebits: NcStyleMask) {
    cell.stylemask &= !(stylebits & NCSTYLE_MASK as u16);
}

/// Sets *just* the specified [NcStyleMask] bits for an [NcCell],
/// whether they're actively supported or not.
///
/// *Method: NcCell.[styles_set()][NcCell#method.styles_set].*
#[inline]
pub fn cell_set_styles(cell: &mut NcCell, stylebits: NcStyleMask) {
    cell.stylemask = stylebits & NCSTYLE_MASK as u16;
}

// Chars -----------------------------------------------------------------------

/// Does the [NcCell] contain an East Asian Wide codepoint?
///
/// *Method: NcCell.[double_wide_p()][NcCell#method.double_wide_p].*
//
// REMAINDER: remove casting when fixed:
// Waiting for: https://github.com/rust-lang/rust-bindgen/issues/1875
#[inline]
pub const fn cell_double_wide_p(cell: &NcCell) -> bool {
    cell.width > 0
}

/// Is this the right half of a wide character?
///
/// *Method: NcCell.[wide_right_p()][NcCell#method.wide_right_p].*
#[inline]
pub const fn cell_wide_right_p(cell: &NcCell) -> bool {
    cell_double_wide_p(cell) && cell.gcluster == 0
}

/// Is this the left half of a wide character?
///
/// *Method: NcCell.[wide_left_p()][NcCell#method.wide_left_p].*
#[inline]
pub const fn cell_wide_left_p(cell: &NcCell) -> bool {
    cell_double_wide_p(cell) && cell.gcluster != 0
}

/// Loads a 7-bit char into the [NcCell].
///
/// *Method: NcCell.[load_char()][NcCell#method.load_char].*
//
// NOTE: Unlike the original C function this doesn't return anything.
// REMINDER: remove casting for NCCELL_WIDEASIAN_MASK when fixed:
// Waiting for: https://github.com/rust-lang/rust-bindgen/issues/1875
#[inline]
pub fn cell_load_char(plane: &mut NcPlane, cell: &mut NcCell, ch: NcEgc) /* -> i32 */
{
    unsafe {
        crate::cell_release(plane, cell);
    }
    cell.channels &= !(NCCELL_WIDEASIAN_MASK as NcChannelPair | NCCELL_NOBACKGROUND_MASK);
    cell.gcluster = ch as u32;

    /* TODO new version:

    char gcluster[2];
    gcluster[0] = ch;
    gcluster[1] = '\0';
    return cell_load(n, c, gcluster);
    */
}

// TODO:
// // Load a UTF-8 encoded EGC of up to 4 bytes into the cell `c`.
// static inline int
// cell_load_egc32(struct ncplane* n, cell* c, uint32_t egc){
//     char gcluster[sizeof(egc) + 1];
//     egc = egc.to_le();
//     memcpy(gcluster, &egc, sizeof(egc));
//     gcluster[4] = '\0';
//     return cell_load(n, c, gcluster);
// }

/// Copies the UTF8-encoded [NcEgc] out of the [NcCell], whether simple or complex.
///
/// The result is not tied to the [NcPlane],
/// and persists across erases and destruction.
///
/// *Method: NcCell.[strdup()][NcCell#method.strdup].*
#[inline]
pub fn cell_strdup(plane: &NcPlane, cell: &NcCell) -> NcEgc {
    core::char::from_u32(
        unsafe { libc::strdup(crate::cell_extended_gcluster(plane, cell)) } as i32 as u32,
    )
    .expect("wrong char")

    // Unsafer option B (maybe faster, TODO: bench):
    // unsafe {
    //     core::char::from_u32_unchecked(libc::strdup(cell_extended_gcluster(plane, cell)) as i32 as u32)
    // }
}

// Misc. -----------------------------------------------------------------------

/// Saves the [NcStyleMask] and the [NcChannelPair],
/// and returns the [NcEgc], of an [NcCell].
///
/// *Method: NcCell.[extract()][NcCell#method.extract].*
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

/// Returns true if the two cells are distinct [NcEgc]s, attributes, or channels.
///
/// The actual egcpool index needn't be the same--indeed, the planes needn't even
/// be the same. Only the expanded NcEgc must be equal. The NcEgc must be bit-equal;
///
/// *Method: NcCell.[compare()][NcCell#method.compare].*
//
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
            crate::cell_extended_gcluster(plane1, cell1),
            crate::cell_extended_gcluster(plane2, cell2),
        ) != 0
    }
}

/// Initializes (zeroes out) an [NcCell].
///
/// *Method: NcCell.[init()][NcCell#method.init].*
#[inline]
pub fn cell_init(cell: &mut NcCell) {
    *cell = unsafe { core::mem::zeroed() }
}

/// Same as [cell_load][crate::cell_load], plus blasts the styling with
/// `style` and `channels`.
///
/// - Breaks the UTF-8 string in `gcluster` down, setting up the cell `cell`.
/// - Returns the number of bytes copied out of `gcluster`, or -1 on failure.
/// - The styling of the cell is left untouched, but any resources are released.
/// - Blasts the styling with `style` and `channels`.
///
/// *Method: NcCell.[prime()][NcCell#method.prime].*
pub fn cell_prime(
    plane: &mut NcPlane,
    cell: &mut NcCell,
    gcluster: &str,
    style: NcStyleMask,
    channels: NcChannelPair,
) -> NcIntResult {
    cell.stylemask = style;
    cell.channels = channels;
    unsafe { crate::cell_load(plane, cell, cstring![gcluster]) }
}

/// Loads up six cells with the [NcEgc]s necessary to draw a box.
///
/// Returns [NCRESULT_OK] on success or [NCRESULT_ERR] on error.
///
/// On error, any [NcCell]s this function might have loaded before the error
/// are [cell_release]d. There must be at least six [NcEgc]s in `gcluster`.
///
/// *Method: NcCell.[load_box()][NcCell#method.load_box].*
pub fn cells_load_box(
    plane: &mut NcPlane,
    style: NcStyleMask,
    channels: NcChannelPair,
    ul: &mut NcCell,
    ur: &mut NcCell,
    ll: &mut NcCell,
    lr: &mut NcCell,
    hl: &mut NcCell,
    vl: &mut NcCell,
    gcluster: &str,
) -> NcIntResult {
    // TODO: CHECK: mutable copy for pointer arithmetics:
    let mut gclu = cstring![gcluster];

    let mut ulen: NcIntResult;

    ulen = cell_prime(plane, ul, gcluster, style, channels);

    if ulen > 0 {
        gclu = unsafe { gclu.offset(ulen as isize) };
        ulen = cell_prime(plane, ur, gcluster, style, channels);

        if ulen > 0 {
            gclu = unsafe { gclu.offset(ulen as isize) };
            ulen = cell_prime(plane, ll, gcluster, style, channels);

            if ulen > 0 {
                gclu = unsafe { gclu.offset(ulen as isize) };
                ulen = cell_prime(plane, lr, gcluster, style, channels);

                if ulen > 0 {
                    gclu = unsafe { gclu.offset(ulen as isize) };
                    ulen = cell_prime(plane, hl, gcluster, style, channels);

                    if ulen > 0 {
                        let _gclu = unsafe { gclu.offset(ulen as isize) };
                        ulen = cell_prime(plane, vl, gcluster, style, channels);

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
