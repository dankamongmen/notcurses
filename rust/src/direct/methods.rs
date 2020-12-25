//! `NcDirect` methods and associated functions.

use core::ptr::{null, null_mut};

use crate::ffi::sigset_t;
use crate::{
    cstring, error, NcAlign, NcBlitter, NcChannelPair, NcColor, NcDimension, NcDirect,
    NcDirectFlags, NcEgc, NcError, NcInput, NcPaletteIndex, NcPlane, NcResult, NcRgb, NcScale,
    NcStyleMask, NcTime, NCRESULT_ERR,
};

/// # `NcDirect` constructors and destructors
impl NcDirect {
    /// New NcDirect with the default options.
    ///
    /// Initializes a direct-mode notcurses context on the tty.
    ///
    /// Direct mode supports a limited subset of notcurses routines,
    /// and neither supports nor requires
    /// [notcurses_render()][crate::notcurses_render]. This can be used to add
    /// color and styling to text in the standard output paradigm.
    ///
    /// *C style function: [ncdirect_init()][crate::ncdirect_init].*
    pub fn new<'a>() -> NcResult<&'a mut NcDirect> {
        Self::with_flags(0)
    }

    /// New NcDirect with optional flags.
    ///
    /// `flags` is a bitmask over:
    /// - [NCDIRECT_OPTION_INHIBIT_CBREAK][crate::NCDIRECT_OPTION_INHIBIT_CBREAK]
    /// - [NCDIRECT_OPTION_INHIBIT_SETLOCALE][crate::NCDIRECT_OPTION_INHIBIT_SETLOCALE]
    ///
    /// *C style function: [ncdirect_init()][crate::ncdirect_init].*
    pub fn with_flags<'a>(flags: NcDirectFlags) -> NcResult<&'a mut NcDirect> {
        let res = unsafe { crate::ncdirect_init(null(), null_mut(), flags) };
        if res == null_mut() {
            return Err(NcError::with_msg(NCRESULT_ERR, "Initializing NcDirect"));
        }
        Ok(unsafe { &mut *res })
    }

    /// Releases this NcDirect and any associated resources.
    ///
    /// *C style function: [ncdirect_stop()][crate::ncdirect_stop].*
    pub fn stop(&mut self) -> NcResult<()> {
        error![unsafe { crate::ncdirect_stop(self) }]
    }
}

/// ## NcDirect methods: clear, flush, render
impl NcDirect {
    /// Clears the screen.
    ///
    /// *C style function: [ncdirect_clear()][crate::ncdirect_clear].*
    pub fn clear(&mut self) -> NcResult<()> {
        error![unsafe { crate::ncdirect_clear(self) }]
    }

    /// Forces a flush.
    ///
    /// *C style function: [ncdirect_flush()][crate::ncdirect_flush].*
    pub fn flush(&self) -> NcResult<()> {
        error![unsafe { crate::ncdirect_flush(self) }]
    }

    /// Takes the result of [render_frame()][NcDirect#method.render_frame]
    /// and writes it to the output.
    ///
    /// The `align`, `blitter`, and `scale` arguments must be the same as those
    /// passed to render_frame().
    ///
    /// *C style function: [ncdirect_raster_frame()][crate::ncdirect_raster_frame].*
    pub fn raster_frame(
        &mut self,
        faken: &mut NcPlane,
        align: NcAlign,
        blitter: NcBlitter,
        scale: NcScale,
    ) -> NcResult<()> {
        error![
            unsafe { crate::ncdirect_raster_frame(self, faken, align, blitter, scale) },
            (),
            "Rastering frame"
        ]
    }

    /// Renders an image using the specified blitter and scaling,
    /// but do not write the result.
    ///
    /// The image may be arbitrarily many rows -- the output will scroll --
    /// but will only occupy the column of the cursor, and those to the right.
    ///
    /// To actually write (and free) this, invoke ncdirect_raster_frame().
    /// and writes it to the output.
    ///
    /// The `align`, `blitter`, and `scale` arguments must be the same as those
    /// passed to render_frame().
    ///
    /// *C style function: [ncdirect_render_frame()][crate::ncdirect_render_frame].*
    pub fn render_frame<'a>(
        &mut self,
        filename: &str,
        blitter: NcBlitter,
        scale: NcScale,
    ) -> NcResult<&'a mut NcPlane> {
        let res = unsafe { crate::ncdirect_render_frame(self, cstring![filename], blitter, scale) };
        if res == null_mut() {
            return Err(NcError::with_msg(NCRESULT_ERR, "Rendering frame"));
        }
        Ok(unsafe { &mut *res })
    }

    /// Displays an image using the specified blitter and scaling.
    ///
    /// The image may be arbitrarily many rows -- the output will scroll -- but
    /// will only occupy the column of the cursor, and those to the right.
    ///
    /// The render/raster process can be split by using
    /// [render_frame()][#method.render_frame] and
    /// [raster_frame()][#method.raster_frame].
    ///
    /// *C style function: [ncdirect_render_image()][crate::ncdirect_render_image].*
    pub fn render_image(
        &mut self,
        filename: &str,
        align: NcAlign,
        blitter: NcBlitter,
        scale: NcScale,
    ) -> NcResult<()> {
        error![unsafe {
            crate::ncdirect_render_image(self, cstring![filename], align, blitter, scale)
        }]
    }
}

/// ## NcDirect methods: `NcPaletteIndex`, `NcRgb`, `NcStyleMask` & default color
impl NcDirect {
    /// Sets the foreground [NcPaletteIndex].
    ///
    /// *C style function: [ncdirect_fg_palindex()][crate::ncdirect_fg_palindex].*
    pub fn fg_palindex(&mut self, index: NcPaletteIndex) -> NcResult<()> {
        error![unsafe { crate::ncdirect_fg_palindex(self, index as i32) }]
    }

    /// Sets the background [NcPaletteIndex].
    ///
    /// *C style function: [ncdirect_bg_palindex()][crate::ncdirect_bg_palindex].*
    pub fn bg_palindex(&mut self, index: NcPaletteIndex) -> NcResult<()> {
        error![unsafe { crate::ncdirect_bg_palindex(self, index as i32) }]
    }

    /// Returns the number of simultaneous colors claimed to be supported.
    ///
    /// Note that several terminal emulators advertise more colors than they
    /// actually support, downsampling internally.
    ///
    /// *C style function: [ncdirect_palette_size()][crate::ncdirect_palette_size].*
    pub fn palette_size(&self) -> NcResult<u32> {
        let res = unsafe { crate::ncdirect_palette_size(self) };
        if res == 1 {
            return Err(NcError::with_msg(1, "No color support"));
        }
        Ok(res)
    }

    /// Sets the foreground [NcRgb].
    ///
    /// *C style function: [ncdirect_fg_rgb()][crate::ncdirect_fg_rgb].*
    pub fn fg_rgb(&mut self, rgb: NcRgb) -> NcResult<()> {
        error![unsafe { crate::ncdirect_fg_rgb(self, rgb) }]
    }

    /// Sets the background [NcRgb].
    ///
    /// *C style function: [ncdirect_bg_rgb()][crate::ncdirect_bg_rgb].*
    pub fn bg_rgb(&mut self, rgb: NcRgb) -> NcResult<()> {
        error![unsafe { crate::ncdirect_bg_rgb(self, rgb) }]
    }

    /// Sets the foreground [NcColor] components.
    ///
    /// *C style function: [ncdirect_fg_rgb8()][crate::ncdirect_fg_rgb8].*
    pub fn fg_rgb8(&mut self, red: NcColor, green: NcColor, blue: NcColor) -> NcResult<()> {
        error![crate::ncdirect_fg_rgb8(self, red, green, blue)]
    }

    /// Sets the background [NcColor] components.
    ///
    /// *C style function: [ncdirect_bg_rgb()][crate::ncdirect_bg_rgb].*
    pub fn bg_rgb8(&mut self, red: NcColor, green: NcColor, blue: NcColor) -> NcResult<()> {
        let res = crate::ncdirect_bg_rgb8(self, red, green, blue);
        error![res]
    }

    /// Removes the specified styles.
    ///
    /// *C style function: [ncdirect_styles_off()][crate::ncdirect_styles_off].*
    pub fn styles_off(&mut self, stylebits: NcStyleMask) -> NcResult<()> {
        let res = unsafe { crate::ncdirect_styles_off(self, stylebits.into()) };
        error![res]
    }

    /// Adds the specified styles.
    ///
    /// *C style function: [ncdirect_styles_on()][crate::ncdirect_styles_on].*
    pub fn styles_on(&mut self, stylebits: NcStyleMask) -> NcResult<()> {
        error![unsafe { crate::ncdirect_styles_on(self, stylebits.into()) }]
    }

    /// Sets just the specified styles.
    ///
    /// *C style function: [ncdirect_styles_set()][crate::ncdirect_styles_set].*
    pub fn styles_set(&mut self, stylebits: NcStyleMask) -> NcResult<()> {
        error![unsafe { crate::ncdirect_styles_set(self, stylebits.into()) }]
    }

    /// Indicates to use the "default color" for the foreground.
    ///
    /// *C style function: [ncdirect_fg_default()][crate::ncdirect_fg_default].*
    pub fn fg_default(&mut self) -> NcResult<()> {
        error![unsafe { crate::ncdirect_fg_default(self) }]
    }

    /// Indicates to use the "default color" for the background.
    ///
    /// *C style function: [ncdirect_bg_default()][crate::ncdirect_bg_default].*
    pub fn bg_default(&mut self) -> NcResult<()> {
        error![unsafe { crate::ncdirect_bg_default(self) }]
    }
}

/// ## NcDirect methods: capabilities, cursor, dimensions
impl NcDirect {
    /// Can we load images?
    ///
    /// Requires being built against FFmpeg/OIIO.
    ///
    /// *C style function: [ncdirect_canopen_images()][crate::ncdirect_canopen_images].*
    pub fn canopen_images(&self) -> bool {
        unsafe { crate::ncdirect_canopen_images(self) }
    }

    /// Is our encoding UTF-8?
    ///
    /// Requires LANG being set to a UTF8 locale.
    ///
    /// *C style function: [ncdirect_canutf8()][crate::ncdirect_canutf8].*
    pub fn canutf8(&self) -> bool {
        unsafe { crate::ncdirect_canutf8(self) }
    }

    /// Disables the terminal's cursor, if supported.
    ///
    /// *C style function: [ncdirect_cursor_disable()][crate::ncdirect_cursor_disable].*
    pub fn cursor_disable(&mut self) -> NcResult<()> {
        error![unsafe { crate::ncdirect_cursor_disable(self) }]
    }

    /// Enables the terminal's cursor, if supported.
    ///
    /// *C style function: [ncdirect_cursor_enable()][crate::ncdirect_cursor_enable].*
    pub fn cursor_enable(&mut self) -> NcResult<()> {
        error![unsafe { crate::ncdirect_cursor_enable(self) }]
    }

    /// Moves the cursor down, `num` rows.
    ///
    /// *C style function: [ncdirect_cursor_down()][crate::ncdirect_cursor_down].*
    pub fn cursor_down(&mut self, num: NcDimension) -> NcResult<()> {
        error![unsafe { crate::ncdirect_cursor_down(self, num as i32) }]
    }

    /// Moves the cursor left, `num` columns.
    ///
    /// *C style function: [ncdirect_cursor_left()][crate::ncdirect_cursor_left].*
    pub fn cursor_left(&mut self, num: NcDimension) -> NcResult<()> {
        error![unsafe { crate::ncdirect_cursor_left(self, num as i32) }]
    }

    /// Moves the cursor right, `num` columns.
    ///
    /// *C style function: [ncdirect_cursor_right()][crate::ncdirect_cursor_right].*
    pub fn cursor_right(&mut self, num: NcDimension) -> NcResult<()> {
        error![unsafe { crate::ncdirect_cursor_right(self, num as i32) }]
    }

    /// Moves the cursor up, `num` rows.
    ///
    /// *C style function: [ncdirect_cursor_up()][crate::ncdirect_cursor_up].*
    pub fn cursor_up(&mut self, num: NcDimension) -> NcResult<()> {
        error![unsafe { crate::ncdirect_cursor_up(self, num as i32) }]
    }

    /// Moves the cursor in direct mode to the specified row, column.
    ///
    /// *C style function: [ncdirect_cursor_move_yx()][crate::ncdirect_cursor_move_yx].*
    pub fn cursor_move_yx(&mut self, y: NcDimension, x: NcDimension) -> NcResult<()> {
        error![unsafe { crate::ncdirect_cursor_move_yx(self, y as i32, x as i32) }]
    }

    /// Moves the cursor in direct mode to the specified row.
    ///
    /// *(No equivalent C style function)*
    pub fn cursor_move_y(&mut self, y: NcDimension) -> NcResult<()> {
        error![unsafe { crate::ncdirect_cursor_move_yx(self, y as i32, -1) }]
    }

    /// Moves the cursor in direct mode to the specified column.
    ///
    /// *(No equivalent C style function)*
    pub fn cursor_move_x(&mut self, x: NcDimension) -> NcResult<()> {
        error![unsafe { crate::ncdirect_cursor_move_yx(self, -1, x as i32) }]
    }

    /// Gets the cursor position, when supported.
    ///
    /// This requires writing to the terminal, and then reading from it.
    /// If the terminal doesn't reply, or doesn't reply in a way we understand,
    /// the results might be detrimental.
    ///
    /// *C style function: [ncdirect_cursor_yx()][crate::ncdirect_cursor_yx].*
    pub fn cursor_yx(&mut self) -> NcResult<(NcDimension, NcDimension)> {
        let (mut y, mut x) = (0, 0);
        error![
            unsafe { crate::ncdirect_cursor_yx(self, &mut y, &mut x) },
            (y as NcDimension, x as NcDimension)
        ]
    }

    /// Pushes the cursor location to the terminal's stack.
    ///
    /// The depth of this stack, and indeed its existence, is terminal-dependent.
    ///
    /// *C style function: [ncdirect_cursor_push()][crate::ncdirect_cursor_push].*
    pub fn cursor_push(&mut self) -> NcResult<()> {
        error![unsafe { crate::ncdirect_cursor_push(self) }]
    }

    /// Pops the cursor location from the terminal's stack.
    ///
    /// The depth of this stack, and indeed its existence, is terminal-dependent.
    ///
    /// *C style function: [ncdirect_cursor_pop()][crate::ncdirect_cursor_pop].*
    pub fn cursor_pop(&mut self) -> NcResult<()> {
        error![unsafe { crate::ncdirect_cursor_pop(self) }]
    }

    /// Gets the current number of rows.
    ///
    /// *C style function: [ncdirect_dim_y()][crate::ncdirect_dim_y].*
    pub fn dim_y(&self) -> NcDimension {
        unsafe { crate::ncdirect_dim_y(self) as NcDimension }
    }

    /// Gets the current number of columns.
    ///
    /// *C style function: [ncdirect_dim_x()][crate::ncdirect_dim_x].*
    pub fn dim_x(&self) -> NcDimension {
        unsafe { crate::ncdirect_dim_x(self) as NcDimension }
    }

    /// Gets the current number of rows and columns.
    ///
    /// *C style function: [ncdirect_dim_y()][crate::ncdirect_dim_y].*
    pub fn dim_yx(&self) -> (NcDimension, NcDimension) {
        let y = unsafe { crate::ncdirect_dim_y(self) as NcDimension };
        let x = unsafe { crate::ncdirect_dim_x(self) as NcDimension };
        (y, x)
    }
}

/// ## NcDirect methods: I/O
impl NcDirect {
    /// Returns a [char] representing a single unicode point.
    ///
    /// If an event is processed, the return value is the `id` field from that
    /// event.
    ///
    /// Provide a None `time` to block at length, a `time` of 0 for non-blocking
    /// operation, and otherwise a timespec to bound blocking.
    ///
    /// Signals in sigmask (less several we handle internally) will be atomically
    /// masked and unmasked per [ppoll(2)](https://linux.die.net/man/2/ppoll).
    ///
    /// `*sigmask` should generally contain all signals.
    ///
    /// *C style function: [ncdirect_getc()][crate::ncdirect_getc].*
    //
    // CHECK returns 0 on a timeout.
    pub fn getc(
        &mut self,
        time: Option<NcTime>,
        sigmask: Option<&mut sigset_t>,
        input: Option<&mut NcInput>,
    ) -> NcResult<char> {
        let ntime;
        if let Some(time) = time {
            ntime = &time as *const _;
        } else {
            ntime = null();
        }

        let nsigmask;
        if let Some(sigmask) = sigmask {
            nsigmask = sigmask as *mut _;
        } else {
            nsigmask = null_mut() as *mut _;
        }
        let ninput;
        if let Some(input) = input {
            ninput = input as *mut _;
        } else {
            ninput = null_mut();
        }
        let c = unsafe {
            core::char::from_u32_unchecked(crate::ncdirect_getc(self, ntime, nsigmask, ninput))
        };
        if c as u32 as i32 == NCRESULT_ERR {
            return Err(NcError::new(NCRESULT_ERR));
        }
        Ok(c)
    }

    ///
    /// *C style function: [ncdirect_getc_nblock()][crate::ncdirect_getc_nblock].*
    pub fn getc_nblock(&mut self, input: &mut NcInput) -> char {
        crate::ncdirect_getc_nblock(self, input)
    }

    ///
    /// *C style function: [ncdirect_getc_nblocking()][crate::ncdirect_getc_nblocking].*
    pub fn getc_nblocking(&mut self, input: &mut NcInput) -> char {
        crate::ncdirect_getc_nblocking(self, input)
    }

    /// Get a file descriptor suitable for input event poll()ing.
    ///
    /// When this descriptor becomes available, you can call
    /// [getc_nblock()][NcDirect#method.getc_nblock], and input ought be ready.
    ///
    /// This file descriptor is not necessarily the file descriptor associated
    /// with stdin (but it might be!).
    ///
    /// *C style function: [ncdirect_inputready_fd()][crate::ncdirect_inputready_fd].*
    pub fn inputready_fd(&mut self) -> NcResult<()> {
        error![unsafe { crate::ncdirect_inputready_fd(self) }]
    }

    /// Outputs the `string` according to the `channels`, and
    /// returns the total number of characters written on success.
    ///
    /// Note that it does not explicitly flush output buffers, so it will not
    /// necessarily be immediately visible.
    ///
    /// *C style function: [ncdirect_putstr()][crate::ncdirect_putstr].*
    pub fn putstr(&mut self, channels: NcChannelPair, string: &str) -> NcResult<()> {
        error![unsafe { crate::ncdirect_putstr(self, channels, cstring![string]) }]
    }

    /// Draws a box with its upper-left corner at the current cursor position,
    /// having dimensions `ylen` * `xlen`.
    ///
    /// See NcPlane.[box()][crate::NcPlane#method.box] for more information.
    ///
    /// The minimum box size is 2x2, and it cannot be drawn off-screen.
    ///
    /// `wchars` is an array of 6 characters: UL, UR, LL, LR, HL, VL.
    ///
    /// *C style function: [ncdirect_box()][crate::ncdirect_box].*
    // TODO: CHECK, specially wchars.
    pub fn r#box(
        &mut self,
        ul: NcChannelPair,
        ur: NcChannelPair,
        ll: NcChannelPair,
        lr: NcChannelPair,
        wchars: &[char; 6],
        y_len: NcDimension,
        x_len: NcDimension,
        ctlword: u32,
    ) -> NcResult<()> {
        error![unsafe {
            let wchars = core::mem::transmute(wchars);
            crate::ncdirect_box(
                self,
                ul,
                ur,
                ll,
                lr,
                wchars,
                y_len as i32,
                x_len as i32,
                ctlword,
            )
        }]
    }

    /// NcDirect.[box()][NcDirect#method.box] with the double box-drawing characters.
    ///
    /// *C style function: [ncdirect_double_box()][crate::ncdirect_double_box].*
    pub fn double_box(
        &mut self,
        ul: NcChannelPair,
        ur: NcChannelPair,
        ll: NcChannelPair,
        lr: NcChannelPair,
        y_len: NcDimension,
        x_len: NcDimension,
        ctlword: u32,
    ) -> NcResult<()> {
        error![unsafe {
            crate::ncdirect_double_box(self, ul, ur, ll, lr, y_len as i32, x_len as i32, ctlword)
        }]
    }

    /// NcDirect.[box()][NcDirect#method.box] with the rounded box-drawing characters.
    ///
    /// *C style function: [ncdirect_rounded_box()][crate::ncdirect_rounded_box].*
    pub fn rounded_box(
        &mut self,
        ul: NcChannelPair,
        ur: NcChannelPair,
        ll: NcChannelPair,
        lr: NcChannelPair,
        y_len: NcDimension,
        x_len: NcDimension,
        ctlword: u32,
    ) -> NcResult<()> {
        error![unsafe {
            crate::ncdirect_rounded_box(self, ul, ur, ll, lr, y_len as i32, x_len as i32, ctlword)
        }]
    }
    /// Draws horizontal lines using the specified [NcChannelPair]s, interpolating
    /// between them as we go.
    ///
    /// All lines start at the current cursor position.
    ///
    /// The [NcEgc] at `egc` may not use more than one column.
    ///
    /// For a horizontal line, `len` cannot exceed the screen width minus the
    /// cursor's offset.
    ///
    /// *C style function: [ncdirect_hline_interp()][crate::ncdirect_hline_interp].*
    #[inline]
    pub fn hline_interp(
        &mut self,
        egc: &NcEgc,
        len: NcDimension,
        h1: NcChannelPair,
        h2: NcChannelPair,
    ) -> NcResult<()> {
        error![unsafe { crate::ncdirect_hline_interp(self, &(*egc as i8), len as i32, h1, h2) }]
    }

    /// Draws horizontal lines using the specified [NcChannelPair]s, interpolating
    /// between them as we go.
    ///
    /// All lines start at the current cursor position.
    ///
    /// The [NcEgc] at `egc` may not use more than one column.
    ///
    /// For a vertical line, `len` may be as long as you'd like; the screen
    /// will scroll as necessary.
    ///
    /// *C style function: [ncdirect_vline_interp()][crate::ncdirect_vline_interp].*
    #[inline]
    pub fn vline_interp(
        &mut self,
        egc: &NcEgc,
        len: NcDimension,
        h1: NcChannelPair,
        h2: NcChannelPair,
    ) -> NcResult<()> {
        error![unsafe { crate::ncdirect_vline_interp(self, &(*egc as i8), len as i32, h1, h2) }]
    }
}
