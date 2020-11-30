//! NcCell constructors and cell_* static functions reimplementations

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
        NcAlphaBits, NcCell, NcChannel, NcChannels, NcChar, NcCharBackstop, NcColor,
        NcPaletteIndex, NcPlane, NcResult, NcStyleMask, NCCELL_ALPHA_OPAQUE, NCCELL_BGDEFAULT_MASK,
        NCCELL_BG_PALETTE, NCCELL_FGDEFAULT_MASK, NCCELL_FG_PALETTE, NCCELL_NOBACKGROUND_MASK,
        NCCELL_WIDEASIAN_MASK, NCRESULT_ERR, NCRESULT_OK,
    },
    NCSTYLE_MASK,
};

// Constructors ----------------------------------------------------------------

impl NcCell {
    /// [`NcCell`] constructor expecting [`char`], [`NcStyleMask`] and [`NcChannels`]
    ///
    /// This is analogous to the [`cell_initializer`] macro
    #[inline]
    pub const fn new(ch: char, stylemask: NcStyleMask, channels: NcChannels) -> Self {
        NcCell {
            gcluster: ch as u32,
            gcluster_backstop: 0 as NcCharBackstop,
            reserved: 0,
            stylemask,
            channels,
        }
    }

    /// [`NcCell`] simple constructor just expecting a [`char`]
    ///
    /// This is analogous to the [`cell_char_initializer`] macro
    #[inline]
    pub const fn with_char(ch: char) -> Self {
        Self::new(ch, 0 as NcStyleMask, 0 as NcChannels)
    }

    /// [`NcCell`] simple constructor for an empty cell
    ///
    /// This is analogous to the [`cell_trivial_initializer`] macro
    #[inline]
    pub const fn new_blank() -> Self {
        Self::with_char(0 as char)
    }
}

// Static Functions ------------------------------------------------------------

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

/// load up six cells with the [NcChar]s necessary to draw a box.
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

///
#[inline]
pub fn cell_init(cell: &mut NcCell) {
    *cell = unsafe { core::mem::zeroed() }
}

/// Set the specified style bits for the cell 'c', whether they're actively
/// supported or not. Only the lower 16 bits are meaningful.
/// static inline void
#[inline]
pub fn cell_styles_set(cell: &mut NcCell, stylebits: NcStyleMask) {
    cell.stylemask = stylebits & NCSTYLE_MASK as u16;
}

/// Extract the style bits from the cell.
#[inline]
pub fn cell_styles(cell: &NcCell) -> NcStyleMask {
    cell.stylemask
}

/// Add the specified styles (in the LSBs) to the cell's existing spec, whether
/// they're actively supported or not.
#[inline]
pub fn cell_styles_on(cell: &mut NcCell, stylebits: NcStyleMask) {
    cell.stylemask |= stylebits & NCSTYLE_MASK as u16;
}

/// Remove the specified styles (in the LSBs) from the cell's existing spec.
#[inline]
pub fn cell_styles_off(cell: &mut NcCell, stylebits: NcStyleMask) {
    cell.stylemask &= !(stylebits & NCSTYLE_MASK as u16);
}

/// Use the default color for the foreground.
#[inline]
pub fn cell_set_fg_default(cell: &mut NcCell) {
    channels_set_fg_default(&mut cell.channels);
}

/// Use the default color for the background.
#[inline]
pub fn cell_set_bg_default(cell: &mut NcCell) {
    channels_set_bg_default(&mut cell.channels);
}

/// Set the foreground alpha.
#[inline]
pub fn cell_set_fg_alpha(cell: &mut NcCell, alpha: NcAlphaBits) {
    channels_set_fg_alpha(&mut cell.channels, alpha);
}

/// Set the background alpha.
#[inline]
pub fn cell_set_bg_alpha(cell: &mut NcCell, alpha: NcAlphaBits) {
    channels_set_bg_alpha(&mut cell.channels, alpha);
}

/// Does the cell contain an East Asian Wide codepoint?
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

/// Extract the three elements of a cell.
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

/// Extract the 32-bit background channel from a cell.
#[inline]
pub fn cell_bchannel(cell: &NcCell) -> NcChannel {
    channels_bchannel(cell.channels)
}

/// Extract the 32-bit foreground channel from a cell.
#[inline]
pub fn cell_fchannel(cell: &NcCell) -> NcChannel {
    channels_fchannel(cell.channels)
}

/// Set the 32-bit background channel of a cell.
#[inline]
pub fn cell_set_bchannel(cell: &mut NcCell, channel: NcChannel) -> NcChannels {
    channels_set_bchannel(&mut cell.channels, channel)
}

/// Set the 32-bit foreground channel of a cell.
#[inline]
pub fn cell_set_fchannel(cell: &mut NcCell, channel: NcChannel) -> NcChannels {
    channels_set_fchannel(&mut cell.channels, channel)
}

/// Extract 24 bits of foreground RGB from 'cell', shifted to LSBs.
#[inline]
pub fn cell_fg_rgb(cell: &NcCell) -> NcChannel {
    channels_fg_rgb(cell.channels)
}

/// Extract 24 bits of background RGB from 'cell', shifted to LSBs.
#[inline]
pub fn cell_bg_rgb(cell: &NcCell) -> NcChannel {
    channels_bg_rgb(cell.channels)
}

/// Extract 2 bits of foreground alpha from 'cell', shifted to LSBs.
#[inline]
pub fn cell_fg_alpha(cell: &NcCell) -> NcAlphaBits {
    channels_fg_alpha(cell.channels)
}

/// Extract 2 bits of background alpha from 'cell', shifted to LSBs.
#[inline]
pub fn cell_bg_alpha(cell: &NcCell) -> NcAlphaBits {
    channels_bg_alpha(cell.channels)
}

/// Extract 24 bits of foreground RGB from 'cell', split into components.
#[inline]
pub fn cell_fg_rgb8(
    cell: &NcCell,
    red: &mut NcColor,
    green: &mut NcColor,
    blue: &mut NcColor,
) -> NcChannel {
    channels_fg_rgb8(cell.channels, red, green, blue)
}

/// Extract 24 bits of background RGB from 'cell', split into components.
#[inline]
pub fn cell_bg_rgb8(
    cell: &NcCell,
    red: &mut NcColor,
    green: &mut NcColor,
    blue: &mut NcColor,
) -> NcChannel {
    channels_bg_rgb8(cell.channels, red, green, blue)
}

/// Set the r, g, and b cell for the foreground component of this 64-bit
/// 'cell' variable, and mark it as not using the default color.
#[inline]
pub fn cell_set_fg_rgb8(cell: &mut NcCell, red: NcColor, green: NcColor, blue: NcColor) {
    channels_set_fg_rgb8(&mut cell.channels, red, green, blue);
}

/// Same as `cell_set_fg_rgb8()` but with an assembled 24-bit RGB value.
#[inline]
pub fn cell_set_fg_rgb(cell: &mut NcCell, channel: NcChannel) {
    channels_set_fg_rgb(&mut cell.channels, channel);
}

/// Set the cell's foreground palette index, set the foreground palette index
/// bit, set it foreground-opaque, and clear the foreground default color bit.
///
// NOTE: this function now can't fail
#[inline]
pub fn cell_set_fg_palindex(cell: &mut NcCell, index: NcPaletteIndex) {
    cell.channels |= NCCELL_FGDEFAULT_MASK;
    cell.channels |= NCCELL_FG_PALETTE;
    cell_set_fg_alpha(cell, NCCELL_ALPHA_OPAQUE);
    cell.channels &= 0xff000000ffffffff as NcChannels;
    cell.channels |= (index as NcChannels) << 32;
}

///
#[inline]
pub fn cell_fg_palindex(cell: &NcCell) -> NcPaletteIndex {
    ((cell.channels & 0xff00000000 as NcChannels) >> 32) as NcPaletteIndex
}

/// Set the r, g, and b cell for the background component of this 64-bit
/// 'cell' variable, and mark it as not using the default color.
///
#[inline]
pub fn cell_set_bg_rgb8(cell: &mut NcCell, red: NcColor, green: NcColor, blue: NcColor) {
    channels_set_bg_rgb8(&mut cell.channels, red, green, blue);
}

/// Same as `cell_set_fg_rgb8()` but with an assembled 24-bit RGB value.
///
#[inline]
pub fn cell_set_bg_rgb(cell: &mut NcCell, channel: NcChannel) {
    channels_set_bg_rgb(&mut cell.channels, channel);
}

/// Set the cell's background palette index, set the background palette index
/// bit, set it background-opaque, and clear the background default color bit.
///
// NOTE: this function now can't fail
#[inline]
pub fn cell_set_bg_palindex(cell: &mut NcCell, index: NcPaletteIndex) {
    cell.channels |= NCCELL_BGDEFAULT_MASK as NcChannels;
    cell.channels |= NCCELL_BG_PALETTE as NcChannels;
    cell_set_bg_alpha(cell, NCCELL_ALPHA_OPAQUE);
    cell.channels &= 0xffffffffff000000;
    cell.channels |= index as NcChannels;
}

///
#[inline]
pub fn cell_bg_palindex(cell: &NcCell) -> NcPaletteIndex {
    (cell.channels & 0xff) as NcPaletteIndex
}

/// Is the foreground using the "default foreground color"?
#[inline]
pub fn cell_fg_default_p(cell: &NcCell) -> bool {
    channels_fg_default_p(cell.channels)
}

///
#[inline]
pub fn cell_fg_palindex_p(cell: &NcCell) -> bool {
    channels_fg_palindex_p(cell.channels)
}

/// Is the background using the "default background color"? The "default
/// background color" must generally be used to take advantage of
/// terminal-effected transparency.
#[inline]
pub fn cell_bg_default_p(cell: &NcCell) -> bool {
    channels_bg_default_p(cell.channels)
}

///
#[inline]
pub fn cell_bg_palindex_p(cell: &NcCell) -> bool {
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
