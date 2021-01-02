//! `NcPalette` methods and associated functions.

use crate::{error, NcChannel, NcColor, NcNotcurses, NcPalette, NcPaletteIndex, NcResult, NcRgb};

impl NcPalette {
    /// New NcPalette.
    ///
    /// *C style function: [palette256_new()][crate::palette256_new].*
    pub fn new<'a>(nc: &mut NcNotcurses) -> &'a mut Self {
        unsafe { &mut *crate::palette256_new(nc) }
    }

    /// Frees this NcPalette.
    ///
    /// *C style function: [palette256_free()][crate::palette256_free].*
    pub fn free(&mut self) {
        unsafe {
            crate::palette256_free(self);
        }
    }

    /// Attempts to configure the terminal with this NcPalette.
    ///
    /// *C style function: [palette256_use()][crate::palette256_use].*
    pub fn r#use(&self, nc: &mut NcNotcurses) -> NcResult<()> {
        error![unsafe { crate::palette256_use(nc, self) }]
    }

    /// Returns the [NcColor] RGB components from the [NcChannel] in this NcPalette.
    ///
    /// *C style function: [palette256_get_rgb()][crate::palette256_get_rgb].*
    pub fn get_rgb8(&self, index: NcPaletteIndex) -> (NcColor, NcColor, NcColor) {
        let (mut r, mut g, mut b) = (0, 0, 0);
        crate::channel_rgb8(self.chans[index as usize], &mut r, &mut g, &mut b);
        (r, g, b)
    }

    /// Extracts the [NcColor] RGB components from an [NcChannel] entry inside
    /// this NcPalette, and returns the NcChannel.
    ///
    /// *C style function: [palette256_get_rgb()][crate::palette256_get_rgb].*
    pub fn get_rgb(&self, index: NcPaletteIndex) -> NcChannel {
        let (mut r, mut g, mut b) = (0, 0, 0);
        crate::channel_rgb8(self.chans[index as usize], &mut r, &mut g, &mut b)
    }

    /// Sets the [NcRgb] value of the [NcChannel][crate::NcChannel] entry
    /// inside this NcPalette.
    ///
    /// *C style function: [palette256_set()][crate::palette256_set].*
    pub fn set(&mut self, index: NcPaletteIndex, rgb: NcRgb) {
        crate::channel_set(&mut self.chans[index as usize], rgb);
    }
}
