//! Handy [NcCell] constructors

pub use crate::{NcCell, NcPlane, NcChannelPair, NcCharBackstop, NcStyleMask, cell_load, NCRESULT_ERR, cstring};

impl NcCell {
    /// New NcCell, expects a [char], [NcStyleMask] and [NcChannelPair].
    #[inline]
    pub const fn with_all(ch: char, stylemask: NcStyleMask, channels: NcChannelPair) -> Self {
        NcCell {
            gcluster: ch as u32,
            gcluster_backstop: 0 as NcCharBackstop,
            reserved: 0,
            stylemask,
            channels,
        }
    }

    /// New NcCell, expects a 7-bit [char].
    ///
    /// See also `with_char`.
    #[inline]
    pub const fn with_7bitchar(ch: char) -> Self {
        Self::with_all(ch, 0 as NcStyleMask, 0 as NcChannelPair)
    }

    /// New NcCell, expects an [NcPlane] and a utf-8 [char].
    ///
    /// See also `with_7bitchar`.
    #[inline]
    pub fn with_char(plane: &mut NcPlane, ch: char) -> Self {
        let mut cell = Self::new();
        let result = unsafe {
            cell_load(plane,
                &mut cell,
                cstring![ch.to_string()],
            )
        };
        debug_assert_ne![NCRESULT_ERR, result];
        cell
    }

    /// New NcCell, blank.
    #[inline]
    pub const fn new() -> Self {
        Self::with_7bitchar(0 as char)
    }
}
