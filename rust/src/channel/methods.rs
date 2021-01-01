//! `NcChannel*` methods and associated functions.

use crate::{NcAlphaBits, NcChannel, NcChannelPair, NcColor, NcPaletteIndex, NcRgb};

/// Enables the [NcChannel] methods.
pub trait NcChannelMethods {
    // constructors
    fn new() -> Self;
    fn with_default() -> Self;
    fn with_rgb(rgb: NcRgb) -> Self;
    fn with_rgb_alpha(rgb: NcRgb, alpha: NcAlphaBits) -> Self;
    fn with_rgb8(r: NcColor, g: NcColor, b: NcColor) -> Self;
    fn with_rgb8_alpha(r: NcColor, g: NcColor, b: NcColor, alpha: NcAlphaBits) -> Self;

    // methods
    fn fcombine(&self, bchannel: NcChannel) -> NcChannelPair;
    fn bcombine(&self, fchannel: NcChannel) -> NcChannelPair;

    fn alpha(&self) -> NcAlphaBits;
    fn set_alpha(&mut self, alpha: NcAlphaBits);

    fn set(&mut self, rgb: NcRgb);

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

    fn palindex_p(&self) -> bool;
}

/// Enables the [NcChannelPair] methods.
pub trait NcChannelPairMethods {
    // constructors
    // …

    // methods
    fn combine(fchannel: NcChannel, bchannel: NcChannel) -> NcChannelPair;

    fn fchannel(&self) -> NcChannel;
    fn bchannel(&self) -> NcChannel;
    fn set_fchannel(&mut self, fchannel: NcChannel) -> NcChannelPair;
    fn set_bchannel(&mut self, bchannel: NcChannel) -> NcChannelPair;

    fn fg_alpha(&self) -> NcAlphaBits;
    fn bg_alpha(&self) -> NcAlphaBits;
    fn set_fg_alpha(&mut self, alpha: NcAlphaBits);
    fn set_bg_alpha(&mut self, alpha: NcAlphaBits);

    fn fg_rgb(&self) -> NcRgb;
    fn bg_rgb(&self) -> NcRgb;
    fn set_fg_rgb(&mut self, alpha: NcAlphaBits);
    fn set_bg_rgb(&mut self, alpha: NcAlphaBits);

    fn fg_rgb8(&self) -> (NcColor, NcColor, NcColor);
    fn bg_rgb8(&self) -> (NcColor, NcColor, NcColor);
    fn set_fg_rgb8(&mut self, r: NcColor, g: NcColor, b: NcColor) -> NcChannelPair;
    fn set_bg_rgb8(&mut self, r: NcColor, g: NcColor, b: NcColor) -> NcChannelPair;
    fn fg_r(&self) -> NcColor;
    fn fg_g(&self) -> NcColor;
    fn fg_b(&self) -> NcColor;
    fn bg_r(&self) -> NcColor;
    fn bg_g(&self) -> NcColor;
    fn bg_b(&self) -> NcColor;
    fn fg_set_r(&mut self, r: NcColor) -> NcChannelPair;
    fn fg_set_g(&mut self, g: NcColor) -> NcChannelPair;
    fn fg_set_b(&mut self, b: NcColor) -> NcChannelPair;
    fn bg_set_r(&mut self, r: NcColor) -> NcChannelPair;
    fn bg_set_g(&mut self, g: NcColor) -> NcChannelPair;
    fn bg_set_b(&mut self, b: NcColor) -> NcChannelPair;

    fn fg_default_p(&self) -> bool;
    fn bg_default_p(&self) -> bool;
    fn set_fg_default(&mut self) -> NcChannelPair;
    fn set_bg_default(&mut self) -> NcChannelPair;

    fn fg_palindex_p(&self) -> bool;
    fn bg_palindex_p(&self) -> bool;
    fn set_fg_palindex(&mut self, index: NcPaletteIndex);
    fn set_bg_palindex(&mut self, index: NcPaletteIndex);
}

// NcChannel -------------------------------------------------------------------

/// # `NcChannel` Methods
impl NcChannelMethods for NcChannel {
    // Constructors

    /// New NcChannel, set to black and NOT using the default color.
    fn new() -> Self {
        0 as NcChannel | crate::NCCELL_BGDEFAULT_MASK
    }

    /// New NcChannel, set to black but using the default color.
    fn with_default() -> Self {
        0 as NcChannel
    }

    /// New NcChannel, expects [NcRgb].
    fn with_rgb(rgb: NcRgb) -> Self {
        let mut channel = 0;
        crate::channel_set(&mut channel, rgb);
        channel
    }

    /// New NcChannel, expects [NcRgb] & [NcAlphaBits].
    fn with_rgb_alpha(rgb: NcRgb, alpha: NcAlphaBits) -> Self {
        let mut channel = 0;
        crate::channel_set(&mut channel, rgb);
        crate::channel_set_alpha(&mut channel, alpha);
        channel
    }

    /// New NcChannel, expects three RGB [NcColor] components.
    fn with_rgb8(r: NcColor, g: NcColor, b: NcColor) -> Self {
        let mut channel = 0;
        crate::channel_set_rgb8(&mut channel, r, g, b);
        channel
    }

    /// New NcChannel, expects three RGB [NcColor] components.
    fn with_rgb8_alpha(r: NcColor, g: NcColor, b: NcColor, alpha: NcAlphaBits) -> Self {
        let mut channel = 0;
        crate::channel_set_rgb8(&mut channel, r, g, b);
        crate::channel_set_alpha(&mut channel, alpha);
        channel
    }

    // Combine

    /// Combines this [NcChannel] as foreground, with another as background
    /// into an [NcChannelPair].
    ///
    /// *C style function: [channels_combine()][crate::channels_combine].*
    //
    // Not in the C API
    fn fcombine(&self, bchannel: NcChannel) -> NcChannelPair {
        crate::channels_combine(*self, bchannel)
    }

    /// Combines this [NcChannel] as background, with another as foreground
    /// into an [NcChannelPair].
    ///
    /// *C style function: [channels_combine()][crate::channels_combine].*
    //
    // Not in the C API
    fn bcombine(&self, fchannel: NcChannel) -> NcChannelPair {
        crate::channels_combine(fchannel, *self)
    }

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

    // NcRgb

    /// Sets the [NcRgb], and marks the NcChannel as NOT using the
    /// "default color", retaining the other bits unchanged.
    ///
    /// *C style function: [channel_set()][crate::channel_set].*
    fn set(&mut self, rgb: NcRgb) {
        crate::channel_set(self, rgb);
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

    // default color

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

    // NcPaletteIndex

    /// Is this NcChannel using palette-indexed color rather than RGB?
    ///
    /// *C style function: [channel_set_default()][crate::channel_set_default].*
    fn palindex_p(&self) -> bool {
        crate::channel_palindex_p(*self)
    }
}

// NcChannelPair ---------------------------------------------------------------

/// # `NcChannelPair` Methods
impl NcChannelPairMethods for NcChannelPair {
    // Combine

    /// Combines two [NcChannel]s into an [NcChannelPair].
    ///
    /// *C style function: [channels_combine()][crate::channels_combine].*
    fn combine(fchannel: NcChannel, bchannel: NcChannel) -> NcChannelPair {
        crate::channels_combine(fchannel, bchannel)
    }

    // NcChannel

    /// Extracts the foreground [NcChannel].
    ///
    /// *C style function: [channels_fchannel()][crate::channels_fchannel].*
    fn fchannel(&self) -> NcChannel {
        crate::channels_fchannel(*self)
    }

    /// Extracts the background [NcChannel].
    ///
    /// *C style function: [channels_bchannel()][crate::channels_bchannel].*
    fn bchannel(&self) -> NcChannel {
        crate::channels_bchannel(*self)
    }

    /// Sets the foreground [NcChannel].
    ///
    /// *C style function: [channels_set_fchannel()][crate::channels_set_fchannel].*
    fn set_fchannel(&mut self, fchannel: NcChannel) -> NcChannelPair {
        crate::channels_set_fchannel(self, fchannel)
    }

    /// Sets the background [NcChannel].
    ///
    /// *C style function: [channels_set_bchannel()][crate::channels_set_bchannel].*
    fn set_bchannel(&mut self, bchannel: NcChannel) -> NcChannelPair {
        crate::channels_set_bchannel(self, bchannel)
    }

    // Alpha

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

    // NcRgb

    /// Gets the foreground [NcRgb].
    ///
    /// *C style function: [channels_fg_rgb()][crate::channels_fg_rgb].*
    fn fg_rgb(&self) -> NcRgb {
        crate::channels_fg_rgb(*self)
    }

    /// Gets the background [NcRgb].
    ///
    /// *C style function: [channels_bg_rgb()][crate::channels_bg_rgb].*
    fn bg_rgb(&self) -> NcRgb {
        crate::channels_bg_rgb(*self)
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

    // NcColor

    /// Gets the three foreground [NcColor]s (r, g, b).
    ///
    /// *C style function: [channels_fg_rgb8()][crate::channels_fg_rgb8].*
    fn fg_rgb8(&self) -> (NcColor, NcColor, NcColor) {
        let (mut r, mut g, mut b) = (0, 0, 0);
        crate::channels_fg_rgb8(*self, &mut r, &mut g, &mut b);
        (r, g, b)
    }

    /// Gets the three background [NcColor]s (r, g, b).
    ///
    /// *C style function: [channels_bg_rgb8()][crate::channels_bg_rgb8].*
    fn bg_rgb8(&self) -> (NcColor, NcColor, NcColor) {
        let (mut r, mut g, mut b) = (0, 0, 0);
        crate::channels_bg_rgb8(*self, &mut r, &mut g, &mut b);
        (r, g, b)
    }

    /// Sets the three foreground [NcColor]s (r, g, b), and
    /// marks the foreground [NcChannel] as not using the "default color".
    ///
    /// *C style function: [channels_set_fg_rgb8()][crate::channels_set_fg_rgb8].*
    fn set_fg_rgb8(&mut self, r: NcColor, g: NcColor, b: NcColor) -> NcChannelPair {
        crate::channels_set_fg_rgb8(self, r, g, b)
    }

    /// Sets the three background [NcColor]s (r, g, b), and
    /// marks the background [NcChannel] as not using the "default color".
    ///
    /// *C style function: [channels_set_bg_rgb8()][crate::channels_set_bg_rgb8].*
    fn set_bg_rgb8(&mut self, r: NcColor, g: NcColor, b: NcColor) -> NcChannelPair {
        crate::channels_set_bg_rgb8(self, r, g, b)
    }

    /// Gets the foreground red [NcColor].
    ///
    /// *(No equivalent C style function)*
    fn fg_r(&self) -> NcColor {
        crate::channel_r(crate::channels_fchannel(*self))
    }

    /// Gets the foreground green [NcColor].
    ///
    /// *(No equivalent C style function)*
    fn fg_g(&self) -> NcColor {
        crate::channel_g(crate::channels_fchannel(*self))
    }

    /// Gets the foreground blue [NcColor].
    ///
    /// *(No equivalent C style function)*
    fn fg_b(&self) -> NcColor {
        crate::channel_b(crate::channels_fchannel(*self))
    }

    /// Gets the background red [NcColor].
    ///
    /// *(No equivalent C style function)*
    fn bg_r(&self) -> NcColor {
        crate::channel_r(crate::channels_bchannel(*self))
    }

    /// Gets the background green [NcColor].
    ///
    /// *(No equivalent C style function)*
    fn bg_g(&self) -> NcColor {
        crate::channel_g(crate::channels_bchannel(*self))
    }

    /// Gets the background blue [NcColor].
    ///
    /// *(No equivalent C style function)*
    fn bg_b(&self) -> NcColor {
        crate::channel_b(crate::channels_bchannel(*self))
    }

    /// Sets the foreground red [NcColor], and returns the new NcChannelPair.
    ///
    /// *(No equivalent C style function)*
    fn fg_set_r(&mut self, r: NcColor) -> NcChannelPair {
        let (_, g, b) = self.bg_rgb8();
        crate::channels_set_fg_rgb8(self, r, g, b)
    }

    /// Sets the foreground green [NcColor], and returns the new NcChannelPair.
    ///
    /// *(No equivalent C style function)*
    fn fg_set_g(&mut self, g: NcColor) -> NcChannelPair {
        let (r, _, b) = self.bg_rgb8();
        crate::channels_set_fg_rgb8(self, r, g, b)
    }

    /// Sets the foreground blue [NcColor], and returns the new NcChannelPair.
    ///
    /// *(No equivalent C style function)*
    fn fg_set_b(&mut self, b: NcColor) -> NcChannelPair {
        let (r, g, _) = self.bg_rgb8();
        crate::channels_set_fg_rgb8(self, r, g, b)
    }

    /// Sets the background red [NcColor], and returns the new NcChannelPair.
    ///
    /// *(No equivalent C style function)*
    fn bg_set_r(&mut self, r: NcColor) -> NcChannelPair {
        let (_, g, b) = self.bg_rgb8();
        crate::channels_set_bg_rgb8(self, r, g, b)
    }

    /// Sets the background green [NcColor], and returns the new NcChannelPair.
    ///
    /// *(No equivalent C style function)*
    fn bg_set_g(&mut self, g: NcColor) -> NcChannelPair {
        let (r, _, b) = self.bg_rgb8();
        crate::channels_set_bg_rgb8(self, r, g, b)
    }

    /// Sets the background blue [NcColor], and returns the new NcChannelPair.
    ///
    /// *(No equivalent C style function)*
    fn bg_set_b(&mut self, b: NcColor) -> NcChannelPair {
        let (r, g, _) = self.bg_rgb8();
        crate::channels_set_bg_rgb8(self, r, g, b)
    }

    // default color

    /// Is the background using the "default background color"?
    ///
    /// *C style function: [channels_fg_default_p()][crate::channels_fg_default_p].*
    fn fg_default_p(&self) -> bool {
        crate::channels_fg_default_p(*self)
    }

    /// Is the background using the "default background color"?
    ///
    /// The "default background color" must generally be used to take advantage
    /// of terminal-effected transparency.
    ///
    /// *C style function: [channels_bg_default_p()][crate::channels_bg_default_p].*
    fn bg_default_p(&self) -> bool {
        crate::channels_bg_default_p(*self)
    }

    /// Marks the foreground as using its "default color", and
    /// returns the new [NcChannelPair].
    ///
    /// *C style function: [channels_set_fg_default()][crate::channels_set_fg_default].*
    fn set_fg_default(&mut self) -> NcChannelPair {
        crate::channels_set_fg_default(self)
    }

    /// Marks the background as using its "default color", and
    /// returns the new [NcChannelPair].
    ///
    /// *C style function: [channels_set_bg_default()][crate::channels_set_bg_default].*
    fn set_bg_default(&mut self) -> NcChannelPair {
        crate::channels_set_bg_default(self)
    }

    // NcPaletteIndex

    /// Is the foreground of using an [indexed][NcPaletteIndex]
    /// [NcPalette][crate::NcPalette] color?
    ///
    /// *C style function: [channels_fg_palindex_p()][crate::channels_fg_palindex_p].*
    fn fg_palindex_p(&self) -> bool {
        crate::channels_fg_palindex_p(*self)
    }

    /// Is the background of using an [indexed][NcPaletteIndex]
    /// [NcPalette][crate::NcPalette] color?
    ///
    /// *C style function: [channels_bg_palindex_p()][crate::channels_bg_palindex_p].*
    fn bg_palindex_p(&self) -> bool {
        crate::channels_bg_palindex_p(*self)
    }

    /// Sets the foreground of an [NcChannelPair] as using an
    /// [indexed][NcPaletteIndex] [NcPalette][crate::NcPalette] color.
    ///
    /// *C style function: [channels_set_fg_palindex()][crate::channels_set_fg_palindex].*
    fn set_fg_palindex(&mut self, index: NcPaletteIndex) {
        crate::channels_set_fg_palindex(self, index)
    }

    /// Sets the background of an [NcChannelPair] as using an
    /// [indexed][NcPaletteIndex] [NcPalette][crate::NcPalette] color.
    ///
    /// *C style function: [channels_set_bg_palindex()][crate::channels_set_bg_palindex].*
    fn set_bg_palindex(&mut self, index: NcPaletteIndex) {
        crate::channels_set_bg_palindex(self, index)
    }
}
