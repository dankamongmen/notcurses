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

    //
}

/// Enables the [NcChannelPair] methods.
pub trait NcChannelPairMethods {
    fn fg_alpha(&self) -> NcAlphaBits;
    fn bg_alpha(&self) -> NcAlphaBits;
    fn set_fg_alpha(&mut self, alpha: NcAlphaBits);
    fn set_bg_alpha(&mut self, alpha: NcAlphaBits);
}

// NcChannel -------------------------------------------------------------------

/// # `NcChannel` Methods
impl NcChannelMethods for NcChannel {
    // Alpha

    /// Gets the [NcAlphaBits].
    fn alpha(&self) -> NcAlphaBits {
        crate::channel_alpha(*self)
    }

    /// Sets the [NcAlphaBits].
    fn set_alpha(&mut self, alpha: NcAlphaBits) {
        crate::channel_set_alpha(self, alpha)
    }

    // NcColor

    /// Gets the three [NcColor]s.
    fn rgb8(&self) -> (NcColor, NcColor, NcColor) {
        let (mut r, mut g, mut b) = (0, 0, 0);
        crate::channel_rgb8(*self, &mut r, &mut g, &mut b);
        (r, g, b)
    }

    /// Sets the three [NcColor]s, and
    /// marks the NcChannel as NOT using the default color.
    fn set_rgb8(&mut self, r: NcColor, g: NcColor, b: NcColor) {
        crate::channel_set_rgb8(self, r, g, b);
    }

    /// Gets the red [NcColor].
    fn r(&self) -> NcColor {
        crate::channel_r(*self)
    }

    /// Gets the green [NcColor].
    fn g(&self) -> NcColor {
        crate::channel_g(*self)
    }

    /// Gets the blue [NcColor].
    fn b(&self) -> NcColor {
        crate::channel_b(*self)
    }

    /// Sets the red [NcColor], and returns the new NcChannel.
    fn set_r(&mut self, r: NcColor) -> NcChannel {
        crate::channel_set_r(self, r)
    }

    /// Sets the green [NcColor], and returns the new NcChannel.
    fn set_g(&mut self, g: NcColor) -> NcChannel {
        crate::channel_set_g(self, g)
    }

    /// Sets the blue [NcColor], and returns the new NcChannel.
    fn set_b(&mut self, b: NcColor) -> NcChannel {
        crate::channel_set_b(self, b)
    }

    // NcRgb

    /// Gets the [NcRgb].
    fn rgb(&self) -> NcRgb {
        crate::channel_rgb(*self)
    }

    /// Sets the [NcRgb] and marks it as NOT using the default color,
    /// retaining the other bits unchanged.
    fn set_rgb(&mut self, rgb: NcRgb) {
        crate::channel_set(self, rgb);
    }
}

// NcChannelPair ---------------------------------------------------------------

/// # `NcChannelPair` Methods
impl NcChannelPairMethods for NcChannelPair {
    /// Gets the foreground [NcAlphaBits].
    fn fg_alpha(&self) -> NcAlphaBits {
        crate::channels_fg_alpha(*self)
    }

    /// Gets the background [NcAlphaBits].
    fn bg_alpha(&self) -> NcAlphaBits {
        crate::channels_bg_alpha(*self)
    }

    /// Sets the foreground [NcAlphaBits].
    fn set_fg_alpha(&mut self, alpha: NcAlphaBits) {
        crate::channels_set_fg_alpha(self, alpha)
    }

    /// Sets the background [NcAlphaBits].
    fn set_bg_alpha(&mut self, alpha: NcAlphaBits) {
        crate::channels_set_bg_alpha(self, alpha)
    }
}
