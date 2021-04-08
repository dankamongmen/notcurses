//! `NcPlane*` methods and associated functions.

use core::{
    ptr::{null, null_mut},
    slice::from_raw_parts_mut,
};

use crate::{
    cstring, error, error_ref, error_ref_mut, rstring, NcAlign, NcAlphaBits, NcBlitter, NcBoxMask,
    NcCell, NcChannel, NcChannelPair, NcColor, NcDim, NcEgc, NcError, NcFadeCb, NcOffset,
    NcPaletteIndex, NcPlane, NcPlaneOptions, NcResizeCb, NcResult, NcRgb, NcStyleMask, NcTime,
    Notcurses, NCRESULT_ERR,
};

/// # NcPlaneOptions Constructors
impl NcPlaneOptions {
    /// New NcPlaneOptions using the horizontal x.
    pub fn new(y: NcOffset, x: NcOffset, rows: NcDim, cols: NcDim) -> Self {
        Self::with_flags(y, x, rows, cols, None, 0, 0, 0)
    }

    /// New NcPlaneOptions with horizontal alignment.
    pub fn new_aligned(y: NcOffset, align: NcAlign, rows: NcDim, cols: NcDim) -> Self {
        Self::with_flags_aligned(y, align, rows, cols, None, crate::NCPLANE_OPTION_HORALIGNED)
    }

    /// New NcPlaneOptions, with flags.
    pub fn with_flags(
        y: NcOffset,
        x: NcOffset,
        rows: NcDim,
        cols: NcDim,
        resizecb: Option<NcResizeCb>,
        flags: u64,
        margin_b: NcOffset,
        margin_r: NcOffset,
    ) -> Self {
        NcPlaneOptions {
            y: y as i32,
            x: x as i32,
            rows: rows as i32,
            cols: cols as i32,
            userptr: null_mut(),
            name: null(),
            resizecb: crate::ncresizecb_to_c(resizecb),
            flags,
            margin_b: margin_b as i32,
            margin_r: margin_r as i32,
        }
    }

    /// New NcPlaneOptions, with flags and horizontal alignment.
    ///
    /// Note: Already includes the
    /// [NCPLANE_OPTION_HORALIGNED][crate::NCPLANE_OPTION_HORALIGNED] flag.
    pub fn with_flags_aligned(
        y: NcOffset,
        align: NcAlign,
        rows: NcDim,
        cols: NcDim,
        resizecb: Option<NcResizeCb>,
        flags: u64,
    ) -> Self {
        let flags = crate::NCPLANE_OPTION_HORALIGNED | flags;
        NcPlaneOptions {
            y: y as i32,
            x: align as i32,
            rows: rows as i32,
            cols: cols as i32,
            userptr: null_mut(),
            name: null(),
            resizecb: crate::ncresizecb_to_c(resizecb),
            flags,
            margin_b: 0,
            margin_r: 0,
        }
    }
}

/// # NcPlane constructors & destructors
impl NcPlane {
    /// New NcPlane.
    ///
    /// The returned plane will be the top, bottom, and root of this new pile.
    ///
    /// *C style function: [ncpile_create()][crate::ncpile_create].*
    pub fn new<'a>(
        nc: &mut Notcurses,
        y: NcOffset,
        x: NcOffset,
        rows: NcDim,
        cols: NcDim,
    ) -> NcResult<&'a mut NcPlane> {
        Self::with_options(nc, NcPlaneOptions::new(y, x, rows, cols))
    }

    /// New NcPlane, expects an [NcPlaneOptions] struct.
    ///
    /// The returned plane will be the top, bottom, and root of this new pile.
    ///
    /// *C style function: [ncpile_create()][crate::ncpile_create].*
    pub fn with_options<'a>(
        nc: &mut Notcurses,
        options: NcPlaneOptions,
    ) -> NcResult<&'a mut NcPlane> {
        error_ref_mut![
            unsafe { crate::ncpile_create(nc, &options) },
            &format!["NcPlane::with_options(Notcurses, {:?})", options]
        ]
    }

    /// New NcPlane, bound to another NcPlane.
    ///
    /// *C style function: [ncplane_create()][crate::ncplane_create].*
    pub fn new_bound<'a>(
        bound_to: &mut NcPlane,
        y: NcOffset,
        x: NcOffset,
        rows: NcDim,
        cols: NcDim,
    ) -> NcResult<&'a mut NcPlane> {
        Self::with_options_bound(bound_to, NcPlaneOptions::new(y, x, rows, cols))
    }

    /// New NcPlane, bound to another plane, expects an [NcPlaneOptions] struct.
    ///
    /// *C style function: [ncplane_create()][crate::ncplane_create].*
    pub fn with_options_bound<'a>(
        bound_to: &mut NcPlane,
        options: NcPlaneOptions,
    ) -> NcResult<&'a mut NcPlane> {
        error_ref_mut![
            unsafe { crate::ncplane_create(bound_to, &options) },
            &format!("NcPlane::with_options_bound(NcPlane, {:?})", options)
        ]
    }

    /// New NcPlane, with the same dimensions of the terminal.
    ///
    /// The returned plane will be the top, bottom, and root of this new pile.
    ///
    /// *(No equivalent C style function)*
    pub fn with_termsize<'a>(nc: &mut Notcurses) -> NcResult<&'a mut NcPlane> {
        let (trows, tcols) = crate::notcurses_term_dim_yx(nc);
        assert![(trows > 0) & (tcols > 0)];
        Self::with_options(
            nc,
            NcPlaneOptions::new(0, 0, trows as NcDim, tcols as NcDim),
        )
    }

    /// Destroys this NcPlane.
    ///
    /// None of its contents will be visible after the next render call.
    /// It is an error to attempt to destroy the standard plane.
    ///
    /// *C style function: [ncplane_destroy()][crate::ncplane_destroy].*
    pub fn destroy(&mut self) -> NcResult<()> {
        error![unsafe { crate::ncplane_destroy(self) }, "NcPlane.destroy()"]
    }
}

// -----------------------------------------------------------------------------
/// ## NcPlane methods: `NcAlphaBits`
impl NcPlane {
    /// Gets the foreground [NcAlphaBits] from this NcPlane, shifted to LSBs.
    ///
    /// *C style function: [ncplane_fg_alpha()][crate::ncplane_fg_alpha].*
    #[inline]
    pub fn fg_alpha(&self) -> NcAlphaBits {
        crate::channels_fg_alpha(unsafe { crate::ncplane_channels(self) })
    }

    /// Gets the background [NcAlphaBits] for this NcPlane, shifted to LSBs.
    ///
    /// *C style function: [ncplane_bg_alpha()][crate::ncplane_bg_alpha].*
    #[inline]
    pub fn bg_alpha(&self) -> NcAlphaBits {
        crate::channels_bg_alpha(unsafe { crate::ncplane_channels(self) })
    }

    /// Sets the foreground [NcAlphaBits] from this NcPlane.
    ///
    /// *C style function: [ncplane_set_fg_alpha()][crate::ncplane_set_fg_alpha].*
    pub fn set_fg_alpha(&mut self, alpha: NcAlphaBits) -> NcResult<()> {
        error![
            unsafe { crate::ncplane_set_fg_alpha(self, alpha as i32) },
            &format!("NcPlane.set_fg_alpha({:0X})", alpha)
        ]
    }

    /// Sets the background [NcAlphaBits] for this NcPlane.
    ///
    /// *C style function: [ncplane_set_bg_alpha()][crate::ncplane_set_bg_alpha].*
    pub fn set_bg_alpha(&mut self, alpha: NcAlphaBits) -> NcResult<()> {
        error![
            unsafe { crate::ncplane_set_bg_alpha(self, alpha as i32) },
            &format!("NcPlane.set_bg_alpha({:0X})", alpha)
        ]
    }
}

// -----------------------------------------------------------------------------
/// ## NcPlane methods: `NcChannel`
impl NcPlane {
    /// Gets the current [NcChannelPair] from this NcPlane.
    ///
    /// *C style function: [ncplane_channels()][crate::ncplane_channels].*
    pub fn channels(&self) -> NcChannelPair {
        unsafe { crate::ncplane_channels(self) }
    }

    /// Gets the foreground [NcChannel] from an [NcPlane].
    ///
    /// *C style function: [ncplane_fchannel()][crate::ncplane_fchannel].*
    #[inline]
    pub fn fchannel(&self) -> NcChannel {
        crate::channels_fchannel(unsafe { crate::ncplane_channels(self) })
    }

    /// Gets the background [NcChannel] from an [NcPlane].
    ///
    /// *C style function: [ncplane_bchannel()][crate::ncplane_bchannel].*
    #[inline]
    pub fn bchannel(&self) -> NcChannel {
        crate::channels_bchannel(unsafe { crate::ncplane_channels(self) })
    }

    /// Sets the current [NcChannelPair] for this NcPlane.
    ///
    /// *C style function: [ncplane_set_channels()][crate::ncplane_set_channels].*
    pub fn set_channels(&mut self, channels: NcChannelPair) {
        unsafe { crate::ncplane_set_channels(self, channels) }
    }

    /// Sets the current foreground [NcChannel] for this NcPlane.
    /// Returns the updated [NcChannelPair].
    ///
    /// *C style function: [ncplane_set_fchannel()][crate::ncplane_set_fchannel].*
    pub fn set_fchannel(&mut self, channel: NcChannel) -> NcChannelPair {
        unsafe { crate::ncplane_set_fchannel(self, channel) }
    }

    /// Sets the current background [NcChannel] for this NcPlane.
    /// Returns the updated [NcChannelPair].
    ///
    /// *C style function: [ncplane_set_bchannel()][crate::ncplane_set_bchannel].*
    pub fn set_bchannel(&mut self, channel: NcChannel) -> NcChannelPair {
        unsafe { crate::ncplane_set_bchannel(self, channel) }
    }

    /// Sets the given [NcChannelPair]s throughout the specified region,
    /// keeping content and attributes unchanged.
    ///
    /// Returns the number of cells set.
    ///
    /// *C style function: [ncplane_stain()][crate::ncplane_stain].*
    pub fn stain(
        &mut self,
        y_stop: NcDim,
        x_stop: NcDim,
        ul: NcChannelPair,
        ur: NcChannelPair,
        ll: NcChannelPair,
        lr: NcChannelPair,
    ) -> NcResult<u32> {
        let res =
            unsafe { crate::ncplane_stain(self, y_stop as i32, x_stop as i32, ul, ur, ll, lr) };
        error![
            res,
            &format!(
                "NcPlane.stain({}, {}, {:0X}, {:0X}, {:0X}, {:0X})",
                y_stop, x_stop, ul, ur, ll, lr
            ),
            res as u32
        ]
    }
}

// -----------------------------------------------------------------------------
/// ## NcPlane methods: `NcColor`, `NcRgb` & default color
impl NcPlane {
    /// Gets the foreground [NcColor] RGB components from this NcPlane.
    ///
    /// *C style function: [ncplane_fg_rgb8()][crate::ncplane_fg_rgb8].*
    #[inline]
    pub fn fg_rgb8(&self) -> (NcColor, NcColor, NcColor) {
        let (mut r, mut g, mut b) = (0, 0, 0);
        let _ = crate::channels_fg_rgb8(
            unsafe { crate::ncplane_channels(self) },
            &mut r,
            &mut g,
            &mut b,
        );
        (r, g, b)
    }

    /// Gets the background [NcColor] RGB components from this NcPlane.
    ///
    /// *C style function: [ncplane_bg_rgb8()][crate::ncplane_bg_rgb8].*
    #[inline]
    pub fn bg_rgb8(&self) -> (NcColor, NcColor, NcColor) {
        let (mut r, mut g, mut b) = (0, 0, 0);
        let _ = crate::channels_bg_rgb8(
            unsafe { crate::ncplane_channels(self) },
            &mut r,
            &mut g,
            &mut b,
        );
        (r, g, b)
    }

    /// Sets the foreground [NcColor] RGB components for this NcPlane.
    ///
    /// If the terminal does not support directly-specified 3x8b cells
    /// (24-bit "TrueColor", indicated by the "RGB" terminfo capability),
    /// the provided values will be interpreted in some lossy fashion.
    ///
    /// "HP-like" terminals require setting foreground and background at the same
    /// time using "color pairs"; Notcurses will manage color pairs transparently.
    ///
    /// *C style function: [ncplane_set_fg_rgb8()][crate::ncplane_set_fg_rgb8].*
    pub fn set_fg_rgb8(&mut self, red: NcColor, green: NcColor, blue: NcColor) {
        unsafe {
            // Can't fail because of type enforcing.
            let _ = crate::ncplane_set_fg_rgb8(self, red as i32, green as i32, blue as i32);
        }
    }

    /// Sets the background [NcColor] RGB components for this NcPlane.
    ///
    /// If the terminal does not support directly-specified 3x8b cells
    /// (24-bit "TrueColor", indicated by the "RGB" terminfo capability),
    /// the provided values will be interpreted in some lossy fashion.
    ///
    /// "HP-like" terminals require setting foreground and background at the same
    /// time using "color pairs"; Notcurses will manage color pairs transparently.
    ///
    /// *C style function: [ncplane_set_bg_rgb8()][crate::ncplane_set_bg_rgb8].*
    pub fn set_bg_rgb8(&mut self, red: NcColor, green: NcColor, blue: NcColor) {
        unsafe {
            // Can't fail because of type enforcing.
            let _ = crate::ncplane_set_bg_rgb8(self, red as i32, green as i32, blue as i32);
        }
    }

    /// Gets the foreground [NcRgb] from this NcPlane, shifted to LSBs.
    ///
    /// *C style function: [ncplane_fg_rgb()][crate::ncplane_fg_rgb].*
    #[inline]
    pub fn fg_rgb(&self) -> NcRgb {
        crate::channels_fg_rgb(unsafe { crate::ncplane_channels(self) })
    }

    /// Gets the background [NcRgb] from this NcPlane, shifted to LSBs.
    ///
    /// *C style function: [ncplane_bg_rgb()][crate::ncplane_bg_rgb].*
    #[inline]
    pub fn bg_rgb(&self) -> NcRgb {
        crate::channels_bg_rgb(unsafe { crate::ncplane_channels(self) })
    }

    /// Sets the foreground [NcRgb] for this NcPlane.
    ///
    /// *C style function: [ncplane_set_fg_rgb()][crate::ncplane_set_fg_rgb].*
    #[inline]
    pub fn set_fg_rgb(&mut self, rgb: NcRgb) {
        unsafe {
            crate::ncplane_set_fg_rgb(self, rgb);
        }
    }

    /// Sets the background [NcRgb] for this NcPlane.
    ///
    /// *C style function: [ncplane_set_bg_rgb()][crate::ncplane_set_bg_rgb].*
    #[inline]
    pub fn set_bg_rgb(&mut self, rgb: NcRgb) {
        unsafe {
            crate::ncplane_set_bg_rgb(self, rgb);
        }
    }

    /// Is this NcPlane's foreground using the "default foreground color"?
    ///
    /// *C style function: [ncplane_fg_default_p()][crate::ncplane_fg_default_p].*
    #[inline]
    pub fn fg_default_p(&self) -> bool {
        crate::channels_fg_default_p(unsafe { crate::ncplane_channels(self) })
    }

    /// Is this NcPlane's background using the "default background color"?
    ///
    /// *C style function: [ncplane_bg_default_p()][crate::ncplane_bg_default_p].*
    #[inline]
    pub fn bg_default_p(&self) -> bool {
        crate::channels_bg_default_p(unsafe { crate::ncplane_channels(self) })
    }

    /// Uses the default color for the foreground.
    ///
    /// *C style function: [ncplane_set_fg_default()][crate::ncplane_set_fg_default].*
    #[inline]
    pub fn set_fg_default(&mut self) {
        unsafe {
            crate::ncplane_set_fg_default(self);
        }
    }

    /// Uses the default color for the background.
    ///
    /// *C style function: [ncplane_set_bg_default()][crate::ncplane_set_bg_default].*
    #[inline]
    pub fn set_bg_default(&mut self) {
        unsafe {
            crate::ncplane_set_bg_default(self);
        }
    }
}

// -----------------------------------------------------------------------------
/// ## NcPlane methods: `NcStyleMask` & `PaletteIndex`
impl NcPlane {
    /// Sets the given style throughout the specified region, keeping content
    /// and channels unchanged.
    ///
    /// Returns the number of cells set.
    ///
    /// *C style function: [ncplane_format()][crate::ncplane_format].*
    pub fn format(
        &mut self,
        y_stop: NcDim,
        x_stop: NcDim,
        stylemask: NcStyleMask,
    ) -> NcResult<NcDim> {
        let res =
            unsafe { crate::ncplane_format(self, y_stop as i32, x_stop as i32, stylemask as u32) };
        error![
            res,
            &format!("NcPlane.format({}, {}, {:0X})", y_stop, x_stop, stylemask),
            res as u32
        ]
    }

    /// Returns the current styling for this NcPlane.
    ///
    /// *C style function: [ncplane_styles()][crate::ncplane_styles].*
    pub fn styles(&self) -> NcStyleMask {
        unsafe { crate::ncplane_styles(self) }
    }

    /// Removes the specified styles from this NcPlane's existing spec.
    ///
    /// *C style function: [ncplane_off_styles()][crate::ncplane_off_styles].*
    pub fn off_styles(&mut self, stylemask: NcStyleMask) {
        unsafe {
            crate::ncplane_off_styles(self, stylemask as u32);
        }
    }

    /// Adds the specified styles to this NcPlane's existing spec.
    ///
    /// *C style function: [ncplane_on_styles()][crate::ncplane_on_styles].*
    pub fn on_styles(&mut self, stylemask: NcStyleMask) {
        unsafe {
            crate::ncplane_on_styles(self, stylemask as u32);
        }
    }

    /// Sets just the specified styles for this NcPlane.
    ///
    /// *C style function: [ncplane_set_styles()][crate::ncplane_set_styles].*
    pub fn set_styles(&mut self, stylemask: NcStyleMask) {
        unsafe {
            crate::ncplane_set_styles(self, stylemask as u32);
        }
    }

    /// Sets this NcPlane's foreground [NcPaletteIndex].
    ///
    /// Also sets the foreground palette index bit, sets it foreground-opaque,
    /// and clears the foreground default color bit.
    ///
    /// *C style function: [ncplane_set_fg_palindex()][crate::ncplane_set_fg_palindex].*
    pub fn set_fg_palindex(&mut self, palindex: NcPaletteIndex) {
        unsafe {
            crate::ncplane_set_fg_palindex(self, palindex as i32);
        }
    }

    /// Sets this NcPlane's background [NcPaletteIndex].
    ///
    /// Also sets the background palette index bit, sets it background-opaque,
    /// and clears the background default color bit.
    ///
    /// *C style function: [ncplane_set_bg_palindex()][crate::ncplane_set_bg_palindex].*
    pub fn set_bg_palindex(&mut self, palindex: NcPaletteIndex) {
        unsafe {
            crate::ncplane_set_bg_palindex(self, palindex as i32);
        }
    }
}

// -----------------------------------------------------------------------------
/// ## NcPlane methods: `NcCell` & `NcEgc`
impl NcPlane {
    /// Retrieves the current contents of the [NcCell] under the cursor,
    /// returning the [NcEgc] and writing out the [NcStyleMask] and the [NcChannelPair].
    ///
    /// This NcEgc must be freed by the caller.
    ///
    /// *C style function: [ncplane_at_cursor()][crate::ncplane_at_cursor].*
    pub fn at_cursor(
        &mut self,
        stylemask: &mut NcStyleMask,
        channels: &mut NcChannelPair,
    ) -> NcResult<NcEgc> {
        let egc = unsafe { crate::ncplane_at_cursor(self, stylemask, channels) };
        if egc.is_null() {
            return Err(NcError::with_msg(
                NCRESULT_ERR,
                &format!("NcPlane.at_cursor({:0X}, {:0X})", stylemask, channels),
            ));
        }
        let egc = core::char::from_u32(unsafe { *egc } as u32).expect("wrong char");
        Ok(egc)
    }

    /// Retrieves the current contents of the [NcCell] under the cursor
    /// into `cell`. Returns the number of bytes in the [NcEgc].
    ///
    /// This NcCell is invalidated if the associated NcPlane is destroyed.
    ///
    /// *C style function: [ncplane_at_cursor_cell()][crate::ncplane_at_cursor_cell].*
    #[inline]
    pub fn at_cursor_cell(&mut self, cell: &mut NcCell) -> NcResult<u32> {
        let bytes = unsafe { crate::ncplane_at_cursor_cell(self, cell) };
        error![
            bytes,
            &format!("NcPlane.at_cursor_cell({:?})", cell),
            bytes as u32
        ]
    }

    /// Retrieves the current contents of the specified [NcCell], returning the
    /// [NcEgc] and writing out the [NcStyleMask] and the [NcChannelPair].
    ///
    /// This NcEgc must be freed by the caller.
    ///
    /// *C style function: [ncplane_at_yx()][crate::ncplane_at_yx].*
    pub fn at_yx(
        &mut self,
        y: NcDim,
        x: NcDim,
        stylemask: &mut NcStyleMask,
        channels: &mut NcChannelPair,
    ) -> NcResult<NcEgc> {
        let egc = unsafe { crate::ncplane_at_yx(self, y as i32, x as i32, stylemask, channels) };
        if egc.is_null() {
            return Err(NcError::with_msg(
                NCRESULT_ERR,
                &format!(
                    "NcPlane.at_yx({}, {}, {:0X}, {:0X})",
                    y, x, stylemask, channels
                ),
            ));
        }
        let egc = core::char::from_u32(unsafe { *egc } as u32).expect("wrong char");
        Ok(egc)
    }

    /// Retrieves the current contents of the specified [NcCell] into `cell`.
    /// Returns the number of bytes in the [NcEgc].
    ///
    /// This NcCell is invalidated if the associated plane is destroyed.
    ///
    /// *C style function: [ncplane_at_yx_cell()][crate::ncplane_at_yx_cell].*
    #[inline]
    pub fn ncplane_at_yx_cell(&mut self, y: NcDim, x: NcDim, cell: &mut NcCell) -> NcResult<u32> {
        let bytes = unsafe { crate::ncplane_at_yx_cell(self, y as i32, x as i32, cell) };
        error![
            bytes,
            &format!("NcPlane.at_yx_cell({}, {}, {:?})", y, x, cell),
            bytes as u32
        ]
    }

    /// Extracts this NcPlane's base [NcCell] into `cell`.
    ///
    /// The reference is invalidated if this NcPlane is destroyed.
    ///
    /// *C style function: [ncplane_base()][crate::ncplane_base].*
    pub fn base(&mut self) -> NcResult<NcCell> {
        let mut cell = NcCell::new();
        let res = unsafe { crate::ncplane_base(self, &mut cell) };
        error![res, "NcPlane.base()", cell]
    }

    /// Sets this NcPlane's base [NcCell] from its components.
    ///
    /// It will be used for purposes of rendering anywhere that the NcPlane's
    /// gcluster is 0.
    ///
    /// Erasing the NcPlane does not reset the base cell.
    ///
    /// *C style function: [ncplane_set_base()][crate::ncplane_set_base].*
    // call stack:
    // - ncplane_set_base calls cell_prime:
    //      return cell_prime(ncp, &ncp->basecell, egc, stylemask, channels);
    // - cell_prime calls notcurses.c/cell_load:
    //      return cell_load(n, c, gcluster);
    // - cell-load calls internal.h/pool load:
    //      return pool_load(&n->pool, c, gcluster);
    pub fn set_base(
        &mut self,
        egc: &str,
        stylemask: NcStyleMask,
        channels: NcChannelPair,
    ) -> NcResult<u32> {
        let res =
            unsafe { crate::ncplane_set_base(self, cstring![egc], stylemask as u32, channels) };
        error![
            res,
            &format!(
                "NcPlane.set_base({:?}, {:0X}, {:0X})",
                egc, stylemask, channels
            ),
            res as u32
        ]
    }

    /// Sets this NcPlane's base NcCell.
    ///
    /// It will be used for purposes of rendering anywhere that the NcPlane's
    /// gcluster is 0.
    ///
    /// Erasing the NcPlane does not reset the base cell.
    ///
    /// *C style function: [ncplane_set_base_cell()][crate::ncplane_set_base_cell].*
    pub fn set_base_cell(&mut self, cell: &NcCell) -> NcResult<()> {
        error![
            unsafe { crate::ncplane_set_base_cell(self, cell) },
            &format!("NcPlane.base({:?})", cell)
        ]
    }

    /// Creates a flat string from the NcEgc's of the selected region of the
    /// NcPlane.
    ///
    /// Starts at the plane's `beg_y` * `beg_x` coordinates (which must lie on
    /// the plane), continuing for `len_y` x `len_x` cells.
    ///
    /// If either `through_y` or `through_x` are true, then `len_y` or `len_x`,
    /// will be respectively ignored, and will go through the boundary of the plane.
    ///
    /// *C style function: [ncplane_contents()][crate::ncplane_contents].*
    pub fn contents(
        &self,
        beg_y: NcDim,
        beg_x: NcDim,
        len_y: NcDim,
        len_x: NcDim,
        through_y: bool,
        through_x: bool,
    ) -> String {
        let (mut len_y, mut len_x) = (len_y as i32, len_x as i32);
        if through_y {
            len_y = -1;
        }
        if through_x {
            len_x = -1;
        }
        rstring![crate::ncplane_contents(
            self,
            beg_y as i32,
            beg_x as i32,
            len_y,
            len_x
        )]
        .to_string()
    }

    /// Erases every NcCell in this NcPlane, resetting all attributes to normal,
    /// all colors to the default color, and all cells to undrawn.
    ///
    /// All cells associated with this NcPlane are invalidated, and must not be
    /// used after the call, excluding the base cell. The cursor is homed.
    ///
    /// *C style function: [ncplane_erase()][crate::ncplane_erase].*
    pub fn erase(&mut self) {
        unsafe {
            crate::ncplane_erase(self);
        }
    }

    /// Replaces the NcCell at the specified coordinates with the provided NcCell,
    /// advancing the cursor by its width (but not past the end of the plane).
    ///
    /// The new NcCell must already be associated with the Plane.
    /// On success, returns the number of columns the cursor was advanced.
    ///
    /// *C style function: [ncplane_putc_yx()][crate::ncplane_putc_yx].*
    pub fn putc_yx(&mut self, y: NcDim, x: NcDim, cell: &NcCell) -> NcResult<NcDim> {
        let res = unsafe { crate::ncplane_putc_yx(self, y as i32, x as i32, cell) };
        error![
            res,
            &format!("NcPlane.putc_yx({}, {}, {:?})", y, x, cell),
            res as NcDim
        ]
    }

    /// Replaces the NcCell at the current coordinates with the provided NcCell,
    /// advancing the cursor by its width (but not past the end of the plane).
    ///
    /// The new NcCell must already be associated with the Plane.
    /// On success, returns the number of columns the cursor was advanced.
    ///
    /// *C style function: [ncplane_putc()][crate::ncplane_putc].*
    pub fn putc(&mut self, cell: &NcCell) -> NcResult<NcDim> {
        let res = crate::ncplane_putc(self, cell);
        error![res, &format!("NcPlane.putc({:?})", cell), res as NcDim]
    }

    /// Calls [putchar_yx][NcPlane#method.putchar_yx] at the current cursor location.
    ///
    /// On success, returns the number of columns the cursor was advanced.
    ///
    /// *C style function: [ncplane_putchar()][crate::ncplane_putchar].*
    pub fn putchar(&mut self, ch: char) -> NcResult<NcDim> {
        let res = crate::ncplane_putchar(self, ch);
        error![res, &format!("NcPlane.putchar({:?})", ch), res as NcDim]
    }

    // TODO: call put_egc
    // /// Replaces the [NcEgc][crate::NcEgc] to the current location, but retain
    // /// the styling. The current styling of the plane will not be changed.
    // pub fn putchar_stained(&mut self, y: NcDim, x: NcDim, ch: char) ->
    // NcResult<NcDim> {
    //     error![crate::ncplane_putchar_stained(self, ch)]
    // }

    /// Replaces the [NcEgc][crate::NcEgc], but retain the styling.
    /// The current styling of the plane will not be changed.
    ///
    /// On success, returns the number of columns the cursor was advanced.
    ///
    /// *C style function: [ncplane_putchar_yx()][crate::ncplane_putchar_yx].*
    pub fn putchar_yx(&mut self, y: NcDim, x: NcDim, ch: char) -> NcResult<NcDim> {
        let res = crate::ncplane_putchar_yx(self, y, x, ch);
        error![
            res,
            &format!("NcPlane.putchar_yx({}, {}, {:?})", y, x, ch),
            res as NcDim
        ]
    }

    /// Writes a series of [NcEgc][crate::NcEgc]s to the current location,
    /// using the current style.
    ///
    /// Advances the cursor by some positive number of columns
    /// (though not beyond the end of the plane),
    /// and this number is returned on success.
    ///
    /// On error, a non-positive number is returned, indicating
    /// the number of columns which were written before the error.
    ///
    /// *C style function: [ncplane_putstr()][crate::ncplane_putstr].*
    #[inline]
    pub fn putstr(&mut self, string: &str) -> NcResult<NcDim> {
        let res = crate::ncplane_putstr(self, string);
        error![res, &format!("NcPlane.putstr({:?})", string), res as NcDim]
    }

    /// Same as [putstr][NcPlane#method.putstr], but it also tries to move the
    /// cursor to the beginning of the next row.
    ///
    /// Advances the cursor by some positive number of columns (though not beyond
    /// the end of the plane); this number is returned on success.
    ///
    /// On error, a non-positive number is returned, indicating the number of
    /// columns which were written before the error.
    ///
    /// *(No equivalent C style function)*
    pub fn putstrln(&mut self, string: &str) -> NcResult<NcDim> {
        let cols = self.putstr(string)?;
        let (y, _x) = self.cursor_yx();
        self.cursor_move_yx(y + 1, 0)?;
        return Ok(cols);
    }

    /// Same as [putstr_yx()][NcPlane#method.putstr_yx] but [NcAlign]ed on x.
    ///
    /// *C style function: [ncplane_putstr_aligned()][crate::ncplane_putstr_aligned].*
    pub fn putstr_aligned(&mut self, y: NcDim, align: NcAlign, string: &str) -> NcResult<NcDim> {
        let res = unsafe { crate::ncplane_putstr_aligned(self, y as i32, align, cstring![string]) };
        error![
            res,
            &format!("NcPlane.putstr_aligned({}, {}, {:?})", y, align, string),
            res as NcDim
        ]
    }

    /// Writes a series of [NcEgc][crate::NcEgc]s to the current location, but
    /// retain the styling.
    /// The current styling of the plane will not be changed.
    ///
    /// Advances the cursor by some positive number of columns (though not beyond
    /// the end of the plane); this number is returned on success.
    ///
    /// On error, a non-positive number is returned, indicating the number of
    /// columns which were written before the error.
    ///
    /// *C style function: [ncplane_putstr_stained()][crate::ncplane_putstr_stained].*
    pub fn putstr_stained(&mut self, string: &str) -> NcResult<NcDim> {
        let res = unsafe { crate::ncplane_putstr_stained(self, cstring![string]) };
        error![
            res,
            &format!("NcPlane.putstr_stained({:?})", string),
            res as NcDim
        ]
    }

    /// Write a string, which is a series of [NcEgc][crate::NcEgc]s, to the
    /// current location, using the current style.
    ///
    /// They will be interpreted as a series of columns.
    ///
    /// Advances the cursor by some positive number of columns (though not
    /// beyond the end of the plane); this number is returned on success.
    ///
    /// On error, a non-positive number is returned, indicating the number of
    /// columns which were written before the error.
    ///
    /// *C style function: [ncplane_putstr_yx()][crate::ncplane_putstr_yx].*
    pub fn putstr_yx(&mut self, y: NcDim, x: NcDim, string: &str) -> NcResult<NcDim> {
        let res = unsafe { crate::ncplane_putstr_yx(self, y as i32, x as i32, cstring![string]) };
        error![
            res,
            &format!("NcPlane.putstr_yx({}, {}, {:?})", y, x, string),
            res as NcDim
        ]
    }
}

// -----------------------------------------------------------------------------
/// ## NcPlane methods: `NcPlane` & `Notcurses`
impl NcPlane {
    /// Gets the origin of this plane relative to its pile.
    ///
    /// *C style function: [ncplane_abs_yx()][crate::ncplane_abs_yx].*
    pub fn abs_yx(&self) -> (NcDim, NcDim) {
        let mut y = 0;
        let mut x = 0;
        unsafe {
            crate::ncplane_abs_yx(self, &mut y, &mut x);
        }
        (y as NcDim, x as NcDim)
    }

    /// Gets the origin of this plane relative to its pile, in the y axis.
    ///
    /// *C style function: [ncplane_abs_y()][crate::ncplane_abs_y].*
    pub fn abs_y(&self) -> NcDim {
        unsafe { crate::ncplane_abs_y(self) as NcDim }
    }

    /// Gets the origin of this plane relative to its pile, in the x axis.
    ///
    /// *C style function: [ncplane_abs_x()][crate::ncplane_abs_x].*
    pub fn abs_x(&self) -> NcDim {
        unsafe { crate::ncplane_abs_x(self) as NcDim }
    }

    /// Duplicates this NcPlane.
    ///
    /// The new NcPlane will have the same geometry, the same rendering state,
    /// and all the same duplicated content.
    ///
    /// The new plane will be immediately above the old one on the z axis,
    /// and will be bound to the same parent. Bound planes are not duplicated;
    /// the new plane is bound to the current parent, but has no bound planes.
    ///
    /// *C style function: [ncplane_dup()][crate::ncplane_dup].*
    //
    // TODO: deal with the opaque field.
    pub fn dup<'a>(&'a mut self) -> &'a mut NcPlane {
        unsafe { &mut *crate::ncplane_dup(self, null_mut()) }
    }

    /// Returns the topmost NcPlane of the current pile.
    ///
    /// *C style function: [ncpile_top()][crate::ncpile_top].*
    pub fn top<'a>(&mut self) -> &'a mut NcPlane {
        unsafe { &mut *crate::ncpile_top(self) }
    }

    /// Returns the bottommost NcPlane of the current pile.
    ///
    /// *C style function: [ncpile_bottom()][crate::ncpile_bottom].*
    pub fn bottom<'a>(&mut self) -> &'a mut NcPlane {
        unsafe { &mut *crate::ncpile_bottom(self) }
    }

    /// Relocates this NcPlane at the top of the z-buffer.
    ///
    /// *C style function: [ncplane_move_top()][crate::ncplane_move_top].*
    pub fn move_top(&mut self) {
        unsafe {
            crate::ncplane_move_top(self);
        }
    }

    /// Relocates this NcPlane at the bottom of the z-buffer.
    ///
    /// *C style function: [ncplane_move_bottom()][crate::ncplane_move_bottom].*
    pub fn move_bottom(&mut self) {
        unsafe {
            crate::ncplane_move_bottom(self);
        }
    }

    /// Moves this NcPlane relative to the standard plane, or the plane to
    /// which it is bound.
    ///
    /// It is an error to attempt to move the standard plane.
    ///
    /// *C style function: [ncplane_move_yx()][crate::ncplane_move_yx].*
    pub fn move_yx(&mut self, y: NcOffset, x: NcOffset) -> NcResult<()> {
        error![
            unsafe { crate::ncplane_move_yx(self, y, x) },
            &format!("NcPlane.move_yx({}, {})", y, x)
        ]
    }

    /// Moves this NcPlane relative to its current position.
    ///
    /// It is an error to attempt to move the standard plane.
    ///
    /// *(No equivalent C style function)*
    pub fn move_rel(&mut self, rows: NcOffset, cols: NcOffset) -> NcResult<()> {
        let (y, x) = self.yx();
        error![
            unsafe { crate::ncplane_move_yx(self, y + rows, x + cols) },
            &format!("NcPlane.move_yx({}, {})", y, x)
        ]
    }

    /// Returns the NcPlane above this one, or None if already at the top.
    ///
    /// *C style function: [ncplane_above()][crate::ncplane_above].*
    pub fn above<'a>(&'a mut self) -> NcResult<&'a mut NcPlane> {
        error_ref_mut![unsafe { crate::ncplane_above(self) }, "NcPlane.above()"]
    }

    /// Returns the NcPlane below this one, or None if already at the bottom.
    ///
    /// *C style function: [ncplane_below()][crate::ncplane_below].*
    pub fn below<'a>(&'a mut self) -> NcResult<&'a mut NcPlane> {
        error_ref_mut![unsafe { crate::ncplane_below(self) }, "NcPlane.below()"]
    }

    /// Relocates this NcPlane above the `above` NcPlane, in the z-buffer.
    ///
    /// Returns [NCRESULT_ERR] if the current plane is
    /// already in the desired location. Both planes must not be the same.
    ///
    /// *C style function: [ncplane_move_above()][crate::ncplane_move_above].*
    pub fn move_above(&mut self, above: &mut NcPlane) -> NcResult<()> {
        error![
            unsafe { crate::ncplane_move_above(self, above) },
            "NcPlane.move_above(NcPlane)"
        ]
    }

    /// Relocates this NcPlane below the `below` NcPlane, in the z-buffer.
    ///
    /// Returns [NCRESULT_ERR] if the current plane is
    /// already in the desired location. Both planes must not be the same.
    ///
    /// *C style function: [ncplane_move_below()][crate::ncplane_move_below].*
    pub fn move_below(&mut self, below: &mut NcPlane) -> NcResult<()> {
        error![
            unsafe { crate::ncplane_move_below(self, below) },
            "NcPlane.move_below(NcPlane)"
        ]
    }

    /// Merges `source` down onto this NcPlane.
    ///
    /// Merging is independent of the position of both NcPlanes on the z-axis.
    ///
    /// It is an error to define a subregion of zero area, or that is not
    /// entirely contained within `source`.
    ///
    /// It is an error to define a target origin such that the projected
    /// subregion is not entirely contained within 'dst'.
    ///
    /// Behavior is undefined if both NcPlanes are equivalent.
    ///
    /// *C style function: [ncplane_mergedown()][crate::ncplane_mergedown].*
    pub fn mergedown(
        &mut self,
        source: &NcPlane,
        source_y: NcDim,
        source_x: NcDim,
        len_y: NcDim,
        len_x: NcDim,
        target_y: NcDim,
        target_x: NcDim,
    ) -> NcResult<()> {
        error![
            unsafe {
                crate::ncplane_mergedown(
                    source,
                    self,
                    source_y as i32,
                    source_x as i32,
                    len_y as i32,
                    len_x as i32,
                    target_y as i32,
                    target_x as i32,
                )
            },
            &format!(
                "NcPlane.mergedown(NcPlane, {}, {}, {}, {}, {}, {})",
                source_y, source_x, len_y, len_x, target_y, target_x
            )
        ]
    }

    /// Merges `source` down onto this NcPlane.
    ///
    /// If `source` does not intersect, this plane will not be changed,
    /// but it is not an error.
    ///
    /// See [`mergedown`][NcPlane#method.mergedown]
    /// for more information.
    ///
    /// *C style function: [ncplane_mergedown_simple()][crate::ncplane_mergedown_simple].*
    //
    // TODO: maybe create a reversed method, and/or an associated function,
    // for `mergedown` too.
    pub fn mergedown_simple(&mut self, source: &NcPlane) -> NcResult<()> {
        error![
            unsafe { crate::ncplane_mergedown_simple(source, self) },
            "NcPlane.mergedown_simple(NcPlane)"
        ]
    }

    /// Gets the parent to which this NcPlane is bound, if any.
    ///
    /// *C style function: [ncplane_parent()][crate::ncplane_parent].*
    //
    // TODO: CHECK: what happens when it's bound to itself.
    pub fn parent<'a>(&'a mut self) -> NcResult<&'a mut NcPlane> {
        error_ref_mut![unsafe { crate::ncplane_parent(self) }, "NcPlane.parent()"]
    }

    /// Gets the parent to which this NcPlane is bound, if any.
    ///
    /// *C style function: [ncplane_parent_const()][crate::ncplane_parent_const].*
    //
    // CHECK: what happens when it's bound to itself.
    pub fn parent_const<'a>(&'a self) -> NcResult<&'a NcPlane> {
        error_ref![
            unsafe { crate::ncplane_parent_const(self) },
            "NcPlane.parent_const()"
        ]
    }

    /// Unbounds this NcPlane from its parent, makes it a bound child of
    /// 'newparent', and returns itself.
    ///
    /// Any planes bound to this NcPlane are reparented to the previous parent.
    ///
    /// If this NcPlane is equal to `newparent`, then becomes the root of a new
    /// pile, unless it is already the root of a pile, in which case this is a
    /// no-op.
    ///
    /// The standard plane cannot be reparented.
    ///
    /// *C style function: [ncplane_reparent()][crate::ncplane_reparent].*
    pub fn reparent<'a>(&mut self, newparent: &'a mut NcPlane) -> NcResult<&'a mut NcPlane> {
        error_ref_mut![
            unsafe { crate::ncplane_reparent(self, newparent) },
            "NcPlane.reparent(NcPlane)"
        ]
    }

    /// Like [`reparent`][NcPlane#method.reparent], except any bound
    /// planes comes along with this NcPlane to its new destination.
    ///
    /// Their z-order is maintained.
    ///
    /// *C style function: [ncplane_reparent_family()][crate::ncplane_reparent_family].*
    //
    // TODO:CHECK: If 'newparent' is an ancestor, NULL is returned & no changes're made.
    pub fn reparent_family<'a>(&mut self, newparent: &'a mut NcPlane) -> NcResult<&'a mut NcPlane> {
        error_ref_mut![
            unsafe { crate::ncplane_reparent_family(self, newparent) },
            "NcPlane.reparent_family(NcPlane)"
        ]
    }

    /// Makes the physical screen match the last rendered frame from the pile of
    /// which this NcPlane is a part.
    ///
    /// This is a blocking call. Don't call this before the pile has been
    /// rendered (doing so will likely result in a blank screen).
    ///
    /// *C style function: [ncpile_rasterize()][crate::ncpile_rasterize].*
    pub fn rasterize<'a>(&mut self) -> NcResult<()> {
        error![
            unsafe { crate::ncpile_rasterize(self) },
            "NcPlane.rasterize()"
        ]
    }

    /// Renders the pile of which this NcPlane is a part.
    /// Rendering this pile again will blow away the render.
    /// To actually write out the render, call ncpile_rasterize().
    ///
    /// *C style function: [ncpile_render()][crate::ncpile_render].*
    pub fn render<'a>(&mut self) -> NcResult<()> {
        error![unsafe { crate::ncpile_render(self) }, "NcPlane.render()"]
    }

    /// Gets a mutable reference to the [Notcurses] context of this NcPlane.
    ///
    /// *C style function: [ncplane_notcurses()][crate::ncplane_notcurses].*
    pub fn notcurses<'a>(&mut self) -> NcResult<&'a mut Notcurses> {
        error_ref_mut![
            unsafe { crate::ncplane_notcurses(self) },
            "NcPlane.notcurses()"
        ]
    }

    /// Gets an immutable reference to the [Notcurses] context of this NcPlane.
    ///
    /// *C style function: [ncplane_notcurses_const()][crate::ncplane_notcurses_const].*
    pub fn notcurses_const<'a>(&self) -> NcResult<&'a Notcurses> {
        error_ref![
            unsafe { crate::ncplane_notcurses_const(self) },
            "NcPlane.notcurses()"
        ]
    }
}

// -----------------------------------------------------------------------------
/// ## NcPlane methods: cursor
impl NcPlane {
    /// Moves the cursor to 0, 0.
    ///
    /// *C style function: [ncplane_home()][crate::ncplane_home].*
    pub fn home(&mut self) {
        unsafe {
            crate::ncplane_home(self);
        }
    }

    /// Returns the current position of the cursor within this NcPlane.
    ///
    /// *C style function: [ncplane_cursor_yx()][crate::ncplane_cursor_yx].*
    //
    // NOTE: y and/or x may be NULL.
    // check for null and return NcResult
    pub fn cursor_yx(&self) -> (NcDim, NcDim) {
        let (mut y, mut x) = (0, 0);
        unsafe { crate::ncplane_cursor_yx(self, &mut y, &mut x) };
        (y as NcDim, x as NcDim)
    }

    /// Returns the current row of the cursor within this NcPlane.
    ///
    /// *(No equivalent C style function)*
    pub fn cursor_y(&self) -> NcDim {
        self.cursor_yx().0
    }

    /// Returns the current column of the cursor within this NcPlane.
    ///
    /// *(No equivalent C style function)*
    pub fn cursor_x(&self) -> NcDim {
        self.cursor_yx().1
    }

    /// Moves the cursor to the specified position within this NcPlane.
    ///
    /// The cursor doesn't need to be visible.
    ///
    /// Parameters exceeding the plane's dimensions will result in an error,
    /// and the cursor position will remain unchanged.
    ///
    /// *C style function: [ncplane_cursor_move_yx()][crate::ncplane_cursor_move_yx].*
    pub fn cursor_move_yx(&mut self, y: NcDim, x: NcDim) -> NcResult<()> {
        error![
            unsafe { crate::ncplane_cursor_move_yx(self, y as i32, x as i32) },
            &format!("NcPlane.move_yx({}, {})", y, x)
        ]
    }

    /// Moves the cursor to the specified row within this NcPlane.
    ///
    /// *(No equivalent C style function)*
    pub fn cursor_move_y(&mut self, y: NcDim) -> NcResult<()> {
        let x = self.cursor_x();
        error![
            unsafe { crate::ncplane_cursor_move_yx(self, y as i32, x as i32) },
            &format!("NcPlane.move_y({})", y)
        ]
    }

    /// Moves the cursor to the specified column within this NcPlane.
    ///
    /// *(No equivalent C style function)*
    pub fn cursor_move_x(&mut self, x: NcDim) -> NcResult<()> {
        let y = self.cursor_y();
        error![
            unsafe { crate::ncplane_cursor_move_yx(self, y as i32, x as i32) },
            &format!("NcPlane.move_x({})", x)
        ]
    }

    /// Moves the cursor the number of rows specified (forward or backwards).
    ///
    /// It will error if the target row exceeds the plane dimensions.
    ///
    /// *(No equivalent C style function)*
    pub fn cursor_move_rows(&mut self, rows: NcOffset) -> NcResult<()> {
        let (y, x) = self.cursor_yx();
        self.cursor_move_yx((y as NcOffset + rows) as NcDim, x)
    }

    /// Moves the cursor the number of columns specified (forward or backwards).
    ///
    /// It will error if the target column exceeds the plane dimensions.
    ///
    /// *(No equivalent C style function)*
    pub fn cursor_move_cols(&mut self, cols: NcOffset) -> NcResult<()> {
        let (y, x) = self.cursor_yx();
        self.cursor_move_yx(y, (x as NcOffset + cols) as NcDim)
    }
}

// -----------------------------------------------------------------------------
/// ## NcPlane methods: size, position & alignment
impl NcPlane {
    /// Returns the column at which `cols` columns ought start in order to be
    /// aligned according to `align` within this NcPlane.
    ///
    /// Returns `-`[NCRESULT_MAX][crate::NCRESULT_MAX] if
    /// [NCALIGN_UNALIGNED][crate::NCALIGN_UNALIGNED] or invalid [NcAlign].
    ///
    /// *C style function: [ncplane_halign()][crate::ncplane_halign].*
    #[inline]
    pub fn halign(&mut self, align: NcAlign, cols: NcDim) -> NcResult<()> {
        error![
            crate::ncplane_halign(self, align, cols),
            &format!("NcPlane.halign({:?}, {})", align, cols)
        ]
    }

    /// Returns the row at which `rows` rows ought start in order to be
    /// aligned according to `align` within this NcPlane.
    ///
    /// Returns `-`[NCRESULT_MAX][crate::NCRESULT_MAX] if
    /// [NCALIGN_UNALIGNED][crate::NCALIGN_UNALIGNED] or invalid [NcAlign].
    ///
    /// *C style function: [ncplane_valign()][crate::ncplane_valign].*
    #[inline]
    pub fn valign(&mut self, align: NcAlign, cols: NcDim) -> NcResult<()> {
        error![
            crate::ncplane_valign(self, align, cols),
            &format!("NcPlane.valign({:?}, {})", align, cols)
        ]
    }

    ///
    ///
    /// *C style function: [ncplane_center_abs()][crate::ncplane_center_abs].*
    //
    // TODO: doc.
    pub fn center_abs(&self, y: &mut NcDim, x: &mut NcDim) {
        unsafe {
            crate::ncplane_center_abs(self, &mut (*y as i32), &mut (*x as i32));
        }
    }

    /// Returns the dimensions of this NcPlane.
    ///
    /// *C style function: [ncplane_dim_yx()][crate::ncplane_dim_yx].*
    pub fn dim_yx(&self) -> (NcDim, NcDim) {
        let (mut y, mut x) = (0, 0);
        unsafe { crate::ncplane_dim_yx(self, &mut y, &mut x) };
        (y as NcDim, x as NcDim)
    }

    /// Return the rows of this NcPlane.
    ///
    /// *C style function: [ncplane_dim_y()][crate::ncplane_dim_y].*
    #[inline]
    pub fn dim_y(&self) -> NcDim {
        self.dim_yx().0
    }

    /// Return the columns of this NcPlane.
    ///
    /// *C style function: [ncplane_dim_x()][crate::ncplane_dim_x].*
    #[inline]
    pub fn dim_x(&self) -> NcDim {
        self.dim_yx().1
    }

    /// Return the rows of this NcPlane.
    ///
    /// Alias of [dim_y][NcPlane#method.dim_y]
    ///
    /// *C style function: [ncplane_dim_y()][crate::ncplane_dim_y].*
    #[inline]
    pub fn rows(&self) -> NcDim {
        self.dim_yx().0
    }

    /// Return the cols of this NcPlane.
    ///
    /// Alias of [dim_x][NcPlane#method.dim_x]
    ///
    /// *C style function: [ncplane_dim_x()][crate::ncplane_dim_x].*
    #[inline]
    pub fn cols(&self) -> NcDim {
        self.dim_yx().1
    }

    /// Resizes this NcPlane.
    ///
    /// The four parameters `keep_y`, `keep_x`, `keep_len_y`, and `keep_len_x`
    /// defines a subset of this NcPlane to keep unchanged. This may be a section
    /// of size 0.
    ///
    /// `keep_x` and `keep_y` are relative to this NcPlane. They must specify a
    /// coordinate within the ncplane's totality. If either of `keep_len_y` or
    /// `keep_len_x` is non-zero, both must be non-zero.
    ///
    /// `y_off` and `x_off` are relative to `keep_y` and `keep_x`, and place the
    /// upper-left corner of the resized NcPlane.
    ///
    /// `y_len` and `x_len` are the dimensions of this NcPlane after resizing.
    /// `y_len` must be greater than or equal to `keep_len_y`,
    /// and `x_len` must be greater than or equal to `keeplenx`.
    ///
    /// It is an error to attempt to resize the standard plane.
    ///
    /// *C style function: [ncplane_resize()][crate::ncplane_resize].*
    pub fn resize(
        &mut self,
        keep_y: NcDim,
        keep_x: NcDim,
        keep_len_y: NcDim,
        keep_len_x: NcDim,
        y_off: NcOffset,
        x_off: NcOffset,
        y_len: NcDim,
        x_len: NcDim,
    ) -> NcResult<()> {
        error![
            unsafe {
                crate::ncplane_resize(
                    self,
                    keep_y as i32,
                    keep_x as i32,
                    keep_len_y as i32,
                    keep_len_x as i32,
                    y_off as i32,
                    x_off as i32,
                    y_len as i32,
                    x_len as i32,
                )
            },
            &format!(
                "NcPlane.resize({}, {}, {}, {}, {}, {}, {}, {})",
                keep_y, keep_x, keep_len_y, keep_len_x, y_off, x_off, y_len, x_len
            )
        ]
    }

    /// Suitable for use as a 'resizecb' with planes created with
    /// [`NCPLANE_OPTION_MARGINALIZED`][crate::NCPLANE_OPTION_MARGINALIZED].
    ///
    /// This will resize this plane against its parent, attempting to enforce
    /// the supplied margins.
    ///
    /// *C style function: [ncplane_resize_marginalized()][crate::ncplane_resize_marginalized].*
    pub fn resize_marginalized(&mut self) -> NcResult<()> {
        error![
            unsafe { crate::ncplane_resize_marginalized(self) },
            "NcPlane.resize_marginalized()"
        ]
    }

    /// Suitable for use as a 'resizecb', this will resize the plane
    /// to the visual region's size. It is used for the standard plane.
    ///
    /// *C style function: [ncplane_resize_maximize()][crate::ncplane_resize_maximize].*
    pub fn resize_maximize(&mut self) -> NcResult<()> {
        error![
            unsafe { crate::ncplane_resize_maximize(self) },
            "NcPlane.resize_maximize()"
        ]
    }

    /// Creates an RGBA flat array from the selected region of the ncplane.
    ///
    /// Starts at the plane's `beg_y`x`beg_x` coordinate (which must lie on the
    /// plane), continuing for `len_y`x`len_x` cells.
    ///
    /// Use `None` for either or both of `len_y` and `len_x` in order to
    /// go through the boundary of the plane in that axis.
    ///
    /// Only glyphs from the specified blitset may be present.
    ///
    /// *C style function: [ncplane_rgba()][crate::ncplane_rgba].*
    pub fn rgba(
        &mut self,
        blitter: NcBlitter,
        beg_y: NcDim,
        beg_x: NcDim,
        len_y: Option<NcDim>,
        len_x: Option<NcDim>,
    ) -> NcResult<&mut [u32]> {
        // converts length arguments to expected format
        let len_y2: i32;
        let len_x2: i32;
        if let Some(y) = len_y {
            len_y2 = y as i32;
        } else {
            len_y2 = -1;
        }
        if let Some(x) = len_x {
            len_x2 = x as i32;
        } else {
            len_x2 = -1;
        }

        let res_array = unsafe {
            crate::ncplane_rgba(self, blitter, beg_y as i32, beg_x as i32, len_y2, len_x2)
        };

        // calculates array length
        let array_len_y;
        let array_len_x;
        if len_y2 == -1 {
            array_len_y = self.dim_y() - beg_y;
        } else {
            array_len_y = len_y2 as u32;
        }
        if len_x2 == -1 {
            array_len_x = self.dim_x() - beg_x;
        } else {
            array_len_x = len_x2 as u32;
        }
        let array_len = (array_len_y * array_len_x) as usize;

        // returns the result
        if res_array != null_mut() {
            return Ok(unsafe { from_raw_parts_mut(res_array, array_len) });
        } else {
            Err(NcError::with_msg(NCRESULT_ERR, "NcPlane.rgba()"))
        }
    }

    /// Realigns this NcPlane against its parent, using the alignment specified
    /// at creation time.
    ///
    /// Suitable for use as an [NcResizeCb].
    ///
    /// *C style function: [ncplane_resize_realign()][crate::ncplane_resize_realign].*
    //
    // TODO: suitable for use as an NcResizeCb?
    pub fn resize_realign(&mut self) -> NcResult<()> {
        error![unsafe { crate::ncplane_resize_realign(self) }]
    }

    /// Resizes this NcPlane, retaining what data we can (everything, unless we're
    /// shrinking in some dimension). Keeps the origin where it is.
    ///
    /// *C style function: [ncplane_resize_simple()][crate::ncplane_resize_simple].*
    #[inline]
    pub fn resize_simple(&mut self, y_len: NcDim, x_len: NcDim) -> NcResult<()> {
        error![crate::ncplane_resize_simple(
            self,
            y_len as u32,
            x_len as u32
        )]
    }

    /// Returns this NcPlane's current resize callback.
    ///
    /// *C style function: [ncplane_resizecb()][crate::ncplane_resizecb].*
    pub fn resizecb(&self) -> Option<NcResizeCb> {
        unsafe { crate::ncresizecb_to_rust(crate::ncplane_resizecb(self)) }
    }

    /// Replaces this NcPlane's existing resize callback (which may be [None]).
    ///
    /// The standard plane's resizecb may not be changed.
    ///
    /// *C style function: [ncplane_set_resizecb()][crate::ncplane_set_resizecb].*
    pub fn set_resizecb(&mut self, resizecb: Option<NcResizeCb>) {
        unsafe { crate::ncplane_set_resizecb(self, crate::ncresizecb_to_c(resizecb)) }
    }

    /// Rotate the plane π/2 radians clockwise.
    ///
    /// This cannot be performed on arbitrary planes, because glyphs cannot be
    /// arbitrarily rotated.
    ///
    /// The glyphs which can be rotated are limited: line-drawing characters,
    /// spaces, half blocks, and full blocks.
    ///
    /// The plane must have an even number of columns.
    ///
    /// Use the ncvisual rotation for a more flexible approach.
    ///
    /// *C style function: [ncplane_rotate_cw()][crate::ncplane_rotate_cw].*
    pub fn rotate_cw(&mut self) -> NcResult<()> {
        error![unsafe { crate::ncplane_rotate_cw(self) }]
    }

    /// Rotate the plane π/2 radians counter-clockwise.
    ///
    /// See [`rotate_cw`][NcPlane#method.rotate_cw]
    /// for more information.
    ///
    /// *C style function: [ncplane_rotate_ccw()][crate::ncplane_rotate_ccw].*
    pub fn rotate_ccw(&mut self) -> NcResult<()> {
        error![unsafe { crate::ncplane_rotate_ccw(self) }]
    }

    /// Maps the provided coordinates relative to the origin of this NcPlane,
    /// to the same absolute coordinates relative to the origin of `target`.
    ///
    /// *C style function: [ncplane_translate()][crate::ncplane_translate].*
    //
    // TODO: API change, return the coordinates as a tuple instead of being &mut
    pub fn translate(&self, target: &NcPlane, y: &mut NcDim, x: &mut NcDim) {
        unsafe { crate::ncplane_translate(self, target, &mut (*y as i32), &mut (*x as i32)) }
    }

    /// Returns true if the provided absolute `y`/`x` coordinates are within
    /// this NcPlane, or false otherwise.
    ///
    /// Either way, translates the absolute coordinates relative to this NcPlane.
    ///
    /// *C style function: [ncplane_translate_abs()][crate::ncplane_translate_abs].*
    //
    // TODO: API change, return a tuple (y,x,bool)
    pub fn translate_abs(&self, y: &mut NcDim, x: &mut NcDim) -> bool {
        unsafe { crate::ncplane_translate_abs(self, &mut (*y as i32), &mut (*x as i32)) }
    }

    /// Gets the `y`, `x` origin of this NcPlane relative to the standard plane,
    /// or the NcPlane to which it is bound.
    ///
    /// *C style function: [ncplane_yx()][crate::ncplane_yx].*
    //
    // CHECK: negative offsets
    pub fn yx(&self) -> (NcOffset, NcOffset) {
        let (mut y, mut x) = (0, 0);
        unsafe { crate::ncplane_yx(self, &mut y, &mut x) };
        (y as NcOffset, x as NcOffset)
    }

    /// Gets the `x` origin of this NcPlane relative to the standard plane,
    /// or the NcPlane to which it is bound.
    ///
    /// *C style function: [ncplane_x()][crate::ncplane_x].*
    pub fn x(&self) -> NcOffset {
        unsafe { crate::ncplane_x(self) as NcOffset }
    }

    /// Gets the `y` origin of this NcPlane relative to the standard plane,
    /// or the NcPlane to which it is bound.
    ///
    /// *C style function: [ncplane_y()][crate::ncplane_y].*
    pub fn y(&self) -> NcOffset {
        unsafe { crate::ncplane_y(self) as NcOffset }
    }

    /// Sets the scrolling behaviour of the plane, and
    /// returns true if scrolling was previously enabled, of false, if disabled.
    ///
    /// All planes are created with scrolling disabled. Attempting to print past
    /// the end of a line will stop at the plane boundary, and indicate an error.
    ///
    /// On a plane 10 columns wide and two rows high, printing "0123456789"
    /// at the origin should succeed, but printing "01234567890" will by default
    /// fail at the eleventh character. In either case, the cursor will be left
    /// at location 0x10; it must be moved before further printing can take place. I
    ///
    /// *C style function: [ncplane_set_scrolling()][crate::ncplane_set_scrolling].*
    pub fn set_scrolling(&mut self, scroll: bool) -> bool {
        unsafe { crate::ncplane_set_scrolling(self, scroll) }
    }
}

// -----------------------------------------------------------------------------
/// ## NcPlane methods: boxes & perimeters
impl NcPlane {
    /// Draws a box with its upper-left corner at the current cursor position,
    /// and its lower-right corner at `y_stop` * `x_stop`.
    ///
    /// The 6 cells provided are used to draw the upper-left, ur, ll, and lr corners,
    /// then the horizontal and vertical lines.
    ///
    /// See [NcBoxMask] for information about the border and gradient masks,
    /// and the drawing of corners.
    ///
    /// If the gradient bit is not set, the styling from the hline/vlline cells
    /// is used for the horizontal and vertical lines, respectively.
    ///
    /// If the gradient bit is set, the color is linearly interpolated between
    /// the two relevant corner cells.
    ///
    /// *C style function: [ncplane_box()][crate::ncplane_box].*
    pub fn r#box(
        &mut self,
        ul: &NcCell,
        ur: &NcCell,
        ll: &NcCell,
        lr: &NcCell,
        hline: &NcCell,
        vline: &NcCell,
        y_stop: NcDim,
        x_stop: NcDim,
        boxmask: NcBoxMask,
    ) -> NcResult<()> {
        error![unsafe {
            crate::ncplane_box(
                self,
                ul,
                ur,
                ll,
                lr,
                hline,
                vline,
                y_stop as i32,
                x_stop as i32,
                boxmask,
            )
        }]
    }

    /// Draws a box with its upper-left corner at the current cursor position,
    /// having dimensions `y_len` * `x_len`.
    /// The minimum box size is 2x2, and it cannot be drawn off-screen.
    ///
    /// See the [`box`][NcPlane#method.box] method for more information.
    ///
    /// *C style function: [ncplane_box_sized()][crate::ncplane_box_sized].*
    #[inline]
    pub fn box_sized(
        &mut self,
        ul: &NcCell,
        ur: &NcCell,
        ll: &NcCell,
        lr: &NcCell,
        hline: &NcCell,
        vline: &NcCell,
        y_len: NcDim,
        x_len: NcDim,
        boxmask: NcBoxMask,
    ) -> NcResult<()> {
        error![crate::ncplane_box_sized(
            self, ul, ur, ll, lr, hline, vline, y_len, x_len, boxmask
        )]
    }

    /// NcPlane.[box()][NcPlane#method.box] with the double box-drawing characters.
    ///
    /// *C style function: [ncplane_double_box()][crate::ncplane_double_box].*
    #[inline]
    pub fn double_box(
        &mut self,
        stylemask: NcStyleMask,
        channels: NcChannelPair,
        y_stop: NcDim,
        x_stop: NcDim,
        boxmask: NcBoxMask,
    ) -> NcResult<()> {
        error![crate::ncplane_double_box(
            self, stylemask, channels, y_stop, x_stop, boxmask
        )]
    }

    ///
    ///
    /// *C style function: [ncplane_double_box_sized()][crate::ncplane_double_box_sized].*
    #[inline]
    pub fn double_box_sized(
        &mut self,
        stylemask: NcStyleMask,
        channels: NcChannelPair,
        y_len: NcDim,
        x_len: NcDim,
        boxmask: NcBoxMask,
    ) -> NcResult<()> {
        error![crate::ncplane_double_box(
            self, stylemask, channels, y_len, x_len, boxmask
        )]
    }

    /// Draws the perimeter around this NcPlane.
    ///
    /// *C style function: [ncplane_perimeter()][crate::ncplane_perimeter].*
    #[inline]
    pub fn perimeter(
        &mut self,
        ul: &NcCell,
        ur: &NcCell,
        ll: &NcCell,
        lr: &NcCell,
        hline: &NcCell,
        vline: &NcCell,
        boxmask: NcBoxMask,
    ) -> NcResult<()> {
        error![crate::ncplane_perimeter(
            self, ul, ur, ll, lr, hline, vline, boxmask
        )]
    }

    /// NcPlane.[perimeter()][NcPlane#method.perimeter] with the double box-drawing characters.

    ///
    /// *C style function: [ncplane_perimeter_double()][crate::ncplane_perimeter_double].*
    #[inline]
    pub fn perimeter_double(
        &mut self,
        stylemask: NcStyleMask,
        channels: NcChannelPair,
        boxmask: NcBoxMask,
    ) -> NcResult<()> {
        error![crate::ncplane_perimeter_double(
            self, stylemask, channels, boxmask
        )]
    }

    /// NcPlane.[perimeter()][NcPlane#method.perimeter] with the rounded box-drawing characters.
    ///
    ///
    /// *C style function: [ncplane_perimeter_rounded()][crate::ncplane_perimeter_rounded].*
    #[inline]
    pub fn perimeter_rounded(
        &mut self,
        stylemask: NcStyleMask,
        channels: NcChannelPair,
        boxmask: NcBoxMask,
    ) -> NcResult<()> {
        error![crate::ncplane_perimeter_rounded(
            self, stylemask, channels, boxmask
        )]
    }
}

// -----------------------------------------------------------------------------
/// ## NcPlane methods: fading, gradients & greyscale
impl NcPlane {
    /// Fades this NcPlane in, over the specified time, calling 'fader' at
    /// each iteration.
    ///
    /// Usage:
    /// 1. Load this NcPlane with the target cells without rendering.
    /// 2. call this function.
    ///
    /// When it's done, the NcPlane will have reached the target levels,
    /// starting from zeroes.
    ///
    /// *C style function: [ncplane_fadein()][crate::ncplane_fadein].*
    pub fn fadein(&mut self, time: &NcTime, fader: NcFadeCb) -> NcResult<()> {
        error![unsafe { crate::ncplane_fadein(self, time, fader, null_mut()) }]
    }

    /// Fades in through 'iter' iterations,
    /// where 'iter' < 'ncfadectx_iterations(nctx)'.
    ///
    /// *C style function: [ncplane_fadein_iteration()][crate::ncplane_fadein_iteration].*
    pub fn fadein_iteration(&mut self, time: &NcTime, fader: NcFadeCb) -> NcResult<()> {
        error![unsafe { crate::ncplane_fadein(self, time, fader, null_mut()) }]
    }

    /// Fades this NcPlane out, over the specified time, calling 'fader' at
    /// each iteration.
    ///
    /// Requires a terminal which supports truecolor, or at least palette
    /// modification (if the terminal uses a palette, our ability to fade planes
    /// is limited, and affected by the complexity of the rest of the screen).
    ///
    /// *C style function: [ncplane_fadeout()][crate::ncplane_fadeout].*
    pub fn fadeout(&mut self, time: &NcTime, fader: NcFadeCb) -> NcResult<()> {
        error![unsafe { crate::ncplane_fadeout(self, time, fader, null_mut()) }]
    }

    /// Fades out through 'iter' iterations,
    /// where 'iter' < 'ncfadectx_iterations(nctx)'.
    ///
    /// *C style function: [ncplane_fadeout_iteration()][crate::ncplane_fadeout_iteration].*
    pub fn fadeout_iteration(&mut self, time: &NcTime, fader: NcFadeCb) -> NcResult<()> {
        error![unsafe { crate::ncplane_fadeout(self, time, fader, null_mut()) }]
    }

    /// Pulses this NcPlane in and out until the callback returns non-zero,
    /// relying on the callback 'fader' to initiate rendering.
    ///
    /// `time` defines the half-period (i.e. the transition from black to full
    /// brightness, or back again).
    ///
    /// Proper use involves preparing (but not rendering) the NcPlane,
    /// then calling this method, which will fade in from black to the
    /// specified colors.
    ///
    /// *C style function: [ncplane_pulse()][crate::ncplane_pulse].*
    pub fn pulse(&mut self, time: &NcTime, fader: NcFadeCb) -> NcResult<()> {
        error![unsafe { crate::ncplane_pulse(self, time, fader, null_mut()) }]
    }

    /// Draws a gradient with its upper-left corner at the current cursor
    /// position, stopping at `y_stop` * `xstop`.
    ///
    /// Returns the number of cells filled on success,
    /// or [NCRESULT_ERR] on error.
    ///
    /// The glyph composed of `egc` and `stylemask` is used for all cells.
    /// The channels specified by `ul`, `ur`, `ll`, and `lr` are composed into
    /// foreground and background gradients.
    ///
    /// To do a vertical gradient, `ul` ought equal `ur` and `ll` ought equal
    /// `lr`. To do a horizontal gradient, `ul` ought equal `ll` and `ur` ought
    /// equal `ul`.
    ///
    /// To color everything the same, all four channels should be equivalent.
    /// The resulting alpha values are equal to incoming alpha values.
    ///
    /// Palette-indexed color is not supported.
    ///
    /// Preconditions for gradient operations (error otherwise):
    ///
    /// all: only RGB colors, unless all four channels match as default
    /// all: all alpha values must be the same
    /// 1x1: all four colors must be the same
    /// 1xN: both top and both bottom colors must be the same (vertical gradient)
    /// Nx1: both left and both right colors must be the same (horizontal gradient)
    ///
    /// *C style function: [ncplane_gradient()][crate::ncplane_gradient].*
    pub fn gradient(
        &mut self,
        egc: &NcEgc,
        stylemask: NcStyleMask,
        ul: NcChannelPair,
        ur: NcChannelPair,
        ll: NcChannelPair,
        lr: NcChannelPair,
        y_stop: NcDim,
        x_stop: NcDim,
    ) -> NcResult<NcDim> {
        let res = unsafe {
            crate::ncplane_gradient(
                self,
                &(*egc as i8),
                stylemask as u32,
                ul,
                ur,
                ll,
                lr,
                y_stop as i32,
                x_stop as i32,
            )
        };
        error![res, "", res as NcDim]
    }

    /// Draw a gradient with its upper-left corner at the current cursor position,
    /// having dimensions `y_len` * `x_len`.
    ///
    /// See [gradient][NcPlane#method.gradient] for more information.
    ///
    /// *C style function: [ncplane_gradient_sized()][crate::ncplane_gradient_sized].*
    #[inline]
    pub fn gradient_sized(
        &mut self,
        egc: &NcEgc,
        stylemask: NcStyleMask,
        ul: NcChannel,
        ur: NcChannel,
        ll: NcChannel,
        lr: NcChannel,
        y_len: NcDim,
        x_len: NcDim,
    ) -> NcResult<NcDim> {
        let res = crate::ncplane_gradient_sized(self, egc, stylemask, ul, ur, ll, lr, y_len, x_len);
        error![res, "", res as NcDim]
    }

    /// Draws a high-resolution gradient using upper blocks and synced backgrounds.
    ///
    /// Returns the number of cells filled on success,
    /// or [NCRESULT_ERR] on error.
    ///
    /// This doubles the number of vertical gradations, but restricts you to
    /// half blocks (appearing to be full blocks).
    ///
    /// *C style function: [ncplane_highgradient()][crate::ncplane_highgradient].*
    pub fn highgradient(
        &mut self,
        ul: NcChannel,
        ur: NcChannel,
        ll: NcChannel,
        lr: NcChannel,
        y_stop: NcDim,
        x_stop: NcDim,
    ) -> NcResult<NcDim> {
        let res = unsafe {
            crate::ncplane_highgradient(self, ul, ur, ll, lr, y_stop as i32, x_stop as i32)
        };
        error![res, "", res as NcDim]
    }

    /// [`gradient_sized`][NcPlane#method.gradient_sized]
    /// meets [`highgradient`][NcPlane#method.highgradient].
    ///
    /// *C style function: [ncplane_highgradient_sized()][crate::ncplane_highgradient_sized].*
    pub fn highgradient_sized(
        &mut self,
        ul: NcChannel,
        ur: NcChannel,
        ll: NcChannel,
        lr: NcChannel,
        y_stop: NcDim,
        x_stop: NcDim,
    ) -> NcResult<NcDim> {
        let res = unsafe {
            crate::ncplane_highgradient_sized(self, ul, ur, ll, lr, y_stop as i32, x_stop as i32)
        };
        error![res, "", res as NcDim]
    }

    /// Converts this NcPlane's content to greyscale.
    ///
    /// *C style function: [ncplane_greyscale()][crate::ncplane_greyscale].*
    pub fn greyscale(&mut self) {
        unsafe {
            crate::ncplane_greyscale(self);
        }
    }
}
