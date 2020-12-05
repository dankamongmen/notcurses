//! `NcPlane*` methods and associated functions.

use core::ptr::{null, null_mut};

use crate::{cstring, NcAlign, NcCell, NcPlane, NcPlaneOptions, NcResult, Notcurses};

/// # `NcPlaneOptions` Constructors
impl NcPlaneOptions {
    /// New NcPlaneOptions using the horizontal x.
    pub fn new(y: i32, x: i32, rows: u32, cols: u32) -> Self {
        Self::with_flags(y, x, rows, cols, 0)
    }

    /// New NcPlaneOptions with horizontal alignment.
    pub fn new_halign(y: i32, align: NcAlign, rows: u32, cols: u32) -> Self {
        Self::with_flags(
            y,
            align as i32,
            rows,
            cols,
            crate::NCPLANE_OPTION_HORALIGNED,
        )
    }

    /// New NcPlaneOptions, with flags.
    ///
    /// Note: If you use [NCPLANE_OPTION_HORALIGNED] flag, you must provide
    /// the [NcAlign] value to the `x` parameter, casted to `i32`.
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

/// # `NcPlane` Constructors
impl NcPlane {
    /// New NcPlane.
    ///
    /// The returned plane will be the top, bottom, and root of this new pile.
    pub fn new<'a>(nc: &mut Notcurses, y: i32, x: i32, rows: u32, cols: u32) -> &'a mut NcPlane {
        let options = NcPlaneOptions::new(y, x, rows, cols);
        unsafe { &mut *crate::ncpile_create(nc, &options) }
    }

    /// New NcPlane, expects an [NcPlaneOptions] struct.
    ///
    /// The returned plane will be the top, bottom, and root of this new pile.
    pub fn with_options<'a>(nc: &mut Notcurses, options: &NcPlaneOptions) -> &'a mut NcPlane {
        unsafe { &mut *crate::ncpile_create(nc, options) }
    }

    /// New NcPlane, bound to another NcPlane.
    pub fn new_bound<'a>(
        bound_to: &mut NcPlane,
        y: i32,
        x: i32,
        rows: u32,
        cols: u32,
    ) -> &'a mut NcPlane {
        let options = NcPlaneOptions::new(y, x, rows, cols);
        unsafe { &mut *crate::ncplane_create(bound_to, &options) }
    }

    /// New NcPlane, bound to another plane, expects an [NcPlaneOptions] struct.
    ///
    /// The returned plane will be the top, bottom, and root of this new pile.
    pub fn with_options_bound<'a>(nc: &mut Notcurses, options: &NcPlaneOptions) -> &'a mut NcPlane {
        unsafe { &mut *crate::ncpile_create(nc, options) }
    }

    /// New NcPlane, with the same dimensions of the terminal.
    ///
    /// The returned plane will be the top, bottom, and root of this new pile.
    pub fn new_termsize<'a>(nc: &mut Notcurses) -> &'a mut NcPlane {
        let (mut trows, mut tcols) = (0, 0);
        crate::notcurses_term_dim_yx(nc, &mut trows, &mut tcols);
        assert![(trows > 0) & (tcols > 0)];
        unsafe {
            &mut *crate::ncpile_create(nc, &NcPlaneOptions::new(0, 0, trows as u32, tcols as u32))
        }
    }
}

/// # `NcPlane` Methods
impl NcPlane {
    // Cursor ------------------------------------------------------------------

    /// Returns the current position of the cursor within the [NcPlane].
    ///
    /// Unlike [ncplane_cursor_yx] which uses `i32`, this uses [u32].
    //
    // NOTE: y and/or x may be NULL.
    // FIXME: CHECK for NULL and return Some() or None.
    pub fn cursor_yx(&self) -> (i32, i32) {
        let (mut y, mut x) = (0, 0);
        unsafe { crate::ncplane_cursor_yx(self, &mut y, &mut x) };
        (y, x)
    }

    /// Returns the current row of the cursor within the [NcPlane].
    pub fn cursor_y(&self) -> i32 {
        self.cursor_yx().0
    }

    /// Returns the current column of the cursor within the [NcPlane].
    pub fn cursor_x(&self) -> i32 {
        self.cursor_yx().1
    }

    // Size --------------------------------------------------------------------

    /// Return the dimensions of this [NcPlane].
    ///
    /// Unlike [ncplane_dim_yx] which uses `i32`, this uses [u32].
    pub fn dim_yx(&self) -> (u32, u32) {
        let (mut y, mut x) = (0, 0);
        unsafe { crate::ncplane_dim_yx(self, &mut y, &mut x) };
        (y as u32, x as u32)
    }

    /// Return the rows of this [NcPlane].
    pub fn dim_y(&self) -> u32 {
        self.dim_yx().0
    }

    /// Return the columns of this [NcPlane].
    pub fn dim_x(&self) -> u32 {
        self.dim_yx().1
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
    pub fn set_scrolling(&mut self, scroll: bool) -> bool {
        unsafe { crate::ncplane_set_scrolling(self, scroll) }
    }

    // TODO: resize

    // Write -------------------------------------------------------------------

    /// Erases every NcCell in the NcPlane, resetting all attributes to normal,
    /// all colors to the default color, and all cells to undrawn.
    ///
    /// All cells associated with this NcPlane are invalidated, and must not be
    /// used after the call, excluding the base cell. The cursor is homed.
    pub fn erase(&mut self) {
        unsafe { crate::ncplane_erase(self) }
    }

    /// Replaces the NcCell at the specified coordinates with the provided NcCell,
    /// advancing the cursor by its width (but not past the end of the plane).
    ///
    /// The new NcCell must already be associated with the Plane.
    /// On success, returns the number of columns the cursor was advanced.
    /// On failure, -1 is returned.
    pub fn putc_yx(&mut self, y: i32, x: i32, cell: &NcCell) -> NcResult {
        unsafe { crate::ncplane_putc_yx(self, y, x, cell) }
    }

    /// Replaces the NcCell at the current coordinates with the provided NcCell,
    /// advancing the cursor by its width (but not past the end of the plane).
    ///
    /// The new NcCell must already be associated with the Plane.
    /// On success, returns the number of columns the cursor was advanced.
    /// On failure, -1 is returned.
    pub fn putc(&mut self, cell: &NcCell) -> NcResult {
        crate::ncplane_putc(self, cell)
    }

    /// Writes a series of [NcEgc]s to the current location, using the current style.
    pub fn putstr(&mut self, string: &str) -> NcResult {
        crate::ncplane_putstr(self, string)
    }

    // TODO: Stained Replace a string's worth of glyphs at the current cursor location, but retain the styling. The current styling of the plane will not be changed

    /// Write a string, which is a series of [NcEgc]s, to the current location,
    /// using the current style.
    ///
    /// They will be interpreted as a series of columns (according to the
    /// definition of `ncplane_putc()`).
    ///
    /// Advances the cursor by some positive number of columns (though not
    /// beyond the end of the plane); this number is returned on success.
    ///
    /// On error, a non-positive number is returned, indicating the number of
    /// columns which were written before the error.
    pub fn putstr_yx(&mut self, y: i32, x: i32, string: &str) -> NcResult {
        unsafe { crate::ncplane_putstr_yx(self, y, x, cstring![string]) }
    }

    // Pile --------------------------------------------------------------------

    /// Returns the bottommost [NcPlane] of the pile that contains this [NnPlane].
    pub fn bottom<'a>(&mut self) -> &'a mut NcPlane {
        unsafe { &mut *crate::ncpile_bottom(self) }
    }

    /// Returns the topmost [NcPlane] of the pile that contains this [NnPlane].
    pub fn top<'a>(&mut self) -> &'a mut NcPlane {
        unsafe { &mut *crate::ncpile_top(self) }
    }
}
