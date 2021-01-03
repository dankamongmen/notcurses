//! `NcCell` methods and associated functions.

use crate::{
    cell_load, cstring, error, NcAlphaBits, NcCell, NcChannel, NcChannelPair, NcColor, NcEgc,
    NcEgcBackstop, NcPaletteIndex, NcPlane, NcResult, NcRgb, NcStyleMask, NCRESULT_ERR,
};

/// # NcCell constructors
impl NcCell {
    /// New NcCell, expects a 7-bit [char].
    #[inline]
    pub const fn with_char7b(ch: char) -> Self {
        NcCell {
            gcluster: (ch as u32).to_le(),
            gcluster_backstop: 0 as NcEgcBackstop,
            width: 0_u8,
            stylemask: 0 as NcStyleMask,
            channels: 0 as NcChannelPair,
        }
    }

    /// New NcCell, expects an [NcPlane] and a [char].
    #[inline]
    pub fn with_char(ch: char, plane: &mut NcPlane) -> Self {
        let mut cell = Self::new();
        let result = unsafe { cell_load(plane, &mut cell, cstring![ch.to_string()]) };
        debug_assert_ne![NCRESULT_ERR, result];
        cell
    }

    /// New NcCell, expects an [NcPlane] and a &[str].
    #[inline]
    pub fn with_str(plane: &mut NcPlane, string: &str) -> Self {
        let mut cell = Self::new();
        let result = unsafe { cell_load(plane, &mut cell, cstring![string]) };
        debug_assert_ne![NCRESULT_ERR, result];
        cell
    }

    /// New empty NcCell.
    #[inline]
    pub const fn new() -> Self {
        Self::with_char7b(0 as char)
    }

    // TODO: it's not clear what it does
    //
    // /// Breaks the UTF-8 string in `gcluster` down, setting up this NcCell,
    // /// and returns the number of bytes copied out of `gcluster`.
    // ///
    // /// The styling of the cell is left untouched, but any resources are released.
    // /// *C style function: [cell_load()][crate::cell_load].*
    // pub fn load(
    //     plane: &mut NcPlane,
    //     cell: &mut NcCell,
    //     gcluster: NcEgc,
    // ) -> NcResult<u32> {
    //     let bytes = unsafe { crate::cell_load(plane, cell, gcluster as u32 as *const i8)};
    //     error![bytes, bytes as u32]
    // }

    /// Same as [load][NcCell#method.load], plus blasts the styling with
    /// `style` and `channels`.
    ///
    /// - Breaks the UTF-8 string in `gcluster` down, setting up this NcCell.
    /// - Returns the number of bytes copied out of `gcluster`.
    /// - The styling of the cell is left untouched, but any resources are released.
    /// - Blasts the styling with `style` and `channels`.
    ///
    /// *C style function: [cell_prime()][crate::cell_prime].*
    pub fn prime(
        plane: &mut NcPlane,
        cell: &mut NcCell,
        gcluster: &str,
        style: NcStyleMask,
        channels: NcChannelPair,
    ) -> NcResult<u32> {
        let bytes = crate::cell_prime(plane, cell, gcluster, style, channels);
        error![bytes, "", bytes as u32]
    }

    /// Duplicate this NcCell into another one.
    ///
    /// Both must be or will be bound to `common_plane`.
    ///
    /// *C style function: [cell_duplicate()][crate::cell_duplicate].*
    pub fn duplicate(&self, target: &mut NcCell, common_plane: &mut NcPlane) -> NcResult<()> {
        error![unsafe { crate::cell_duplicate(common_plane, target, self) }]
    }

    /// Initializes (zeroes out) the NcCell.
    ///
    /// *C style function: [cell_init()][crate::cell_init].*
    #[inline]
    pub fn init(&mut self) {
        crate::cell_init(self);
    }

    /// Releases resources held by the current cell in the [NcPlane] `plane`.
    ///
    /// *C style function: [cell_release()][crate::cell_release].*
    pub fn release(&mut self, plane: &mut NcPlane) {
        unsafe {
            crate::cell_release(plane, self);
        }
    }
}

// -----------------------------------------------------------------------------
/// ## NcCell methods: bg|fg `NcChannel`s manipulation.
impl NcCell {
    /// Returns the [NcChannelPair] of the NcCell.
    ///
    /// *(No equivalent C style function)*
    pub fn channels(&mut self, plane: &mut NcPlane) -> NcChannelPair {
        let (mut _styles, mut channels) = (0, 0);
        let _char = crate::cell_extract(plane, self, &mut _styles, &mut channels);
        channels
    }

    /// Gets the background [NcChannel].
    ///
    /// *C style function: [cell_bchannel()][crate::cell_bchannel].*
    pub fn bchannel(&self) -> NcChannel {
        crate::cell_bchannel(self)
    }

    /// Extracts the background [NcAlphaBits] (shifted to LSBs).
    ///
    /// *C style function: [cell_bg_alpha()][crate::cell_bg_alpha].*
    pub fn bg_alpha(&self) -> NcAlphaBits {
        crate::cell_bg_alpha(self)
    }

    /// Is the background [NcChannel] using the "default background color"?
    ///
    /// *C style function: [cell_bg_default_p()][crate::cell_bg_default_p].*
    pub fn bg_default_p(&self) -> bool {
        crate::cell_bg_default_p(self)
    }

    /// Gets the [NcPaletteIndex] of the background [NcChannel].
    ///
    /// *C style function: [cell_bg_palindex()][crate::cell_bg_palindex].*
    pub fn bg_palindex(&self) -> NcPaletteIndex {
        crate::cell_bg_palindex(self)
    }

    /// Is the background [NcChannel] using an [NcPaletteIndex] indexed
    /// [NcPalette][crate::NcPalette] color?
    ///
    /// *C style function: [cell_bg_palindex_p()][crate::cell_bg_palindex_p].*
    pub fn bg_palindex_p(&self) -> bool {
        crate::cell_bg_palindex_p(self)
    }

    /// Gets the background [NcRgb] (shifted to LSBs).
    ///
    /// *C style function: [cell_bg_rgb()][crate::cell_bg_rgb].*
    pub fn bg_rgb(&self) -> NcRgb {
        crate::cell_bg_rgb(self)
    }

    /// Gets the background [NcColor] RGB components.
    ///
    /// *C style function: [cell_bg_rgb8()][crate::cell_bg_rgb8].*
    pub fn bg_rgb8(&self) -> (NcColor, NcColor, NcColor) {
        let (mut r, mut g, mut b) = (0, 0, 0);
        crate::cell_bg_rgb8(self, &mut r, &mut g, &mut b);
        (r, g, b)
    }

    /// Gets the foreground [NcChannel].
    ///
    /// *C style function: [cell_fchannel()][crate::cell_fchannel].*
    pub fn fchannel(&self) -> NcChannel {
        crate::cell_fchannel(self)
    }

    /// Extracts the foreground [NcAlphaBits] (shifted to LSBs).
    ///
    /// *C style function: [cell_fg_alpha()][crate::cell_fg_alpha].*
    pub fn fg_alpha(&self) -> NcAlphaBits {
        crate::cell_fg_alpha(self)
    }

    /// Is the foreground [NcChannel] using the "default foreground color"?
    ///
    /// *C style function: [cell_fg_default_p()][crate::cell_fg_default_p].*
    pub fn fg_default_p(&self) -> bool {
        crate::cell_fg_default_p(self)
    }

    /// Gets the [NcPaletteIndex] of the foreground [NcChannel].
    ///
    /// *C style function: [cell_fg_palindex()][crate::cell_fg_palindex].*
    pub fn fg_palindex(&self) -> NcPaletteIndex {
        crate::cell_fg_palindex(self)
    }

    /// Is the foreground [NcChannel] using an [NcPaletteIndex] indexed
    /// [NcPalette][crate::NcPalette] color?
    ///
    /// *C style function: [cell_fg_palindex_p()][crate::cell_fg_palindex_p].*
    pub fn fg_palindex_p(&self) -> bool {
        crate::cell_fg_palindex_p(self)
    }

    /// Gets the foreground [NcRgb] (shifted to LSBs).
    ///
    /// *C style function: [cell_fg_rgb()][crate::cell_fg_rgb].*
    pub fn fg_rgb(&self) -> NcRgb {
        crate::cell_fg_rgb(self)
    }

    /// Gets the foreground [NcColor] RGB components.
    ///
    /// *C style function: [cell_fg_rgb8()][crate::cell_fg_rgb8].*
    pub fn fg_rgb8(&self) -> (NcColor, NcColor, NcColor) {
        let (mut r, mut g, mut b) = (0, 0, 0);
        crate::cell_fg_rgb8(self, &mut r, &mut g, &mut b);
        (r, g, b)
    }

    /// Sets the background [NcChannel] and returns the new [NcChannelPair].
    ///
    /// *C style function: [cell_set_bchannel()][crate::cell_set_bchannel].*
    pub fn set_bchannel(&mut self, channel: NcChannel) -> NcChannelPair {
        crate::cell_set_bchannel(self, channel)
    }

    /// Sets the background [NcAlphaBits].
    ///
    /// *C style function: [cell_set_bg_alpha()][crate::cell_set_bg_alpha].*
    pub fn set_bg_alpha(&mut self, alpha: NcAlphaBits) {
        crate::cell_set_bg_alpha(self, alpha);
    }

    /// Indicates to use the "default color" for the background [NcChannel].
    ///
    /// *C style function: [cell_set_bg_default()][crate::cell_set_bg_default].*
    pub fn set_bg_default(&mut self) {
        crate::cell_set_bg_default(self);
    }

    /// Sets the background [NcPaletteIndex].
    ///
    /// Also sets [NCCELL_BG_PALETTE][crate::NCCELL_BG_PALETTE] and
    /// [NCCELL_ALPHA_OPAQUE][crate::NCCELL_ALPHA_OPAQUE], and clears out
    /// [NCCELL_BGDEFAULT_MASK][crate::NCCELL_BGDEFAULT_MASK].
    ///
    /// *C style function: [cell_set_bg_palindex()][crate::cell_set_bg_palindex].*
    pub fn set_bg_palindex(&mut self, index: NcPaletteIndex) {
        crate::cell_set_bg_palindex(self, index);
    }

    /// Sets the background [NcRgb] and marks it as not using the default color.
    ///
    /// *C style function: [cell_set_bg_rgb()][crate::cell_set_bg_rgb].*
    pub fn set_bg_rgb(&mut self, rgb: NcRgb) {
        crate::cell_set_bg_rgb(self, rgb);
    }

    /// Sets the background [NcColor] RGB components, and marks it as not using
    /// the "default color".
    ///
    /// *C style function: [cell_set_bg_rgb8()][crate::cell_set_bg_rgb8].*
    pub fn set_bg_rgb8(&mut self, red: NcColor, green: NcColor, blue: NcColor) {
        crate::cell_set_bg_rgb8(self, red, green, blue);
    }

    /// Sets the foreground [NcChannel] and returns the new [NcChannelPair].
    ///
    /// *C style function: [cell_set_fchannel()][crate::cell_set_fchannel].*
    pub fn set_fchannel(&mut self, channel: NcChannel) -> NcChannelPair {
        crate::cell_set_fchannel(self, channel)
    }

    /// Sets the foreground [NcAlphaBits].
    ///
    /// *C style function: [cell_set_fg_alpha()][crate::cell_set_fg_alpha].*
    pub fn set_fg_alpha(&mut self, alpha: NcAlphaBits) {
        crate::cell_set_fg_alpha(self, alpha);
    }

    /// Indicates to use the "default color" for the foreground [NcChannel].
    ///
    /// *C style function: [cell_set_fg_default()][crate::cell_set_fg_default].*
    pub fn set_fg_default(&mut self) {
        crate::cell_set_fg_default(self);
    }

    /// Sets the foreground [NcPaletteIndex].
    ///
    /// Also sets [NCCELL_FG_PALETTE][crate::NCCELL_FG_PALETTE] and
    /// [NCCELL_ALPHA_OPAQUE][crate::NCCELL_ALPHA_OPAQUE], and clears out
    /// [NCCELL_BGDEFAULT_MASK][crate::NCCELL_BGDEFAULT_MASK].
    ///
    /// *C style function: [cell_set_fg_palindex()][crate::cell_set_fg_palindex].*
    pub fn set_fg_palindex(&mut self, index: NcPaletteIndex) {
        crate::cell_set_fg_palindex(self, index);
    }

    /// Sets the foreground [NcRgb] and marks it as not using the default color.
    ///
    /// *C style function: [cell_set_fg_rgb()][crate::cell_set_fg_rgb].*
    pub fn set_fg_rgb(&mut self, rgb: NcRgb) {
        crate::cell_set_fg_rgb(self, rgb);
    }

    /// Sets the foreground [NcColor] RGB components, and marks it as not using
    /// the "default color".
    ///
    /// *C style function: [cell_set_fg_rgb8()][crate::cell_set_fg_rgb8].*
    pub fn set_fg_rgb8(&mut self, red: NcColor, green: NcColor, blue: NcColor) {
        crate::cell_set_fg_rgb8(self, red, green, blue);
    }
}

/// # `NcCell` methods: other components
impl NcCell {
    /// Returns true if the two cells have distinct [NcEgc]s, attributes,
    /// or [NcChannel]s.
    ///
    /// The actual egcpool index needn't be the same--indeed, the planes
    /// needn't even be the same. Only the expanded NcEgc must be bit-equal.
    ///
    /// *C style function: [cellcmp()][crate::cellcmp].*
    pub fn compare(plane1: &NcPlane, cell1: &NcCell, plane2: &NcPlane, cell2: &NcCell) -> bool {
        crate::cellcmp(plane1, cell1, plane2, cell2)
    }

    /// Saves the [NcStyleMask] and the [NcChannelPair], and returns the [NcEgc]
    /// (the three elements of an NcCell).
    ///
    /// *C style function: [cell_fg_alpha()][crate::cell_fg_alpha].*
    pub fn extract(
        &mut self,
        plane: &mut NcPlane,
        styles: &mut NcStyleMask,
        channels: &mut NcChannelPair,
    ) -> NcEgc {
        crate::cell_extract(plane, self, styles, channels)
    }

    /// Returns the [NcEgc] of the NcCell.
    ///
    /// See also: [extended_gcluster][NcCell#method.extended_gcluster] method.
    ///
    /// *(No equivalent C style function)*
    pub fn egc(&mut self, plane: &mut NcPlane) -> NcEgc {
        let (mut _styles, mut _channels) = (0, 0);
        crate::cell_extract(plane, self, &mut _styles, &mut _channels)
    }

    /// Returns the [NcStyleMask] bits.
    ///
    /// *C style function: [cell_styles()][crate::cell_styles].*
    pub fn styles(&mut self) -> NcStyleMask {
        crate::cell_styles(self)
    }

    /// Removes the specified [NcStyleMask] bits.
    ///
    /// *C style function: [cell_off()][crate::cell_off_styles].*
    pub fn off_styles(&mut self, stylebits: NcStyleMask) {
        crate::cell_off_styles(self, stylebits)
    }

    /// Adds the specified [NcStyleMask] bits.
    ///
    /// *C style function: [cell_on()][crate::cell_on_styles].*
    pub fn on_styles(&mut self, stylebits: NcStyleMask) {
        crate::cell_on_styles(self, stylebits)
    }

    /// Sets just the specified [NcStyleMask] bits.
    ///
    /// *C style function: [cell_set_styles()][crate::cell_set_styles].*
    pub fn set_styles(&mut self, stylebits: NcStyleMask) {
        crate::cell_set_styles(self, stylebits)
    }
}

/// # `NcCell` methods: text
impl NcCell {
    // /// Returns a pointer to the [NcEgc] of this NcCell in the [NcPlane] `plane`.
    // ///
    // /// This pointer can be invalidated by any further operation on the referred
    // /// plane, so… watch out!
    // ///
    // /// *C style function: [cell_extended_gcluster()][crate::cell_wide_left_p].*
    // pub fn extended_gcluster(&self, plane: &NcPlane) -> u32 {
    //     let egcpointer = unsafe { crate::cell_extended_gcluster(plane, self) };
    //     egcpointer
    // }

    /// Copies the UTF8-encoded [NcEgc] out of this NcCell,
    /// whether simple or complex.
    ///
    /// The result is not tied to the [NcPlane],
    /// and persists across erases and destruction.
    ///
    /// *C style function: [cell_strdup()][crate::cell_strdup].*
    pub fn strdup(&self, plane: &NcPlane) -> NcEgc {
        crate::cell_strdup(plane, self)
    }

    /// Does this NcCell contain a wide codepoint?
    ///
    /// *C style function: [cell_double_wide_p()][crate::cell_double_wide_p].*
    pub fn double_wide_p(&self) -> bool {
        crate::cell_double_wide_p(self)
    }

    /// Is this the left half of a wide character?
    ///
    /// *C style function: [cell_wide_left_p()][crate::cell_wide_left_p].*
    pub fn wide_left_p(&self) -> bool {
        crate::cell_wide_right_p(self)
    }

    /// Is this the right side of a wide character?
    ///
    /// *C style function: [cell_wide_right_p()][crate::cell_wide_right_p].*
    pub fn wide_right_p(&self) -> bool {
        crate::cell_wide_right_p(self)
    }
}

/// # `NcCell` methods: boxes
impl NcCell {
    /// Loads up six cells with the [NcEgc]s necessary to draw a box.
    ///
    /// On error, any [NcCell]s this function might have loaded before the error
    /// are [release][NcCell#method.release]d.
    /// There must be at least six [NcEgc]s in `gcluster`.
    ///
    /// *C style function: [cells_load_box()][crate::cells_load_box].*
    pub fn load_box(
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
    ) -> NcResult<()> {
        error![crate::cells_load_box(
            plane, style, channels, ul, ur, ll, lr, hl, vl, gcluster
        )]
    }

    /// NcCell.[load_box()][NcCell#method.box] with the double box-drawing characters.
    ///
    /// *C style function: [cells_double_box()][crate::cells_double_box].*
    pub fn double_box(
        plane: &mut NcPlane,
        style: NcStyleMask,
        channels: NcChannelPair,
        ul: &mut NcCell,
        ur: &mut NcCell,
        ll: &mut NcCell,
        lr: &mut NcCell,
        hl: &mut NcCell,
        vl: &mut NcCell,
    ) -> NcResult<()> {
        error![unsafe {
            crate::cells_double_box(plane, style as u32, channels, ul, ur, ll, lr, hl, vl)
        }]
    }

    /// NcCell.[load_box()][NcCell#method.box] with the rounded box-drawing characters.
    ///
    /// *C style function: [cells_rounded_box()][crate::cells_double_box].*
    pub fn rounded_box(
        plane: &mut NcPlane,
        style: NcStyleMask,
        channels: NcChannelPair,
        ul: &mut NcCell,
        ur: &mut NcCell,
        ll: &mut NcCell,
        lr: &mut NcCell,
        hl: &mut NcCell,
        vl: &mut NcCell,
    ) -> NcResult<()> {
        error![unsafe {
            crate::cells_rounded_box(plane, style as u32, channels, ul, ur, ll, lr, hl, vl)
        }]
    }
}
