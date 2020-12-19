//! `NcPlane*` methods and associated functions.

use core::ptr::{null, null_mut};
use std::ffi::CStr;

use crate::{
    cstring, NcAlign, NcBoxMask, NcCell, NcChannelPair, NcDimension, NcEgc, NcOffset, NcPlane,
    NcPlaneOptions, NcResult, NcStyleMask, Notcurses,
};

/// # `NcPlaneOptions` Constructors
impl NcPlaneOptions {
    /// New NcPlaneOptions using the horizontal x.
    pub fn new(y: NcOffset, x: NcOffset, rows: NcDimension, cols: NcDimension) -> Self {
        Self::with_flags(y, x, rows, cols, 0)
    }

    /// New NcPlaneOptions with horizontal alignment.
    pub fn new_aligned(y: NcOffset, align: NcAlign, rows: NcDimension, cols: NcDimension) -> Self {
        Self::with_flags_aligned(y, align, rows, cols, crate::NCPLANE_OPTION_HORALIGNED)
    }

    /// New NcPlaneOptions, with flags.
    pub fn with_flags(
        y: NcOffset,
        x: NcOffset,
        rows: NcDimension,
        cols: NcDimension,
        flags: u64,
    ) -> Self {
        NcPlaneOptions {
            y: y as i32,
            x: x as i32,
            rows: rows as i32,
            cols: cols as i32,
            userptr: null_mut(),
            name: null(),
            resizecb: None,
            flags,
        }
    }

    /// New NcPlaneOptions, with flags and horizontal alignment.
    ///
    /// Note: Already includes the
    /// [NCPLANE_OPTION_HORALIGNED][crate::NCPLANE_OPTION_HORALIGNED] flag.
    pub fn with_flags_aligned(
        y: NcOffset,
        align: NcAlign,
        rows: NcDimension,
        cols: NcDimension,
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
            resizecb: None,
            flags,
        }
    }
}

/// # `NcPlane` constructors and destructors
impl NcPlane {
    /// New NcPlane.
    ///
    /// The returned plane will be the top, bottom, and root of this new pile.
    pub fn new<'a>(
        nc: &mut Notcurses,
        y: NcOffset,
        x: NcOffset,
        rows: NcDimension,
        cols: NcDimension,
    ) -> &'a mut NcPlane {
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
        y: NcOffset,
        x: NcOffset,
        rows: NcDimension,
        cols: NcDimension,
    ) -> &'a mut NcPlane {
        let options = NcPlaneOptions::new(y, x, rows, cols);
        unsafe { &mut *crate::ncplane_create(bound_to, &options) }
    }

    /// New NcPlane, bound to another plane, expects an [NcPlaneOptions] struct.
    ///
    /// The returned plane will be the top, bottom, and root of this new pile.
    pub fn with_options_bound<'a>(
        bound_to: &mut NcPlane,
        options: &NcPlaneOptions,
    ) -> &'a mut NcPlane {
        unsafe { &mut *crate::ncplane_create(bound_to, options) }
    }

    /// New NcPlane, with the same dimensions of the terminal.
    ///
    /// The returned plane will be the top, bottom, and root of this new pile.
    pub fn new_termsize<'a>(nc: &mut Notcurses) -> &'a mut NcPlane {
        let (mut trows, mut tcols) = (0, 0);
        crate::notcurses_term_dim_yx(nc, &mut trows, &mut tcols);
        assert![(trows > 0) & (tcols > 0)];
        unsafe {
            &mut *crate::ncpile_create(
                nc,
                &NcPlaneOptions::new(0, 0, trows as NcDimension, tcols as NcDimension),
            )
        }
    }

    /// Destroys the NcPlane.
    /// None of its contents will be visible after the next render call.
    /// It is an error to attempt to destroy the standard plane.
    pub fn destroy(&mut self) -> NcResult {
        unsafe { crate::ncplane_destroy(self) }
    }
}

/// # `NcPlane` Methods
impl NcPlane {
    // Cursor ------------------------------------------------------------------

    /// Returns the current position of the cursor within this NcPlane.
    ///
    // NOTE: y and/or x may be NULL.
    // maybe check for null and return Some() or None?
    pub fn cursor_yx(&self) -> (NcDimension, NcDimension) {
        let (mut y, mut x) = (0, 0);
        unsafe { crate::ncplane_cursor_yx(self, &mut y, &mut x) };
        (y as NcDimension, x as NcDimension)
    }

    /// Returns the current row of the cursor within this NcPlane.
    pub fn cursor_y(&self) -> NcDimension {
        self.cursor_yx().0
    }

    /// Returns the current column of the cursor within this NcPlane.
    pub fn cursor_x(&self) -> NcDimension {
        self.cursor_yx().1
    }

    /// Moves the cursor to the specified position within the NcPlane.
    ///
    /// The cursor doesn't need to be visible.
    ///
    /// Parameters exceeding the plane's dimensions will result in an error,
    /// and the cursor position will remain unchanged.
    pub fn cursor_move_yx(&mut self, y: NcDimension, x: NcDimension) -> NcResult {
        unsafe { crate::ncplane_cursor_move_yx(self, y as i32, x as i32) }
    }

    /// Moves the cursor the number of rows specified (forward or backwards).
    ///
    /// It will error if the target row exceeds the plane dimensions.
    pub fn cursor_move_rows(&mut self, rows: NcOffset) -> NcResult {
        let (y, x) = self.cursor_yx();
        self.cursor_move_yx((y as NcOffset + rows) as NcDimension, x)
    }

    /// Moves the cursor the number of columns specified (forward or backwards).
    ///
    /// It will error if the target column exceeds the plane dimensions.
    // TODO: maybe in this case it can improve
    pub fn cursor_move_cols(&mut self, cols: NcOffset) -> NcResult {
        let (y, x) = self.cursor_yx();
        self.cursor_move_yx(y, (x as NcOffset + cols) as NcDimension)
    }

    /// Moves the cursor to 0, 0.
    pub fn cursor_home(&mut self) {
        unsafe {
            crate::ncplane_home(self);
        }
    }

    // Size & alignment --------------------------------------------------------

    /// Returns the column at which `cols` columns ought start in order to be
    /// aligned according to `align` within this NcPlane.
    /// Returns INT_MAX on invalid `align`.
    pub fn align(&mut self, align: NcAlign, cols: NcDimension) -> NcResult {
        crate::ncplane_align(self, align, cols)
    }

    /// Return the dimensions of this NcPlane.
    ///
    /// Unlike [ncplane_dim_yx][crate::ncplane_dim_yx] which uses `i32`,
    /// this uses [u32].
    pub fn dim_yx(&self) -> (NcDimension, NcDimension) {
        let (mut y, mut x) = (0, 0);
        unsafe { crate::ncplane_dim_yx(self, &mut y, &mut x) };
        (y as NcDimension, x as NcDimension)
    }

    /// Return the rows of this NcPlane.
    pub fn dim_y(&self) -> NcDimension {
        self.dim_yx().0
    }

    /// Return the columns of this NcPlane.
    pub fn dim_x(&self) -> NcDimension {
        self.dim_yx().1
    }

    /// Return the rows of this NcPlane.
    pub fn rows(&self) -> NcDimension {
        self.dim_yx().0
    }

    /// Return the cols of this NcPlane.
    pub fn cols(&self) -> NcDimension {
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

    /// Resizes the NcPlane.
    ///
    /// The four parameters `keep_y`, `keep_x`, `keep_len_y`, and `keep_len_x`
    /// defines a subset of the NcPlane to keep unchanged. This may be a section
    /// of size 0.
    ///
    /// `keep_x` and `keep_y` are relative to the NcPlane. They must specify a
    /// coordinate within the ncplane's totality. If either of `keep_len_y` or
    /// `keep_len_x` is non-zero, both must be non-zero.
    ///
    /// `y_off` and `x_off` are relative to `keep_y` and `keep_x`, and place the
    /// upper-left corner of the resized NcPlane.
    ///
    /// `y_len` and `x_len` are the dimensions of the NcPlane after resizing.
    /// `y_len` must be greater than or equal to `keep_len_y`,
    /// and `x_len` must be greater than or equal to `keeplenx`.
    ///
    /// It is an error to attempt to resize the standard plane.
    pub fn resize(
        &mut self,
        keep_y: NcDimension,
        keep_x: NcDimension,
        keep_len_y: NcDimension,
        keep_len_x: NcDimension,
        y_off: NcOffset,
        x_off: NcOffset,
        y_len: NcDimension,
        x_len: NcDimension,
    ) -> NcResult {
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
        }
    }

    /// Realigns this NcPlane against its parent, using the alignment specified
    /// at creation time. Suitable for use as a `resizecb`.
    pub fn resize_realign(&mut self) -> NcResult {
        unsafe { crate::ncplane_resize_realign(self) }
    }

    /// Resizes the NcPlane, retaining what data we can (everything, unless we're
    /// shrinking in some dimension). Keeps the origin where it is.
    pub fn resize_simple(&mut self, y_len: NcDimension, x_len: NcDimension) -> NcResult {
        crate::ncplane_resize_simple(self, y_len as u32, x_len as u32)
    }

    /// Returns the NcPlane's current resize callback.
    pub fn resizecb(&self) -> Option<unsafe extern "C" fn(*mut NcPlane) -> NcResult> {
        unsafe { crate::ncplane_resizecb(self) }
    }

    // Read -------------------------------------------------------------------

    /// Retrieves the current contents of the [NcCell] under the cursor,
    /// returning the [NcEgc] and writing out the [NcStyleMask] and the [NcChannelPair].
    ///
    /// This NcEgc must be freed by the caller.
    pub fn at_cursor(
        &mut self,
        stylemask: &mut NcStyleMask,
        channels: &mut NcChannelPair,
    ) -> Option<NcEgc> {
        let egc = unsafe { crate::ncplane_at_cursor(self, stylemask, channels) };
        if egc.is_null() {
            return None;
        }
        let egc = core::char::from_u32(unsafe { *egc } as u32).expect("wrong char");
        Some(egc)
    }

    /// Retrieves the current contents of the [NcCell] under the cursor.
    ///
    /// This NcCell is invalidated if the associated NcPlane is destroyed.
    pub fn at_cursor_cell(&mut self, cell: &mut NcCell) -> NcResult {
        crate::ncplane_at_cursor_cell(self, cell)
    }

    /// Retrieves the current contents of the specified [NcCell], returning the
    /// [NcEgc] and writing out the [NcStyleMask] and the [NcChannelPair].
    ///
    /// This NcEgc must be freed by the caller.
    pub fn at_yx(
        &mut self,
        y: i32,
        x: i32,
        stylemask: &mut NcStyleMask,
        channels: &mut NcChannelPair,
    ) -> Option<NcEgc> {
        let egc = unsafe { crate::ncplane_at_yx(self, y, x, stylemask, channels) };
        if egc.is_null() {
            return None;
        }
        let egc = core::char::from_u32(unsafe { *egc } as u32).expect("wrong char");
        Some(egc)
    }

    /// Extracts this NcPlane's base [NcCell] into `cell`.
    ///
    /// The reference is invalidated if the NcPlane is destroyed.
    pub fn base(&mut self, cell: &mut NcCell) -> NcResult {
        unsafe { crate::ncplane_base(self, cell) }
    }

    /// Gets the current ChannelPair for this NcPlane.
    pub fn channels(&self) -> NcChannelPair {
        unsafe { crate::ncplane_channels(self) }
    }

    /// Creates a flat string from the NcEgc's of the selected region of the
    /// NcPlane.
    ///
    /// Starts at the plane's `beg_y` * `beg_x` coordinates (which must lie on
    /// the plane), continuing for `len_y` x `len_x` cells.
    ///
    /// If either of `through_y` or `through_x` are true, then `len_y` or `len_x`,
    /// will ignored respectively, and will go through the boundary of the plane.
    pub fn contents(
        &self,
        beg_y: NcDimension,
        beg_x: NcDimension,
        len_y: NcDimension,
        len_x: NcDimension,
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
        unsafe {
            CStr::from_ptr(crate::ncplane_contents(
                self,
                beg_y as i32,
                beg_x as i32,
                len_y,
                len_x,
            ))
            .to_string_lossy()
            .into_owned()
        }
    }

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
    pub fn putc_yx(&mut self, y: NcDimension, x: NcDimension, cell: &NcCell) -> NcResult {
        unsafe { crate::ncplane_putc_yx(self, y as i32, x as i32, cell) }
    }

    /// Replaces the NcCell at the current coordinates with the provided NcCell,
    /// advancing the cursor by its width (but not past the end of the plane).
    ///
    /// The new NcCell must already be associated with the Plane.
    /// On success, returns the number of columns the cursor was advanced.
    pub fn putc(&mut self, cell: &NcCell) -> NcResult {
        crate::ncplane_putc(self, cell)
    }

    /// Calls [putchar_yx](type.NcPlane.html#method.putchar_yx) at the current cursor location.
    pub fn putchar(&mut self, ch: char) -> NcResult {
        crate::ncplane_putchar(self, ch)
    }

    // TODO: call put_egc
    // /// Replaces the [NcEgc][crate::NcEgc] to the current location, but retain
    // /// the styling. The current styling of the plane will not be changed.
    // pub fn putchar_stained(&mut self, y: NcDimension, x: NcDimension, ch: char) -> NcResult {
    //     crate::ncplane_putchar_stained(self, ch)
    // }

    /// Replaces the [NcEgc][crate::NcEgc], but retain the styling.
    /// The current styling of the plane will not be changed.
    pub fn putchar_yx(&mut self, y: NcDimension, x: NcDimension, ch: char) -> NcResult {
        crate::ncplane_putchar_yx(self, y, x, ch)
    }

    /// Writes a series of [NcEgc][crate::NcEgc]s to the current location,
    /// using the current style.
    ///
    /// Advances the cursor by some positive number of columns (though not beyond
    /// the end of the plane); this number is returned on success.
    ///
    /// On error, a non-positive number is returned, indicating the number of
    /// columns which were written before the error.
    pub fn putstr(&mut self, string: &str) -> NcResult {
        crate::ncplane_putstr(self, string)
    }

    /// Writes a series of [NcEgc][crate::NcEgc]s to the current location, but
    /// retain the styling.
    /// The current styling of the plane will not be changed.
    pub fn putstr_stained(&mut self, string: &str) -> NcResult {
        unsafe { crate::ncplane_putstr_stained(self, cstring![string]) }
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
    pub fn putstr_yx(&mut self, y: NcDimension, x: NcDimension, string: &str) -> NcResult {
        unsafe { crate::ncplane_putstr_yx(self, y as i32, x as i32, cstring![string]) }
    }

    // Pile --------------------------------------------------------------------

    // CHECK:

    /// Returns the bottommost NcPlane of the current pile.
    pub fn bottom<'a>(&mut self) -> &'a mut NcPlane {
        unsafe { &mut *crate::ncpile_bottom(self) }
    }

    /// Returns the topmost NcPlane of the current pile.
    pub fn top<'a>(&mut self) -> &'a mut NcPlane {
        unsafe { &mut *crate::ncpile_top(self) }
    }

    /// Makes the physical screen match the last rendered frame from the pile of
    /// which this NcPlane is a part.
    ///
    /// This is a blocking call. Don't call this before the pile has been
    /// rendered (doing so will likely result in a blank screen).
    pub fn rasterize<'a>(&mut self) -> NcResult {
        unsafe { crate::ncpile_rasterize(self) }
    }

    /// Renders the pile of which this NcPlane is a part.
    /// Rendering this pile again will blow away the render.
    /// To actually write out the render, call ncpile_rasterize().
    pub fn render<'a>(&mut self) -> NcResult {
        unsafe { crate::ncpile_render(self) }
    }

    // Plane -------------------------------------------------------------------

    // move_above
    // move_below
    // move_bottom
    // move_top

    /// Duplicates this NcPlane.
    /// The new plane will have the same geometry, will duplicate all content,
    /// and will start with the same rendering state.
    ///
    /// The new plane will be immediately above the old one on the z axis,
    /// and will be bound to the same parent. Bound planes are not duplicated;
    /// the new plane is bound to the current parent, but has no bound planes.
    // TODO: deal with the opaque field, currently giving a null_mut pointer.
    pub fn dup<'a>(&'a mut self) -> &'a mut NcPlane {
        unsafe { &mut *crate::ncplane_dup(self, null_mut()) }
    }
    /// Moves this NcPlane relative to the standard plane, or the plane to
    /// which it is bound.
    ///
    /// It is an error to attempt to move the standard plane.
    // CHECK: whether a negative offset is valid
    pub fn move_yx(&mut self, y: NcOffset, x: NcOffset) -> NcResult {
        unsafe { crate::ncplane_move_yx(self, y, x) }
    }

    /// Returns the NcPlane above this one, or None if already at the top.
    pub fn above<'a>(&'a mut self) -> Option<&'a mut NcPlane> {
        let plane = unsafe { crate::ncplane_above(self) };
        if plane.is_null() {
            return None;
        }
        Some(unsafe { &mut *plane })
    }

    /// Returns the NcPlane below this one, or None if already at the bottom.
    pub fn below<'a>(&'a mut self) -> Option<&'a mut NcPlane> {
        let plane = unsafe { crate::ncplane_below(self) };
        if plane.is_null() {
            return None;
        }
        Some(unsafe { &mut *plane })
    }

    /// Gets the parent to which this NcPlane is bound, if any.
    // TODO: CHECK: what happens when it's bound to itself.
    // pub fn parent<'a>(&'a mut self) -> Option<&'a mut NcPlane> {
    pub fn parent<'a>(&'a mut self) -> &'a mut NcPlane {
        unsafe { &mut *crate::ncplane_parent(self) }
    }

    /// Gets the parent to which this NcPlane is bound, if any.
    // TODO: CHECK: what happens when it's bound to itself.
    // pub fn parent<'a>(&'a mut self) -> Option<&'a mut NcPlane> {
    pub fn parent_const<'a>(&'a self) -> &'a NcPlane {
        unsafe { &*crate::ncplane_parent_const(self) }
    }

    // Context -----------------------------------------------------------------

    /// Gets a mutable reference to the [Notcurses] context of this NcPlane.
    pub fn notcurses<'a>(&mut self) -> &'a mut Notcurses {
        unsafe { &mut *crate::ncplane_notcurses(self) }
    }

    /// Gets an immutable reference to the [Notcurses] context of this NcPlane.
    pub fn notcurses_const<'a>(&mut self) -> &'a Notcurses {
        unsafe { &*crate::ncplane_notcurses_const(self) }
    }

    // Box, perimeter-----------------------------------------------------------

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
    pub fn r#box(
        &mut self,
        ul: &NcCell,
        ur: &NcCell,
        ll: &NcCell,
        lr: &NcCell,
        hline: &NcCell,
        vline: &NcCell,
        y_stop: NcDimension,
        x_stop: NcDimension,
        boxmask: NcBoxMask,
    ) -> NcResult {
        unsafe {
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
        }
    }

    /// Draws a box with its upper-left corner at the current cursor position,
    /// having dimensions `y_len` * `x_len`.
    /// The minimum box size is 2x2, and it cannot be drawn off-screen.
    ///
    /// See the [`box`](type.NcPlane.html#method.box) method for more information.
    pub fn box_sized(
        &mut self,
        ul: &NcCell,
        ur: &NcCell,
        ll: &NcCell,
        lr: &NcCell,
        hline: &NcCell,
        vline: &NcCell,
        y_len: NcDimension,
        x_len: NcDimension,
        boxmask: NcBoxMask,
    ) -> NcResult {
        crate::ncplane_box_sized(self, ul, ur, ll, lr, hline, vline, y_len, x_len, boxmask)
    }

    /// Draws the perimeter around this NcPlane.
    pub fn perimeter(
        &mut self,
        ul: &NcCell,
        ur: &NcCell,
        ll: &NcCell,
        lr: &NcCell,
        hline: &NcCell,
        vline: &NcCell,
        boxmask: NcBoxMask,
    ) -> NcResult {
        crate::ncplane_perimeter(self, ul, ur, ll, lr, hline, vline, boxmask)
    }

    ///
    pub fn perimeter_double(
        &mut self,
        stylemask: NcStyleMask,
        channels: NcChannelPair,
        boxmask: NcBoxMask,
    ) -> NcResult {
        crate::ncplane_perimeter_double(self, stylemask, channels, boxmask)
    }

    ///
    pub fn perimeter_rounded(
        &mut self,
        stylemask: NcStyleMask,
        channels: NcChannelPair,
        boxmask: NcBoxMask,
    ) -> NcResult {
        crate::ncplane_perimeter_rounded(self, stylemask, channels, boxmask)
    }
}
