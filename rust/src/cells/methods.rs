//! `NcCell` methods and associated functions.

use crate::{
    cell_extract, cell_load, cstring, NcCell, NcChannelPair, NcEgc, NcEgcBackstop, NcPlane,
    NcStyleMask, NCRESULT_ERR,
};

/// # `NcCell` Constructors
impl NcCell {
    /// New NcCell, expects a [char], [NcStyleMask] and [NcChannelPair].
    #[inline]
    pub const fn with_all(ch: char, stylemask: NcStyleMask, channels: NcChannelPair) -> Self {
        NcCell {
            gcluster: ch as u32,
            gcluster_backstop: 0 as NcEgcBackstop,
            reserved: 0,
            stylemask,
            channels,
        }
    }

    /// New NcCell, expects a 7-bit [char].
    #[inline]
    pub const fn with_7bitchar(ch: char) -> Self {
        Self::with_all(ch, 0 as NcStyleMask, 0 as NcChannelPair)
    }

    /// New NcCell, expects an [NcPlane] and a [char].
    #[inline]
    pub fn with_char(plane: &mut NcPlane, ch: char) -> Self {
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

    /// New NcCell, blank.
    #[inline]
    pub const fn new() -> Self {
        Self::with_7bitchar(0 as char)
    }
}

/// # `NcCell` Methods
impl NcCell {
    /// Saves the [NcStyleMask] and the [NcChannelPair], and returns the [NcEgc]
    /// (the three elements of an NcCell).
    pub fn extract(
        &mut self,
        plane: &mut NcPlane,
        styles: &mut NcStyleMask,
        channels: &mut NcChannelPair,
    ) -> NcEgc {
        cell_extract(plane, self, styles, channels)
    }

    /// Saves the [NcChannelPair] of the NcCell.
    // not in the C API
    pub fn channels(&mut self, plane: &mut NcPlane, channels: &mut NcChannelPair) {
        let mut _styles = 0;
        let _char = cell_extract(plane, self, &mut _styles, channels);
    }

    /// Saves the [NcStyleMask] of the NcCell.
    // not in the C API
    pub fn styles(&mut self, plane: &mut NcPlane, styles: &mut NcStyleMask) {
        let mut _channels = 0;
        let _char = cell_extract(plane, self, styles, &mut _channels);
    }

    /// Returns the [NcEgc] of the NcCell.
    // not in the C API
    pub fn egc(&mut self, plane: &mut NcPlane) -> NcEgc {
        let (mut _styles, mut _channels) = (0, 0);
        cell_extract(plane, self, &mut _styles, &mut _channels)
    }
}
