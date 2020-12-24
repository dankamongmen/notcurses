//! `NcCell` methods and associated functions.

use crate::{
    cell_extract, cell_load, cstring, NcAlphaBits, NcCell, NcChannel, NcChannelPair, NcColor,
    NcEgc, NcEgcBackstop, NcPaletteIndex, NcPlane, NcRgb, NcStyleMask, NCRESULT_ERR,
};

/// # `NcCell` Constructors
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
}

// -----------------------------------------------------------------------------
/// ## NcPlane methods: bg|fg `NcChannel`s manipulation.
impl NcCell {
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

/// # `NcCell` Methods
impl NcCell {
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
        cell_extract(plane, self, styles, channels)
    }

    /// Returns the [NcChannelPair] of the NcCell.
    ///
    /// *(No equivalent C style function)*
    pub fn channels(&mut self, plane: &mut NcPlane) -> NcChannelPair {
        let (mut _styles, mut channels) = (0, 0);
        let _char = cell_extract(plane, self, &mut _styles, &mut channels);
        channels
    }

    /// Returns the [NcStyleMask] of the NcCell.
    ///
    /// *(No equivalent C style function)*
    pub fn styles(&mut self, plane: &mut NcPlane) -> NcStyleMask {
        let (mut styles, mut _channels) = (0, 0);
        let _char = cell_extract(plane, self, &mut styles, &mut _channels);
        styles
    }

    /// Returns the [NcEgc] of the NcCell.
    ///
    /// *(No equivalent C style function)*
    pub fn egc(&mut self, plane: &mut NcPlane) -> NcEgc {
        let (mut _styles, mut _channels) = (0, 0);
        cell_extract(plane, self, &mut _styles, &mut _channels)
    }
}
