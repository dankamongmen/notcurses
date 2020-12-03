//! Handy [NcCell] constructors

pub use crate::{NcCell, NcChannelPair, NcCharBackstop, NcStyleMask};

impl NcCell {
    /// New NcCell, expects a [char], [NcStyleMask] and [NcChannelPair].
    #[inline]
    pub const fn new(ch: char, stylemask: NcStyleMask, channels: NcChannelPair) -> Self {
        NcCell {
            gcluster: ch as u32,
            gcluster_backstop: 0 as NcCharBackstop,
            reserved: 0,
            stylemask,
            channels,
        }
    }

    /// New NcCell, expects a [char].
    #[inline]
    pub const fn with_char(ch: char) -> Self {
        Self::new(ch, 0 as NcStyleMask, 0 as NcChannelPair)
    }

    /// New NcCell, blank.
    #[inline]
    pub const fn new_blank() -> Self {
        Self::with_char(0 as char)
    }
}
