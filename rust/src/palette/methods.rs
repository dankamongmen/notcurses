//! `NcPalette` methods and associated functions.

use crate::{error, Nc, NcChannel, NcColor, NcPalette, NcPaletteIndex, NcResult, NcRgb};

impl NcPalette {
    /// New NcPalette.
    ///
    /// *C style function: [ncpalette_new()][crate::ncpalette_new].*
    pub fn new<'a>(nc: &mut Nc) -> &'a mut Self {
        unsafe { &mut *crate::ncpalette_new(nc) }
    }

    /// Frees this NcPalette.
    ///
    /// *C style function: [ncpalette_free()][crate::ncpalette_free].*
    pub fn free(&mut self) {
        unsafe {
            crate::ncpalette_free(self);
        }
    }

    /// Attempts to configure the terminal with this NcPalette.
    ///
    /// *C style function: [ncpalette_use()][crate::ncpalette_use].*
    pub fn r#use(&self, nc: &mut Nc) -> NcResult<()> {
        error![unsafe { crate::ncpalette_use(nc, self) }]
    }

    /// Returns the [NcColor] RGB components from the [NcChannel] in this NcPalette.
    ///
    /// *C style function: [ncpalette_get_rgb()][crate::ncpalette_get_rgb8].*
    pub fn get_rgb8(&self, index: NcPaletteIndex) -> (NcColor, NcColor, NcColor) {
        let (mut r, mut g, mut b) = (0, 0, 0);
        crate::ncchannel_rgb8(self.chans[index as usize], &mut r, &mut g, &mut b);
        (r, g, b)
    }

    /// Extracts the [NcColor] RGB components from an [NcChannel] entry inside
    /// this NcPalette, and returns the NcChannel.
    ///
    /// *C style function: [ncpalette_get_rgb()][crate::ncpalette_get_rgb8].*
    pub fn get_rgb(&self, index: NcPaletteIndex) -> NcChannel {
        let (mut r, mut g, mut b) = (0, 0, 0);
        crate::ncchannel_rgb8(self.chans[index as usize], &mut r, &mut g, &mut b)
    }

    /// Sets the [NcRgb] value of the [NcChannel][crate::NcChannel] entry
    /// inside this NcPalette.
    ///
    /// *C style function: [ncpalette_set()][crate::ncpalette_set].*
    pub fn set(&mut self, index: NcPaletteIndex, rgb: NcRgb) {
        crate::ncchannel_set(&mut self.chans[index as usize], rgb);
    }
}
