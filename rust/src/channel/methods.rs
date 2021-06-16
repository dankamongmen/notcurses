//! `NcChannel*` methods and associated functions.
#![allow(clippy::unnecessary_cast)]

use crate::{NcAlphaBits, NcChannel, NcChannelPair, NcComponent, NcPaletteIndex, NcRgb};

/// Enables the [`NcChannel`] methods.
pub trait NcChannelMethods {
    // constructors
    fn new() -> Self;
    fn with_default() -> Self;
    fn from_rgb(rgb: NcRgb) -> Self;
    fn from_rgb_alpha(rgb: NcRgb, alpha: NcAlphaBits) -> Self;
    fn from_rgb8(r: NcComponent, g: NcComponent, b: NcComponent) -> Self;
    fn from_rgb8_alpha(r: NcComponent, g: NcComponent, b: NcComponent, alpha: NcAlphaBits) -> Self;

    // methods
    fn fcombine(&self, bchannel: NcChannel) -> NcChannelPair;
    fn bcombine(&self, fchannel: NcChannel) -> NcChannelPair;

    fn alpha(&self) -> NcAlphaBits;
    fn set_alpha(&mut self, alpha: NcAlphaBits) -> Self;

    fn set(&mut self, rgb: NcRgb) -> Self;

    fn rgb8(&self) -> (NcComponent, NcComponent, NcComponent);
    fn set_rgb8(&mut self, r: NcComponent, g: NcComponent, b: NcComponent) -> Self;
    fn r(&self) -> NcComponent;
    fn g(&self) -> NcComponent;
    fn b(&self) -> NcComponent;
    fn set_r(&mut self, r: NcComponent) -> Self;
    fn set_g(&mut self, g: NcComponent) -> Self;
    fn set_b(&mut self, b: NcComponent) -> Self;

    fn rgb(&self) -> NcRgb;
    fn set_rgb(&mut self, rgb: NcRgb) -> Self;

    fn default_p(&self) -> bool;
    fn set_default(&mut self) -> Self;
    fn set_not_default(&mut self) -> Self;

    fn palindex_p(&self) -> bool;
}

/// Enables the [`NcChannelPair`] methods.
pub trait NcChannelPairMethods {
    // constructors
    fn new() -> Self;
    fn with_default() -> Self;
    fn from_rgb(fg_rgb: NcRgb, bg_rgb: NcRgb) -> Self;
    fn from_rgb_both(rgb: NcRgb) -> Self;
    fn from_rgb_alpha(
        fg_rgb: NcRgb,
        fg_alpha: NcAlphaBits,
        bg_rgb: NcRgb,
        bg_alpha: NcAlphaBits,
    ) -> Self;
    fn from_rgb_alpha_both(rgb: NcRgb, alpha: NcAlphaBits) -> Self;
    fn from_rgb8(
        fg_r: NcComponent,
        fg_g: NcComponent,
        fg_b: NcComponent,
        bg_r: NcComponent,
        bg_g: NcComponent,
        bg_b: NcComponent,
    ) -> Self;
    fn from_rgb8_both(r: NcComponent, g: NcComponent, b: NcComponent) -> Self;
    fn from_rgb8_alpha(
        fg_r: NcComponent,
        fg_g: NcComponent,
        fg_b: NcComponent,
        fg_alpha: NcAlphaBits,
        bg_r: NcComponent,
        bg_g: NcComponent,
        bg_b: NcComponent,
        bg_alpha: NcAlphaBits,
    ) -> Self;
    fn from_rgb8_alpha_both(
        r: NcComponent,
        g: NcComponent,
        b: NcComponent,
        alpha: NcAlphaBits,
    ) -> Self;

    // methods
    fn combine(fchannel: NcChannel, bchannel: NcChannel) -> Self;

    fn fchannel(&self) -> NcChannel;
    fn bchannel(&self) -> NcChannel;
    fn set_fchannel(&mut self, fchannel: NcChannel) -> Self;
    fn set_bchannel(&mut self, bchannel: NcChannel) -> Self;

    fn fg_alpha(&self) -> NcAlphaBits;
    fn bg_alpha(&self) -> NcAlphaBits;
    fn set_fg_alpha(&mut self, alpha: NcAlphaBits);
    fn set_bg_alpha(&mut self, alpha: NcAlphaBits);

    fn fg_rgb(&self) -> NcRgb;
    fn bg_rgb(&self) -> NcRgb;
    fn set_fg_rgb(&mut self, alpha: NcAlphaBits) -> Self;
    fn set_bg_rgb(&mut self, alpha: NcAlphaBits) -> Self;

    fn fg_rgb8(&self) -> (NcComponent, NcComponent, NcComponent);
    fn bg_rgb8(&self) -> (NcComponent, NcComponent, NcComponent);
    fn set_fg_rgb8(&mut self, r: NcComponent, g: NcComponent, b: NcComponent) -> Self;
    fn set_bg_rgb8(&mut self, r: NcComponent, g: NcComponent, b: NcComponent) -> Self;
    fn fg_r(&self) -> NcComponent;
    fn fg_g(&self) -> NcComponent;
    fn fg_b(&self) -> NcComponent;
    fn bg_r(&self) -> NcComponent;
    fn bg_g(&self) -> NcComponent;
    fn bg_b(&self) -> NcComponent;
    fn fg_set_r(&mut self, r: NcComponent) -> Self;
    fn fg_set_g(&mut self, g: NcComponent) -> Self;
    fn fg_set_b(&mut self, b: NcComponent) -> Self;
    fn bg_set_r(&mut self, r: NcComponent) -> Self;
    fn bg_set_g(&mut self, g: NcComponent) -> Self;
    fn bg_set_b(&mut self, b: NcComponent) -> Self;

    fn fg_default_p(&self) -> bool;
    fn bg_default_p(&self) -> bool;
    fn set_fg_default(&mut self) -> Self;
    fn set_fg_not_default(&mut self) -> Self;
    fn set_bg_default(&mut self) -> Self;
    fn set_bg_not_default(&mut self) -> Self;
    fn set_default(&mut self) -> Self;
    fn set_not_default(&mut self) -> Self;

    fn fg_palindex_p(&self) -> bool;
    fn bg_palindex_p(&self) -> bool;
    fn set_fg_palindex(&mut self, index: NcPaletteIndex) -> Self;
    fn set_bg_palindex(&mut self, index: NcPaletteIndex) -> Self;
}

// NcChannel -------------------------------------------------------------------

/// # NcChannel Methods
impl NcChannelMethods for NcChannel {
    // Constructors

    /// New NcChannel, set to black and NOT using the "default color".
    fn new() -> Self {
        0 as NcChannel | crate::NCALPHA_BGDEFAULT_MASK
    }

    /// New NcChannel, set to black but using the "default color".
    fn with_default() -> Self {
        0 as NcChannel
    }

    /// New NcChannel, expects [`NcRgb`].
    fn from_rgb(rgb: NcRgb) -> Self {
        Self::new().set(rgb)
    }

    /// New NcChannel, expects [`NcRgb`] & [`NcAlphaBits`].
    fn from_rgb_alpha(rgb: NcRgb, alpha: NcAlphaBits) -> Self {
        Self::new().set(rgb).set_alpha(alpha)
    }

    /// New NcChannel, expects three RGB [`NcComponent`] components.
    fn from_rgb8(r: NcComponent, g: NcComponent, b: NcComponent) -> Self {
        Self::new().set_rgb8(r, g, b)
    }

    /// New NcChannel, expects three RGB [`NcComponent`] components & [`NcAlphaBits`].
    fn from_rgb8_alpha(r: NcComponent, g: NcComponent, b: NcComponent, alpha: NcAlphaBits) -> Self {
        Self::new().set_rgb8(r, g, b).set_alpha(alpha)
    }

    // Combine

    /// Combines this [`NcChannel`] as foreground, with another as background
    /// into an [`NcChannelPair`].
    ///
    /// *C style function: [channels_combine()][crate::ncchannels_combine].*
    //
    // Not in the C API
    fn fcombine(&self, bchannel: NcChannel) -> NcChannelPair {
        crate::ncchannels_combine(*self, bchannel)
    }

    /// Combines this [`NcChannel`] as background, with another as foreground
    /// into an [`NcChannelPair`].
    ///
    /// *C style function: [channels_combine()][crate::ncchannels_combine].*
    //
    // Not in the C API
    fn bcombine(&self, fchannel: NcChannel) -> NcChannelPair {
        crate::ncchannels_combine(fchannel, *self)
    }

    // Alpha

    /// Gets the [`NcAlphaBits`].
    ///
    /// *C style function: [channel_alpha()][crate::ncchannel_alpha].*
    fn alpha(&self) -> NcAlphaBits {
        crate::ncchannel_alpha(*self)
    }

    /// Sets the [`NcAlphaBits`].
    ///
    /// *C style function: [channel_set_alpha()][crate::ncchannel_set_alpha].*
    fn set_alpha(&mut self, alpha: NcAlphaBits) -> Self {
        crate::ncchannel_set_alpha(self, alpha);
        *self
    }

    // NcRgb

    /// Sets the [`NcRgb`], and marks the NcChannel as NOT using the
    /// "default color", retaining the other bits unchanged.
    ///
    /// *C style function: [channel_set()][crate::ncchannel_set].*
    fn set(&mut self, rgb: NcRgb) -> Self {
        crate::ncchannel_set(self, rgb);
        *self
    }

    // NcComponent

    /// Gets the three [`NcComponent`]s.
    ///
    /// *C style function: [channel_rgb8()][crate::ncchannel_rgb8].*
    fn rgb8(&self) -> (NcComponent, NcComponent, NcComponent) {
        let (mut r, mut g, mut b) = (0, 0, 0);
        crate::ncchannel_rgb8(*self, &mut r, &mut g, &mut b);
        (r, g, b)
    }

    /// Sets the three [`NcComponent`]s, and
    /// marks the NcChannel as NOT using the "default color".
    ///
    /// *C style function: [channel_set_rgb8()][crate::ncchannel_set_rgb8].*
    fn set_rgb8(&mut self, r: NcComponent, g: NcComponent, b: NcComponent) -> Self {
        crate::ncchannel_set_rgb8(self, r, g, b);
        *self
    }

    /// Gets the red [`NcComponent`].
    ///
    /// *C style function: [channel_r()][crate::ncchannel_r].*
    fn r(&self) -> NcComponent {
        crate::ncchannel_r(*self)
    }

    /// Gets the green [`NcComponent`].
    ///
    /// *C style function: [channel_g()][crate::ncchannel_g].*
    fn g(&self) -> NcComponent {
        crate::ncchannel_g(*self)
    }

    /// Gets the blue [`NcComponent`].
    ///
    /// *C style function: [channel_b()][crate::ncchannel_b].*
    fn b(&self) -> NcComponent {
        crate::ncchannel_b(*self)
    }

    /// Sets the red [`NcComponent`], and returns the new `NcChannel`.
    ///
    /// *C style function: [channel_set_r()][crate::ncchannel_set_r].*
    //
    // Not in the C API
    fn set_r(&mut self, r: NcComponent) -> Self {
        crate::ncchannel_set_r(self, r)
    }

    /// Sets the green [`NcComponent`], and returns the new `NcChannel`.
    ///
    /// *C style function: [channel_set_g()][crate::ncchannel_set_g].*
    //
    // Not in the C API
    fn set_g(&mut self, g: NcComponent) -> Self {
        crate::ncchannel_set_g(self, g)
    }

    /// Sets the blue [`NcComponent`], and returns the new `NcChannel`.
    ///
    /// *C style function: [channel_set_b()][crate::ncchannel_set_b].*
    //
    // Not in the C API
    fn set_b(&mut self, b: NcComponent) -> Self {
        crate::ncchannel_set_b(self, b)
    }

    // NcRgb

    /// Gets the [`NcRgb`].
    ///
    /// *C style function: [channel_rgb()][crate::ncchannel_rgb].*
    //
    // Not in the C API
    fn rgb(&self) -> NcRgb {
        crate::ncchannel_rgb(*self)
    }

    /// Sets the [`NcRgb`] and marks it as NOT using the "default color",
    /// retaining the other bits unchanged.
    ///
    /// *C style function: [channel_set()][crate::ncchannel_set].*
    fn set_rgb(&mut self, rgb: NcRgb) -> Self {
        crate::ncchannel_set(self, rgb);
        *self
    }

    // default color

    /// Is this NcChannel using the "default color" rather than RGB/palette-indexed?
    ///
    /// *C style function: [channel_default_p()][crate::ncchannel_default_p].*
    fn default_p(&self) -> bool {
        crate::ncchannel_default_p(*self)
    }

    /// Marks an NcChannel as using its "default color", which also marks it opaque.
    ///
    /// *C style function: [channel_set_default()][crate::ncchannel_set_default].*
    fn set_default(&mut self) -> Self {
        crate::ncchannel_set_default(self)
    }

    /// Marks an NcChannel as *not* using its "default color".
    ///
    /// The following methods also marks the channel as not using the "default color":
    /// - [new()][NcChannel#method.new]
    /// - [set()][NcChannel#method.set]
    /// - [set_rgb()][NcChannel#method.set_rgb]
    /// - [set_rgb8()][NcChannel#method.set_rgb8]
    ///
    /// *C style function: [channel_set_not_default()][crate::ncchannel_set_not_default].*
    //
    // Not in the C API
    fn set_not_default(&mut self) -> Self {
        crate::ncchannel_set_not_default(self)
    }

    // NcPaletteIndex

    /// Is this NcChannel using palette-indexed color rather than RGB?
    ///
    /// *C style function: [channel_set_default()][crate::ncchannel_set_default].*
    fn palindex_p(&self) -> bool {
        crate::ncchannel_palindex_p(*self)
    }
}

// NcChannelPair ---------------------------------------------------------------

/// # NcChannelPair Methods
impl NcChannelPairMethods for NcChannelPair {
    // Constructors

    /// New NcChannelPair, set to black and NOT using the "default color".
    fn new() -> Self {
        Self::combine(
            0 as NcChannel | crate::NCALPHA_BGDEFAULT_MASK,
            0 as NcChannel | crate::NCALPHA_BGDEFAULT_MASK,
        )
    }

    /// New NcChannelPair, set to black but using the "default color".
    fn with_default() -> Self {
        Self::combine(0 as NcChannel, 0 as NcChannel)
    }

    /// New NcChannel, expects two separate [`NcRgb`]s for the foreground
    /// and background channels.
    fn from_rgb(fg_rgb: NcRgb, bg_rgb: NcRgb) -> Self {
        Self::combine(NcChannel::from_rgb(fg_rgb), NcChannel::from_rgb(bg_rgb))
    }

    /// New NcChannelPair, expects a single [`NcRgb`] for both foreground
    /// and background channels.
    fn from_rgb_both(rgb: NcRgb) -> Self {
        let channel = NcChannel::new().set(rgb);
        Self::combine(channel, channel)
    }

    /// New NcChannel, expects two separate [`NcRgb`] & [`NcAlphaBits`] for the
    /// foreground and background channels.
    fn from_rgb_alpha(
        fg_rgb: NcRgb,
        fg_alpha: NcAlphaBits,
        bg_rgb: NcRgb,
        bg_alpha: NcAlphaBits,
    ) -> Self {
        Self::combine(
            NcChannel::from_rgb(fg_rgb).set_alpha(fg_alpha),
            NcChannel::from_rgb(bg_rgb).set_alpha(bg_alpha),
        )
    }

    /// New NcChannel, expects [`NcRgb`] & [`NcAlphaBits`] for both channels.
    fn from_rgb_alpha_both(rgb: NcRgb, alpha: NcAlphaBits) -> Self {
        let channel = NcChannel::new().set(rgb).set_alpha(alpha);
        Self::combine(channel, channel)
    }

    /// New NcChannelPair, expects three RGB [`NcComponent`] components for each channel.
    fn from_rgb8(
        fg_r: NcComponent,
        fg_g: NcComponent,
        fg_b: NcComponent,
        bg_r: NcComponent,
        bg_g: NcComponent,
        bg_b: NcComponent,
    ) -> Self {
        Self::combine(
            NcChannel::from_rgb8(fg_r, fg_g, fg_b),
            NcChannel::from_rgb8(bg_r, bg_g, bg_b),
        )
    }

    /// New NcChannelPair, expects three RGB [`NcComponent`] components for both
    /// the foreground and background channels.
    fn from_rgb8_both(r: NcComponent, g: NcComponent, b: NcComponent) -> Self {
        let channel = NcChannel::new().set_rgb8(r, g, b);
        Self::combine(channel, channel)
    }

    /// New NcChannelPair, expects three RGB [`NcComponent`] components & [`NcAlphaBits`]
    /// for both foreground and background channels.
    fn from_rgb8_alpha(
        fg_r: NcComponent,
        fg_g: NcComponent,
        fg_b: NcComponent,
        fg_alpha: NcAlphaBits,
        bg_r: NcComponent,
        bg_g: NcComponent,
        bg_b: NcComponent,
        bg_alpha: NcAlphaBits,
    ) -> Self {
        Self::combine(
            NcChannel::from_rgb8_alpha(fg_r, fg_g, fg_b, fg_alpha),
            NcChannel::from_rgb8_alpha(bg_r, bg_g, bg_b, bg_alpha),
        )
    }

    /// New NcChannel, expects three RGB [`NcComponent`] components.
    fn from_rgb8_alpha_both(
        r: NcComponent,
        g: NcComponent,
        b: NcComponent,
        alpha: NcAlphaBits,
    ) -> Self {
        let channel = NcChannel::new().set_rgb8(r, g, b).set_alpha(alpha);
        Self::combine(channel, channel)
    }

    // Combine

    /// Combines two [`NcChannel`]s into an [`NcChannelPair`].
    ///
    /// *C style function: [channels_combine()][crate::ncchannels_combine].*
    fn combine(fchannel: NcChannel, bchannel: NcChannel) -> Self {
        crate::ncchannels_combine(fchannel, bchannel)
    }

    // NcChannel

    /// Extracts the foreground [`NcChannel`].
    ///
    /// *C style function: [channels_fchannel()][crate::ncchannels_fchannel].*
    fn fchannel(&self) -> NcChannel {
        crate::ncchannels_fchannel(*self)
    }

    /// Extracts the background [`NcChannel`].
    ///
    /// *C style function: [channels_bchannel()][crate::ncchannels_bchannel].*
    fn bchannel(&self) -> NcChannel {
        crate::ncchannels_bchannel(*self)
    }

    /// Sets the foreground [`NcChannel`].
    ///
    /// *C style function: [channels_set_fchannel()][crate::ncchannels_set_fchannel].*
    fn set_fchannel(&mut self, fchannel: NcChannel) -> Self {
        crate::ncchannels_set_fchannel(self, fchannel)
    }

    /// Sets the background [`NcChannel`].
    ///
    /// *C style function: [channels_set_bchannel()][crate::ncchannels_set_bchannel].*
    fn set_bchannel(&mut self, bchannel: NcChannel) -> Self {
        crate::ncchannels_set_bchannel(self, bchannel)
    }

    // Alpha

    /// Gets the foreground [`NcAlphaBits`].
    ///
    /// *C style function: [channels_fg_alpha()][crate::ncchannels_fg_alpha].*
    fn fg_alpha(&self) -> NcAlphaBits {
        crate::ncchannels_fg_alpha(*self)
    }

    /// Gets the background [`NcAlphaBits`].
    ///
    /// *C style function: [channels_bg_alpha()][crate::ncchannels_bg_alpha].*
    fn bg_alpha(&self) -> NcAlphaBits {
        crate::ncchannels_bg_alpha(*self)
    }

    /// Sets the foreground [`NcAlphaBits`].
    ///
    /// *C style function: [channels_set_fg_alpha()][crate::ncchannels_set_fg_alpha].*
    fn set_fg_alpha(&mut self, alpha: NcAlphaBits) {
        crate::ncchannels_set_fg_alpha(self, alpha)
    }

    /// Sets the background [`NcAlphaBits`].
    ///
    /// *C style function: [channels_set_bg_alpha()][crate::ncchannels_set_bg_alpha].*
    fn set_bg_alpha(&mut self, alpha: NcAlphaBits) {
        crate::ncchannels_set_bg_alpha(self, alpha)
    }

    // NcRgb

    /// Gets the foreground [`NcRgb`].
    ///
    /// *C style function: [channels_fg_rgb()][crate::ncchannels_fg_rgb].*
    fn fg_rgb(&self) -> NcRgb {
        crate::ncchannels_fg_rgb(*self)
    }

    /// Gets the background [`NcRgb`].
    ///
    /// *C style function: [channels_bg_rgb()][crate::ncchannels_bg_rgb].*
    fn bg_rgb(&self) -> NcRgb {
        crate::ncchannels_bg_rgb(*self)
    }

    /// Sets the foreground [`NcRgb`].
    ///
    /// *C style function: [channels_set_fg_rgb()][crate::ncchannels_set_fg_rgb].*
    fn set_fg_rgb(&mut self, rgb: NcRgb) -> Self {
        crate::ncchannels_set_fg_rgb(self, rgb);
        *self
    }

    /// Sets the background [`NcRgb`].
    ///
    /// *C style function: [channels_set_bg_rgb()][crate::ncchannels_set_bg_rgb].*
    fn set_bg_rgb(&mut self, rgb: NcRgb) -> Self {
        crate::ncchannels_set_bg_rgb(self, rgb);
        *self
    }

    // NcComponent

    /// Gets the three foreground [`NcComponent`]s (r, g, b).
    ///
    /// *C style function: [channels_fg_rgb8()][crate::ncchannels_fg_rgb8].*
    fn fg_rgb8(&self) -> (NcComponent, NcComponent, NcComponent) {
        let (mut r, mut g, mut b) = (0, 0, 0);
        crate::ncchannels_fg_rgb8(*self, &mut r, &mut g, &mut b);
        (r, g, b)
    }

    /// Gets the three background [`NcComponent`]s (r, g, b).
    ///
    /// *C style function: [channels_bg_rgb8()][crate::ncchannels_bg_rgb8].*
    fn bg_rgb8(&self) -> (NcComponent, NcComponent, NcComponent) {
        let (mut r, mut g, mut b) = (0, 0, 0);
        crate::ncchannels_bg_rgb8(*self, &mut r, &mut g, &mut b);
        (r, g, b)
    }

    /// Sets the three foreground [`NcComponent`]s (r, g, b), and
    /// marks the foreground [`NcChannel`] as not using the "default color".
    ///
    /// *C style function: [channels_set_fg_rgb8()][crate::ncchannels_set_fg_rgb8].*
    fn set_fg_rgb8(&mut self, r: NcComponent, g: NcComponent, b: NcComponent) -> Self {
        crate::ncchannels_set_fg_rgb8(self, r, g, b)
    }

    /// Sets the three background [`NcComponent`]s (r, g, b), and
    /// marks the background [`NcChannel`] as not using the "default color".
    ///
    /// *C style function: [channels_set_bg_rgb8()][crate::ncchannels_set_bg_rgb8].*
    fn set_bg_rgb8(&mut self, r: NcComponent, g: NcComponent, b: NcComponent) -> Self {
        crate::ncchannels_set_bg_rgb8(self, r, g, b)
    }

    /// Gets the foreground red [`NcComponent`].
    ///
    /// *(No equivalent C style function)*
    fn fg_r(&self) -> NcComponent {
        crate::ncchannel_r(crate::ncchannels_fchannel(*self))
    }

    /// Gets the foreground green [`NcComponent`].
    ///
    /// *(No equivalent C style function)*
    fn fg_g(&self) -> NcComponent {
        crate::ncchannel_g(crate::ncchannels_fchannel(*self))
    }

    /// Gets the foreground blue [`NcComponent`].
    ///
    /// *(No equivalent C style function)*
    fn fg_b(&self) -> NcComponent {
        crate::ncchannel_b(crate::ncchannels_fchannel(*self))
    }

    /// Gets the background red [`NcComponent`].
    ///
    /// *(No equivalent C style function)*
    fn bg_r(&self) -> NcComponent {
        crate::ncchannel_r(crate::ncchannels_bchannel(*self))
    }

    /// Gets the background green [`NcComponent`].
    ///
    /// *(No equivalent C style function)*
    fn bg_g(&self) -> NcComponent {
        crate::ncchannel_g(crate::ncchannels_bchannel(*self))
    }

    /// Gets the background blue [`NcComponent`].
    ///
    /// *(No equivalent C style function)*
    fn bg_b(&self) -> NcComponent {
        crate::ncchannel_b(crate::ncchannels_bchannel(*self))
    }

    /// Sets the foreground red [`NcComponent`], and returns the new `NcChannelPair`.
    ///
    /// *(No equivalent C style function)*
    fn fg_set_r(&mut self, r: NcComponent) -> Self {
        let (_, g, b) = self.bg_rgb8();
        crate::ncchannels_set_fg_rgb8(self, r, g, b)
    }

    /// Sets the foreground green [`NcComponent`], and returns the new `NcChannelPair`.
    ///
    /// *(No equivalent C style function)*
    fn fg_set_g(&mut self, g: NcComponent) -> Self {
        let (r, _, b) = self.bg_rgb8();
        crate::ncchannels_set_fg_rgb8(self, r, g, b)
    }

    /// Sets the foreground blue [`NcComponent`], and returns the new `NcChannelPair`.
    ///
    /// *(No equivalent C style function)*
    fn fg_set_b(&mut self, b: NcComponent) -> Self {
        let (r, g, _) = self.bg_rgb8();
        crate::ncchannels_set_fg_rgb8(self, r, g, b)
    }

    /// Sets the background red [`NcComponent`], and returns the new `NcChannelPair`.
    ///
    /// *(No equivalent C style function)*
    fn bg_set_r(&mut self, r: NcComponent) -> Self {
        let (_, g, b) = self.bg_rgb8();
        crate::ncchannels_set_bg_rgb8(self, r, g, b)
    }

    /// Sets the background green [`NcComponent`], and returns the new `NcChannelPair`.
    ///
    /// *(No equivalent C style function)*
    fn bg_set_g(&mut self, g: NcComponent) -> Self {
        let (r, _, b) = self.bg_rgb8();
        crate::ncchannels_set_bg_rgb8(self, r, g, b)
    }

    /// Sets the background blue [`NcComponent`], and returns the new `NcChannelPair`.
    ///
    /// *(No equivalent C style function)*
    fn bg_set_b(&mut self, b: NcComponent) -> Self {
        let (r, g, _) = self.bg_rgb8();
        crate::ncchannels_set_bg_rgb8(self, r, g, b)
    }

    // default color

    /// Is the background using the "default background color"?
    ///
    /// *C style function: [channels_fg_default_p()][crate::ncchannels_fg_default_p].*
    fn fg_default_p(&self) -> bool {
        crate::ncchannels_fg_default_p(*self)
    }

    /// Is the background using the "default background color"?
    ///
    /// The "default background color" must generally be used to take advantage
    /// of terminal-effected transparency.
    ///
    /// *C style function: [channels_bg_default_p()][crate::ncchannels_bg_default_p].*
    fn bg_default_p(&self) -> bool {
        crate::ncchannels_bg_default_p(*self)
    }

    /// Marks the foreground as using its "default color", and
    /// returns the new [`NcChannelPair`].
    ///
    /// *C style function: [channels_set_fg_default()][crate::ncchannels_set_fg_default].*
    fn set_fg_default(&mut self) -> Self {
        crate::ncchannels_set_fg_default(self)
    }

    /// Marks the background as using its "default color", and
    /// returns the new [`NcChannelPair`].
    ///
    /// *C style function: [channels_set_bg_default()][crate::ncchannels_set_bg_default].*
    fn set_bg_default(&mut self) -> Self {
        crate::ncchannels_set_bg_default(self)
    }

    /// Marks the foreground as NOT using its "default color", and
    /// returns the new [`NcChannelPair`].
    ///
    /// *C style function: [channels_set_fg_default()][crate::ncchannels_set_fg_default].*
    //
    // Not in the C API
    fn set_fg_not_default(&mut self) -> Self {
        crate::ncchannels_set_fg_not_default(self)
    }

    /// Marks the background as NOT using its "default color", and
    /// returns the new [`NcChannelPair`].
    ///
    /// *C style function: [channels_set_bg_not_default()][crate::ncchannels_set_bg_not_default].*
    //
    // Not in the C API
    fn set_bg_not_default(&mut self) -> Self {
        crate::ncchannels_set_bg_not_default(self)
    }

    /// Marks both the foreground and background as using its "default color", and
    /// returns the new [`NcChannelPair`].
    ///
    //
    // Not in the C API
    fn set_default(&mut self) -> Self {
        crate::ncchannels_set_fg_default(&mut crate::ncchannels_set_bg_default(self))
    }

    /// Marks both the foreground and background as NOT using its "default color",
    /// and returns the new [`NcChannelPair`].
    ///
    //
    // Not in the C API
    fn set_not_default(&mut self) -> Self {
        crate::ncchannels_set_fg_not_default(&mut crate::ncchannels_set_bg_not_default(self))
    }

    // NcPaletteIndex

    /// Is the foreground of using an [indexed][NcPaletteIndex]
    /// [NcPalette][crate::NcPalette] color?
    ///
    /// *C style function: [channels_fg_palindex_p()][crate::ncchannels_fg_palindex_p].*
    fn fg_palindex_p(&self) -> bool {
        crate::ncchannels_fg_palindex_p(*self)
    }

    /// Is the background of using an [indexed][NcPaletteIndex]
    /// [NcPalette][crate::NcPalette] color?
    ///
    /// *C style function: [channels_bg_palindex_p()][crate::ncchannels_bg_palindex_p].*
    fn bg_palindex_p(&self) -> bool {
        crate::ncchannels_bg_palindex_p(*self)
    }

    /// Sets the foreground of an [`NcChannelPair`] as using an
    /// [indexed][NcPaletteIndex] [NcPalette][crate::NcPalette] color.
    ///
    /// *C style function: [channels_set_fg_palindex()][crate::ncchannels_set_fg_palindex].*
    fn set_fg_palindex(&mut self, index: NcPaletteIndex) -> Self {
        crate::ncchannels_set_fg_palindex(self, index);
        *self
    }

    /// Sets the background of an [`NcChannelPair`] as using an
    /// [indexed][NcPaletteIndex] [NcPalette][crate::NcPalette] color.
    ///
    /// *C style function: [channels_set_bg_palindex()][crate::ncchannels_set_bg_palindex].*
    fn set_bg_palindex(&mut self, index: NcPaletteIndex) -> Self {
        crate::ncchannels_set_bg_palindex(self, index);
        *self
    }
}
