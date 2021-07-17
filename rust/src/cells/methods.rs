//! `NcCell` methods and associated functions.

use crate::{
    cstring, error, nccell_load, NcAlphaBits, NcCell, NcChannels, NcComponent, NcEgcBackstop,
    NcPaletteIndex, NcPlane, NcResult, NcRgb, NcStyle, NCRESULT_ERR,
};

#[allow(unused_imports)] // for the doc comments
use crate::NcChannel;

/// # NcCell constructors
impl NcCell {
    /// New `NcCell`, expects a 7-bit [`char`].
    #[inline]
    #[allow(clippy::unnecessary_cast)]
    pub const fn from_char7b(ch: char) -> Self {
        NcCell {
            gcluster: (ch as u32).to_le(),
            gcluster_backstop: 0 as NcEgcBackstop,
            width: 0_u8,
            stylemask: 0 as NcStyle,
            channels: 0 as NcChannels,
        }
    }

    /// New `NcCell`, expects an [`NcPlane`] and a [`char`].
    #[inline]
    pub fn from_char(ch: char, plane: &mut NcPlane) -> Self {
        let mut cell = Self::new();
        let result = unsafe { nccell_load(plane, &mut cell, cstring![ch.to_string()]) };
        // TEMP solution for:
        // https://github.com/dankamongmen/notcurses/pull/1937/checks?check_run_id=3093152924#step:11:339
        #[cfg(not(target_os = "macos"))]
        debug_assert_ne![NCRESULT_ERR, result];
        cell
    }

    /// New `NcCell`, expects an [`NcPlane`] and a &[`str`].
    #[inline]
    pub fn from_str(plane: &mut NcPlane, string: &str) -> Self {
        let mut cell = Self::new();
        let result = unsafe { nccell_load(plane, &mut cell, cstring![string]) };
        debug_assert_ne![NCRESULT_ERR, result];
        cell
    }

    /// New empty `NcCell`.
    #[inline]
    pub const fn new() -> Self {
        Self::from_char7b(0 as char)
    }

    /// Breaks the UTF-8 string in `egc` down, setting up this `NcCell`,
    /// and returns the number of bytes copied out of `egc`.
    ///
    /// The styling of the cell is left untouched, but any resources are released.
    /// *C style function: [nccell_load()][crate::nccell_load].*
    pub fn load(plane: &mut NcPlane, cell: &mut NcCell, egc: &str) -> NcResult<u32> {
        let bytes = unsafe { crate::nccell_load(plane, cell, cstring![egc]) };
        error![
            bytes,
            &format!["NcCell.load(NcPlane, NcCell, {:?})", egc],
            bytes as u32
        ]
    }

    /// Same as [load][NcCell#method.load], plus blasts the styling with
    /// `style` and `channels`.
    ///
    /// - Breaks the UTF-8 string in `gcluster` down, setting up this NcCell.
    /// - Returns the number of bytes copied out of `gcluster`.
    /// - Any resources are released.
    /// - Blasts the styling with `style` and `channels`.
    ///
    /// *C style function: [nccell_prime()][crate::nccell_prime].*
    pub fn prime(
        plane: &mut NcPlane,
        cell: &mut NcCell,
        gcluster: &str,
        style: NcStyle,
        channels: NcChannels,
    ) -> NcResult<u32> {
        let bytes = crate::nccell_prime(plane, cell, gcluster, style, channels);
        error![bytes, "", bytes as u32]
    }

    /// Duplicate this `NcCell` into another one.
    ///
    /// Both must be or will be bound to `common_plane`.
    ///
    /// *C style function: [nccell_duplicate()][crate::nccell_duplicate].*
    pub fn duplicate(&self, target: &mut NcCell, common_plane: &mut NcPlane) -> NcResult<()> {
        error![unsafe { crate::nccell_duplicate(common_plane, target, self) }]
    }

    /// Initializes (zeroes out) this `NcCell`.
    ///
    /// *C style function: [nccell_init()][crate::nccell_init].*
    #[inline]
    pub fn init(&mut self) {
        crate::nccell_init(self);
    }

    /// Releases resources held by the current cell in the [NcPlane] `plane`.
    ///
    /// *C style function: [nccell_release()][crate::nccell_release].*
    pub fn release(&mut self, plane: &mut NcPlane) {
        unsafe {
            crate::nccell_release(plane, self);
        }
    }
}

// -----------------------------------------------------------------------------
/// ## NcCell methods: bg|fg `NcChannel`s manipulation.
impl NcCell {
    /// Returns the [`NcChannels`] of this `NcCell`.
    ///
    /// *(No equivalent C style function)*
    pub fn channels(&mut self, plane: &mut NcPlane) -> NcChannels {
        let (mut _styles, mut channels) = (0, 0);
        let _char = crate::nccell_extract(plane, self, &mut _styles, &mut channels);
        channels
    }

    /// Extracts the background [`NcAlphaBits`] (shifted to LSBs).
    ///
    /// *C style function: [nccell_bg_alpha()][crate::nccell_bg_alpha].*
    pub fn bg_alpha(&self) -> NcAlphaBits {
        crate::nccell_bg_alpha(self)
    }

    /// Is the background [`NcChannel`] using the "default background color"?
    ///
    /// *C style function: [nccell_bg_default_p()][crate::nccell_bg_default_p].*
    pub fn bg_default_p(&self) -> bool {
        crate::nccell_bg_default_p(self)
    }

    /// Gets the [`NcPaletteIndex`] of the background [`NcChannel`].
    ///
    /// *C style function: [nccell_bg_palindex()][crate::nccell_bg_palindex].*
    pub fn bg_palindex(&self) -> NcPaletteIndex {
        crate::nccell_bg_palindex(self)
    }

    /// Is the background [`NcChannel`] using an [`NcPaletteIndex`] indexed
    /// [`NcPalette`][crate::NcPalette] color?
    ///
    /// *C style function: [nccell_bg_palindex_p()][crate::nccell_bg_palindex_p].*
    pub fn bg_palindex_p(&self) -> bool {
        crate::nccell_bg_palindex_p(self)
    }

    /// Gets the background [`NcRgb`] (shifted to LSBs).
    ///
    /// *C style function: [nccell_bg_rgb()][crate::nccell_bg_rgb].*
    pub fn bg_rgb(&self) -> NcRgb {
        crate::nccell_bg_rgb(self)
    }

    /// Gets the background RGB [`NcComponent`]s.
    ///
    /// *C style function: [nccell_bg_rgb8()][crate::nccell_bg_rgb8].*
    pub fn bg_rgb8(&self) -> (NcComponent, NcComponent, NcComponent) {
        let (mut r, mut g, mut b) = (0, 0, 0);
        crate::nccell_bg_rgb8(self, &mut r, &mut g, &mut b);
        (r, g, b)
    }

    /// Extracts the foreground [`NcAlphaBits`] (shifted to LSBs).
    ///
    /// *C style function: [nccell_fg_alpha()][crate::nccell_fg_alpha].*
    pub fn fg_alpha(&self) -> NcAlphaBits {
        crate::nccell_fg_alpha(self)
    }

    /// Is the foreground [`NcChannel`] using the "default foreground color"?
    ///
    /// *C style function: [nccell_fg_default_p()][crate::nccell_fg_default_p].*
    pub fn fg_default_p(&self) -> bool {
        crate::nccell_fg_default_p(self)
    }

    /// Gets the [`NcPaletteIndex`] of the foreground [`NcChannel`].
    ///
    /// *C style function: [nccell_fg_palindex()][crate::nccell_fg_palindex].*
    pub fn fg_palindex(&self) -> NcPaletteIndex {
        crate::nccell_fg_palindex(self)
    }

    /// Is the foreground [`NcChannel`] using an [`NcPaletteIndex`] indexed
    /// [`NcPalette`][crate::NcPalette] color?
    ///
    /// *C style function: [nccell_fg_palindex_p()][crate::nccell_fg_palindex_p].*
    pub fn fg_palindex_p(&self) -> bool {
        crate::nccell_fg_palindex_p(self)
    }

    /// Gets the foreground [`NcRgb`] (shifted to LSBs).
    ///
    /// *C style function: [nccell_fg_rgb()][crate::nccell_fg_rgb].*
    pub fn fg_rgb(&self) -> NcRgb {
        crate::nccell_fg_rgb(self)
    }

    /// Gets the foreground RGB [`NcComponent`]s.
    ///
    /// *C style function: [nccell_fg_rgb8()][crate::nccell_fg_rgb8].*
    pub fn fg_rgb8(&self) -> (NcComponent, NcComponent, NcComponent) {
        let (mut r, mut g, mut b) = (0, 0, 0);
        crate::nccell_fg_rgb8(self, &mut r, &mut g, &mut b);
        (r, g, b)
    }

    /// Sets the background [`NcAlphaBits`].
    ///
    /// *C style function: [nccell_set_bg_alpha()][crate::nccell_set_bg_alpha].*
    pub fn set_bg_alpha(&mut self, alpha: NcAlphaBits) {
        crate::nccell_set_bg_alpha(self, alpha);
    }

    /// Indicates to use the "default color" for the background [`NcChannel`].
    ///
    /// *C style function: [nccell_set_bg_default()][crate::nccell_set_bg_default].*
    pub fn set_bg_default(&mut self) {
        crate::nccell_set_bg_default(self);
    }

    /// Sets the background [`NcPaletteIndex`].
    ///
    /// Also sets [NCALPHA_BG_PALETTE][crate::NCALPHA_BG_PALETTE] and
    /// [NCALPHA_OPAQUE][crate::NCALPHA_OPAQUE], and clears out
    /// [NCALPHA_BGDEFAULT_MASK][crate::NCALPHA_BGDEFAULT_MASK].
    ///
    /// *C style function: [nccell_set_bg_palindex()][crate::nccell_set_bg_palindex].*
    pub fn set_bg_palindex(&mut self, index: NcPaletteIndex) {
        crate::nccell_set_bg_palindex(self, index);
    }

    /// Sets the background [`NcRgb`] and marks it as not using the default color.
    ///
    /// *C style function: [nccell_set_bg_rgb()][crate::nccell_set_bg_rgb].*
    pub fn set_bg_rgb(&mut self, rgb: NcRgb) {
        crate::nccell_set_bg_rgb(self, rgb);
    }

    /// Sets the background RGB [`NcComponent`]s, and marks it as not using
    /// the "default color".
    ///
    /// *C style function: [nccell_set_bg_rgb8()][crate::nccell_set_bg_rgb8].*
    pub fn set_bg_rgb8(&mut self, red: NcComponent, green: NcComponent, blue: NcComponent) {
        crate::nccell_set_bg_rgb8(self, red, green, blue);
    }

    /// Sets the foreground [`NcAlphaBits`].
    ///
    /// *C style function: [nccell_set_fg_alpha()][crate::nccell_set_fg_alpha].*
    pub fn set_fg_alpha(&mut self, alpha: NcAlphaBits) {
        crate::nccell_set_fg_alpha(self, alpha);
    }

    /// Indicates to use the "default color" for the foreground [`NcChannel`].
    ///
    /// *C style function: [nccell_set_fg_default()][crate::nccell_set_fg_default].*
    pub fn set_fg_default(&mut self) {
        crate::nccell_set_fg_default(self);
    }

    /// Sets the foreground [`NcPaletteIndex`].
    ///
    /// Also sets [NCALPHA_FG_PALETTE][crate::NCALPHA_FG_PALETTE] and
    /// [NCALPHA_OPAQUE][crate::NCALPHA_OPAQUE], and clears out
    /// [NCALPHA_BGDEFAULT_MASK][crate::NCALPHA_BGDEFAULT_MASK].
    ///
    /// *C style function: [nccell_set_fg_palindex()][crate::nccell_set_fg_palindex].*
    pub fn set_fg_palindex(&mut self, index: NcPaletteIndex) {
        crate::nccell_set_fg_palindex(self, index);
    }

    /// Sets the foreground [`NcRgb`] and marks it as not using the default color.
    ///
    /// *C style function: [nccell_set_fg_rgb()][crate::nccell_set_fg_rgb].*
    pub fn set_fg_rgb(&mut self, rgb: NcRgb) {
        crate::nccell_set_fg_rgb(self, rgb);
    }

    /// Sets the foreground RGB [`NcComponent`]s, and marks it as not using
    /// the "default color".
    ///
    /// *C style function: [nccell_set_fg_rgb8()][crate::nccell_set_fg_rgb8].*
    pub fn set_fg_rgb8(&mut self, red: NcComponent, green: NcComponent, blue: NcComponent) {
        crate::nccell_set_fg_rgb8(self, red, green, blue);
    }
}

/// # `NcCell` methods: other components
impl NcCell {
    /// Returns true if the two cells have distinct `EGC`s, attributes,
    /// or [`NcChannel`]s.
    ///
    /// The actual egcpool index needn't be the same--indeed, the planes
    /// needn't even be the same. Only the expanded `EGC` must be bit-equal.
    ///
    /// *C style function: [nccellcmp()][crate::nccellcmp].*
    pub fn compare(plane1: &NcPlane, cell1: &NcCell, plane2: &NcPlane, cell2: &NcCell) -> bool {
        crate::nccellcmp(plane1, cell1, plane2, cell2)
    }

    /// Saves the [`NcStyle`] and the [`NcChannels`], and returns the `EGC`.
    /// (These are the three elements of an `NcCell`).
    ///
    /// *C style function: [nccell_fg_alpha()][crate::nccell_fg_alpha].*
    pub fn extract(
        &mut self,
        plane: &mut NcPlane,
        styles: &mut NcStyle,
        channels: &mut NcChannels,
    ) -> String {
        crate::nccell_extract(plane, self, styles, channels)
    }

    /// Returns the `EGC` of the `NcCell`.
    ///
    /// See also: [extended_gcluster][NcCell#method.extended_gcluster] method.
    ///
    /// *(No equivalent C style function)*
    pub fn egc(&mut self, plane: &mut NcPlane) -> String {
        let (mut _styles, mut _channels) = (0, 0);
        crate::nccell_extract(plane, self, &mut _styles, &mut _channels)
    }

    /// Returns the [`NcStyle`] bits.
    ///
    /// *C style function: [nccell_styles()][crate::nccell_styles].*
    pub fn styles(&mut self) -> NcStyle {
        crate::nccell_styles(self)
    }

    /// Removes the specified [`NcStyle`] bits.
    ///
    /// *C style function: [nccell_off_styles()][crate::nccell_off_styles].*
    pub fn styles_off(&mut self, stylebits: NcStyle) {
        crate::nccell_off_styles(self, stylebits)
    }

    /// Adds the specified [`NcStyle`] bits.
    ///
    /// *C style function: [nccell_on_styles()][crate::nccell_on_styles].*
    pub fn styles_on(&mut self, stylebits: NcStyle) {
        crate::nccell_on_styles(self, stylebits)
    }

    /// Sets just the specified [`NcStyle`] bits.
    ///
    /// *C style function: [nccell_set_styles()][crate::nccell_set_styles].*
    pub fn styles_set(&mut self, stylebits: NcStyle) {
        crate::nccell_set_styles(self, stylebits)
    }
}

/// # `NcCell` methods: text
impl NcCell {
    // /// Returns a pointer to the `EGC` of this NcCell in the [NcPlane] `plane`.
    // ///
    // /// This pointer can be invalidated by any further operation on the referred
    // /// plane, so… watch out!
    // ///
    // /// *C style function: [nccell_extended_gcluster()][crate::nccell_wide_left_p].*
    // pub fn extended_gcluster(&self, plane: &NcPlane) -> u32 {
    //     let egcpointer = unsafe { crate::nccell_extended_gcluster(plane, self) };
    //     egcpointer
    // }

    /// Copies the UTF8-encoded `EGC` out of this NcCell,
    /// whether simple or complex.
    ///
    /// The result is not tied to the [NcPlane],
    /// and persists across erases and destruction.
    ///
    /// *C style function: [nccell_strdup()][crate::nccell_strdup].*
    pub fn strdup(&self, plane: &NcPlane) -> String {
        crate::nccell_strdup(plane, self)
    }

    /// Does this NcCell contain a wide codepoint?
    ///
    /// *C style function: [nccell_double_wide_p()][crate::nccell_double_wide_p].*
    pub fn double_wide_p(&self) -> bool {
        crate::nccell_double_wide_p(self)
    }

    /// Is this the left half of a wide character?
    ///
    /// *C style function: [nccell_wide_left_p()][crate::nccell_wide_left_p].*
    pub fn wide_left_p(&self) -> bool {
        crate::nccell_wide_right_p(self)
    }

    /// Is this the right side of a wide character?
    ///
    /// *C style function: [nccell_wide_right_p()][crate::nccell_wide_right_p].*
    pub fn wide_right_p(&self) -> bool {
        crate::nccell_wide_right_p(self)
    }
}

/// # `NcCell` methods: boxes
impl NcCell {
    /// Loads up six cells with the `EGC`s necessary to draw a box.
    ///
    /// On error, any [`NcCell`]s this function might have loaded before the error
    /// are [release][NcCell#method.release]d.
    /// There must be at least six `EGC`s in `gcluster`.
    ///
    /// *C style function: [nccells_load_box()][crate::nccells_load_box].*
    pub fn load_box(
        plane: &mut NcPlane,
        style: NcStyle,
        channels: NcChannels,
        ul: &mut NcCell,
        ur: &mut NcCell,
        ll: &mut NcCell,
        lr: &mut NcCell,
        hl: &mut NcCell,
        vl: &mut NcCell,
        gcluster: &str,
    ) -> NcResult<()> {
        error![crate::nccells_load_box(
            plane, style, channels, ul, ur, ll, lr, hl, vl, gcluster
        )]
    }

    /// NcCell.[load_box()][NcCell#method.box] with the double box-drawing characters.
    ///
    /// *C style function: [nccells_double_box()][crate::nccells_double_box].*
    pub fn double_box(
        plane: &mut NcPlane,
        style: NcStyle,
        channels: NcChannels,
        ul: &mut NcCell,
        ur: &mut NcCell,
        ll: &mut NcCell,
        lr: &mut NcCell,
        hl: &mut NcCell,
        vl: &mut NcCell,
    ) -> NcResult<()> {
        error![unsafe {
            crate::nccells_double_box(plane, style as u32, channels, ul, ur, ll, lr, hl, vl)
        }]
    }

    /// NcCell.[load_box()][NcCell#method.box] with the rounded box-drawing characters.
    ///
    /// *C style function: [nccells_rounded_box()][crate::nccells_double_box].*
    pub fn rounded_box(
        plane: &mut NcPlane,
        style: NcStyle,
        channels: NcChannels,
        ul: &mut NcCell,
        ur: &mut NcCell,
        ll: &mut NcCell,
        lr: &mut NcCell,
        hl: &mut NcCell,
        vl: &mut NcCell,
    ) -> NcResult<()> {
        error![unsafe {
            crate::nccells_rounded_box(plane, style as u32, channels, ul, ur, ll, lr, hl, vl)
        }]
    }
}
