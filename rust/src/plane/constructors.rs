//! Handy [`NcPlane`] and [`NcPlaneOptions`] constructors

use core::ptr::{null, null_mut};

use crate::{
    ncpile_create, ncplane_create, notcurses_term_dim_yx, NcAlign, NcPlane, NcPlaneOptions,
    Notcurses, NCPLANE_OPTION_HORALIGNED,
};

impl NcPlaneOptions {
    /// [`NcPlaneOptions`] simple constructor with horizontal x
    pub fn new(y: i32, x: i32, rows: u32, cols: u32) -> Self {
        Self::with_flags(y, x, rows, cols, 0)
    }

    /// [`NcPlaneOptions`] simple constructor with horizontal alignment
    pub fn new_halign(y: i32, align: NcAlign, rows: u32, cols: u32) -> Self {
        Self::with_flags(y, align as i32, rows, cols, NCPLANE_OPTION_HORALIGNED)
    }

    /// [`NcPlaneOptions`] constructor
    ///
    /// Note: If you use`NCPLANE_OPTION_HORALIGNED` flag, you must provide
    /// the `NcAlign` value as the `x` parameter, casted to `i32`.
    pub fn with_flags(y: i32, x: i32, rows: u32, cols: u32, flags: u64) -> Self {
        NcPlaneOptions {
            y,
            x,
            rows: rows as i32,
            cols: cols as i32,
            userptr: null_mut(),
            name: null(),
            resizecb: None,
            flags,
        }
    }
}

impl NcPlane {
    /// [`NcPlane`] constructor
    ///
    /// The returned plane will be the top, bottom, and root of this new pile.
    pub unsafe fn new<'a>(
        nc: &mut Notcurses,
        y: i32,
        x: i32,
        rows: u32,
        cols: u32,
    ) -> &'a mut NcPlane {
        let options = NcPlaneOptions::new(y, x, rows, cols);
        &mut *ncpile_create(nc, &options)
    }

    /// [`NcPlane`] constructor, expecting an [`NcPlaneOptions`] struct
    ///
    /// The returned plane will be the top, bottom, and root of this new pile.
    pub unsafe fn with_options<'a>(
        nc: &mut Notcurses,
        options: &NcPlaneOptions,
    ) -> &'a mut NcPlane {
        &mut *ncpile_create(nc, options)
    }

    /// [`NcPlane`] constructor, bound to another plane
    pub unsafe fn new_bound<'a>(
        bound_to: &mut NcPlane,
        y: i32,
        x: i32,
        rows: u32,
        cols: u32,
    ) -> &'a mut NcPlane {
        let options = NcPlaneOptions::new(y, x, rows, cols);
        &mut *ncplane_create(bound_to, &options)
    }

    /// [`NcPlane`] constructor, bound to another plane,
    /// expecting an [`NcPlaneOptions`] struct
    ///
    /// The returned plane will be the top, bottom, and root of this new pile.
    pub unsafe fn with_options_bound<'a>(
        nc: &mut Notcurses,
        options: &NcPlaneOptions,
    ) -> &'a mut NcPlane {
        &mut *ncpile_create(nc, options)
    }

    /// [`NcPlane`] constructor, with the same dimensions of the terminal.
    ///
    /// The returned plane will be the top, bottom, and root of this new pile.
    // FIXME BUG
    pub unsafe fn new_termsize<'a>(nc: &mut Notcurses) -> &'a mut NcPlane {
        let (mut trows, mut tcols) = (0, 0);
        notcurses_term_dim_yx(nc, &mut trows, &mut tcols);
        assert![(trows > 0) & (tcols > 0)];
        &mut *ncpile_create(nc, &NcPlaneOptions::new(0, 0, trows as u32, tcols as u32))
    }
}
