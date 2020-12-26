//! `NcChannel*` methods and associated functions.

use crate::{NcAlphaBits, NcChannel, NcChannelPair, NcColor, NcRgb};

/// Enables the [NcChannel] methods.
pub trait NcChannelMethods {
    fn alpha(&self) -> NcAlphaBits;
    fn set_alpha(&mut self, alpha: NcAlphaBits);

    fn rgb8(&self) -> (NcColor, NcColor, NcColor);
    fn set_rgb8(&mut self, r: NcColor, g: NcColor, b: NcColor);
    fn r(&self) -> NcColor;
    fn g(&self) -> NcColor;
    fn b(&self) -> NcColor;
    fn set_r(&mut self, r: NcColor) -> NcChannel;
    fn set_g(&mut self, g: NcColor) -> NcChannel;
    fn set_b(&mut self, b: NcColor) -> NcChannel;

    fn rgb(&self) -> NcRgb;
    fn set_rgb(&mut self, rgb: NcRgb);

    fn default_p(&self) -> bool;
    fn set_default(&mut self) -> NcChannel;

    //
}

/// Enables the [NcChannelPair] methods.
pub trait NcChannelPairMethods {
    fn fg_alpha(&self) -> NcAlphaBits;
    fn bg_alpha(&self) -> NcAlphaBits;
    fn set_fg_alpha(&mut self, alpha: NcAlphaBits);
    fn set_bg_alpha(&mut self, alpha: NcAlphaBits);
    fn set_fg_rgb(&mut self, alpha: NcAlphaBits);
    fn set_bg_rgb(&mut self, alpha: NcAlphaBits);
}

// NcChannel -------------------------------------------------------------------

/// # `NcChannel` Methods
impl NcChannelMethods for NcChannel {
    // Alpha

    /// Gets the [NcAlphaBits].
    ///
    /// *C style function: [channel_alpha()][crate::channel_alpha].*
    fn alpha(&self) -> NcAlphaBits {
        crate::channel_alpha(*self)
    }

    /// Sets the [NcAlphaBits].
    ///
    /// *C style function: [channel_set_alpha()][crate::channel_set_alpha].*
    fn set_alpha(&mut self, alpha: NcAlphaBits) {
        crate::channel_set_alpha(self, alpha)
    }

    // NcColor

    /// Gets the three [NcColor]s.
    ///
    /// *C style function: [channel_rgb8()][crate::channel_rgb8].*
    fn rgb8(&self) -> (NcColor, NcColor, NcColor) {
        let (mut r, mut g, mut b) = (0, 0, 0);
        crate::channel_rgb8(*self, &mut r, &mut g, &mut b);
        (r, g, b)
    }

    /// Sets the three [NcColor]s, and
    /// marks the NcChannel as NOT using the default color.
    ///
    /// *C style function: [channel_set_rgb8()][crate::channel_set_rgb8].*
    fn set_rgb8(&mut self, r: NcColor, g: NcColor, b: NcColor) {
        crate::channel_set_rgb8(self, r, g, b);
    }

    /// Gets the red [NcColor].
    ///
    /// *C style function: [channel_r()][crate::channel_r].*
    fn r(&self) -> NcColor {
        crate::channel_r(*self)
    }

    /// Gets the green [NcColor].
    ///
    /// *C style function: [channel_g()][crate::channel_g].*
    fn g(&self) -> NcColor {
        crate::channel_g(*self)
    }

    /// Gets the blue [NcColor].
    ///
    /// *C style function: [channel_b()][crate::channel_b].*
    fn b(&self) -> NcColor {
        crate::channel_b(*self)
    }

    /// Sets the red [NcColor], and returns the new NcChannel.
    ///
    /// *C style function: [channel_set_r()][crate::channel_set_r].*
    //
    // Not in the C API
    fn set_r(&mut self, r: NcColor) -> NcChannel {
        crate::channel_set_r(self, r)
    }

    /// Sets the green [NcColor], and returns the new NcChannel.
    ///
    /// *C style function: [channel_set_g()][crate::channel_set_g].*
    //
    // Not in the C API
    fn set_g(&mut self, g: NcColor) -> NcChannel {
        crate::channel_set_g(self, g)
    }

    /// Sets the blue [NcColor], and returns the new NcChannel.
    ///
    /// *C style function: [channel_set_b()][crate::channel_set_b].*
    //
    // Not in the C API
    fn set_b(&mut self, b: NcColor) -> NcChannel {
        crate::channel_set_b(self, b)
    }

    // NcRgb

    /// Gets the [NcRgb].
    ///
    /// *C style function: [channel_rgb()][crate::channel_rgb].*
    //
    // Not in the C API
    fn rgb(&self) -> NcRgb {
        crate::channel_rgb(*self)
    }

    /// Sets the [NcRgb] and marks it as NOT using the default color,
    /// retaining the other bits unchanged.
    ///
    /// *C style function: [channel_set()][crate::channel_set].*
    fn set_rgb(&mut self, rgb: NcRgb) {
        crate::channel_set(self, rgb);
    }

    // Default

    /// Is this NcChannel using the "default color" rather than RGB/palette-indexed?
    ///
    /// *C style function: [channel_default_p()][crate::channel_default_p].*
    fn default_p(&self) -> bool {
        crate::channel_default_p(*self)
    }

    /// Marks an NcChannel as using its "default color", which also marks it opaque.
    ///
    /// *C style function: [channel_set_default()][crate::channel_set_default].*
    fn set_default(&mut self) -> NcChannel {
        crate::channel_set_default(self)
    }
}

// NcChannelPair ---------------------------------------------------------------

/// # `NcChannelPair` Methods
impl NcChannelPairMethods for NcChannelPair {
    /// Gets the foreground [NcAlphaBits].
    ///
    /// *C style function: [channels_fg_alpha()][crate::channels_fg_alpha].*
    fn fg_alpha(&self) -> NcAlphaBits {
        crate::channels_fg_alpha(*self)
    }

    /// Gets the background [NcAlphaBits].
    ///
    /// *C style function: [channels_bg_alpha()][crate::channels_bg_alpha].*
    fn bg_alpha(&self) -> NcAlphaBits {
        crate::channels_bg_alpha(*self)
    }

    /// Sets the foreground [NcAlphaBits].
    ///
    /// *C style function: [channels_set_fg_alpha()][crate::channels_set_fg_alpha].*
    fn set_fg_alpha(&mut self, alpha: NcAlphaBits) {
        crate::channels_set_fg_alpha(self, alpha)
    }

    /// Sets the background [NcAlphaBits].
    ///
    /// *C style function: [channels_set_bg_alpha()][crate::channels_set_bg_alpha].*
    fn set_bg_alpha(&mut self, alpha: NcAlphaBits) {
        crate::channels_set_bg_alpha(self, alpha)
    }

    /// Sets the foreground [NcRgb].
    ///
    /// *C style function: [channels_set_fg_rgb()][crate::channels_set_fg_rgb].*
    fn set_fg_rgb(&mut self, rgb: NcRgb) {
        crate::channels_set_fg_rgb(self, rgb)
    }

    /// Sets the background [NcRgb].
    ///
    /// *C style function: [channels_set_bg_rgb()][crate::channels_set_bg_rgb].*
    fn set_bg_rgb(&mut self, rgb: NcRgb) {
        crate::channels_set_bg_rgb(self, rgb)
    }
}
