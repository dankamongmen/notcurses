// functions already exported by bindgen : 36
// ------------------------------------------
//  notcurses_at_yx
//  notcurses_bottom
//# notcurses_canchangecolor
//# notcurses_canfade
//# notcurses_canopen_images
//# notcurses_canopen_videos
//# notcurses_cansixel
//# notcurses_cantruecolor
//# notcurses_canutf8
//  notcurses_cursor_disable
//  notcurses_cursor_enable
//  notcurses_debug
//  notcurses_drop_planes
//  notcurses_getc
//# notcurses_init
//  notcurses_inputready_fd
//  notcurses_lex_blitter
//  notcurses_lex_margins
//  notcurses_lex_scalemode
//  notcurses_mouse_disable
//  notcurses_mouse_enable
//  notcurses_palette_size
//  notcurses_refresh
//  notcurses_render
//  notcurses_render_to_buffer
//  notcurses_render_to_file
//  notcurses_stats
//  notcurses_stats_alloc
//  notcurses_stats_reset
//  notcurses_stdplane
//  notcurses_stdplane_const
//# notcurses_stop
//  notcurses_str_blitter
//  notcurses_str_scalemode
//  notcurses_supported_styles
//  notcurses_top
//  notcurses_ucs32_to_utf8
//  notcurses_version
//  notcurses_version_components
//
// static inline functions total: 6
// ----------------------------------------- (done / remaining)
// (+) implement : 5 / 1
// (#) unit tests: 0 / 6
// -----------------------------------------
//+ notcurses_align
//+ notcurses_getc_blocking
//+ notcurses_getc_nblock
//+ notcurses_stddim_yx
//  notcurses_stddim_yx_const
//+ notcurses_term_dim_yx

use core::ptr::{null, null_mut};

use crate::{
    // NOTE: can't use libc::timespec nor libc::sigset_t
    // with notcurses_getc(()
    bindings::{sigemptyset, sigfillset, sigset_t, timespec},
    ncplane_dim_yx,
    notcurses_getc,
    notcurses_init,
    notcurses_stdplane,
    notcurses_stdplane_const,
    types::{
        NcAlign, NcInput, NcLogLevel, NcPlane, Notcurses, NotcursesOptions, NCALIGN_CENTER,
        NCALIGN_LEFT, NCOPTION_SUPPRESS_BANNERS, NCOPTION_NO_ALTERNATE_SCREEN,
    },
};

impl NotcursesOptions {
    /// Simple `NotcursesOptions` constructor
    pub fn new() -> Self {
        Self::with_all_options(0, 0, 0, 0, 0, 0)
    }

    /// `NotcursesOptions` constructor with customizable margins
    pub fn with_margins(top: i32, right: i32, bottom: i32, left: i32) -> Self {
        Self::with_all_options(0, top, right, bottom, left, 0)
    }

    /// `NotcursesOptions` constructor with customizable flags
    pub fn with_flags(flags: u64) -> Self {
        Self::with_all_options(0, 0, 0, 0, 0, flags)
    }

    /// `NotcursesOptions` constructor with all the options available
    ///
    /// ## Arguments
    ///
    /// - loglevel
    ///
    ///   Progressively higher log levels result in more logging to stderr. By
    ///   default, nothing is printed to stderr once fullscreen service begins.
    ///
    /// - margin_t, margin_r, margin_b, margin_l
    ///
    ///   Desirable margins (top, right, bottom, left).
    ///
    ///   If all are 0 (default), we will render to the entirety of the screen.
    ///   If the screen is too small, we do what we can.
    ///   Absolute coordinates are relative to the rendering area
    ///   ((0, 0) is always the origin of the rendering area).
    ///
    /// - flags
    ///
    ///   General flags; This is expressed as a bitfield so that future options
    ///   can be added without reshaping the struct.
    ///   Undefined bits must be set to 0.
    ///
    ///   - [`NCOPTION_INHIBIT_SETLOCALE`](type.NCOPTION_INHIBIT_SETLOCALE.html)
    ///   - [`NCOPTION_NO_ALTERNATE_SCREEN`](type.NCOPTION_NO_ALTERNATE_SCREEN.html)
    ///   - [`NCOPTION_NO_FONT_CHANGES`](type.NCOPTION_NO_FONT_CHANGES.html)
    ///   - [`NCOPTION_NO_QUIT_SIGHANDLERS`](type.NCOPTION_NO_QUIT_SIGHANDLERS.html)
    ///   - [`NCOPTION_NO_WINCH_SIGHANDLER`](type.NCOPTION_NO_WINCH_SIGHANDLER.html)
    ///   - [`NCOPTION_SUPPRESS_BANNERS`](type.NCOPTION_SUPPRESS_BANNERS.html)
    ///
    pub fn with_all_options(
        loglevel: NcLogLevel,
        margin_t: i32,
        margin_r: i32,
        margin_b: i32,
        margin_l: i32,
        flags: u64,
    ) -> Self {
        Self {
            termtype: null(),
            renderfp: null_mut(),
            loglevel,
            margin_t,
            margin_r,
            margin_b,
            margin_l,
            flags,
        }
    }
}

impl Notcurses {
    /// `Notcurses` simple constructor with clean output
    pub unsafe fn new<'a>() -> &'a mut Notcurses {
        let options = NotcursesOptions::with_flags(NCOPTION_SUPPRESS_BANNERS);
        &mut *notcurses_init(&options, null_mut())
    }

    /// `Notcurses` simple constructor, showing banners
    pub unsafe fn with_banners<'a>() -> &'a mut Notcurses {
        &mut *notcurses_init(&NotcursesOptions::new(), null_mut())
    }

    /// `Notcurses` simple constructor without an alternate screen
    pub unsafe fn without_altscreen<'a>() -> &'a mut Notcurses {
        let options = NotcursesOptions::with_flags(NCOPTION_NO_ALTERNATE_SCREEN);
        &mut *notcurses_init(&options, null_mut())
    }

    /// `Notcurses` simple constructor without an alternate screen
    pub unsafe fn without_altscreen_nor_banners<'a>() -> &'a mut Notcurses {
        let options = NotcursesOptions::with_flags(
            NCOPTION_NO_ALTERNATE_SCREEN | NCOPTION_SUPPRESS_BANNERS);
        &mut *notcurses_init(&options, null_mut())
    }

    /// `Notcurses` constructor with options
    pub unsafe fn with_options<'a>(options: &NotcursesOptions) -> &'a mut Notcurses {
        &mut *notcurses_init(options, null_mut())
    }
}

/// return the offset into 'availcols' at which 'cols' ought be output given the requirements of 'align'
#[inline]
pub fn notcurses_align(availcols: i32, align: NcAlign, cols: i32) -> i32 {
    if align == NCALIGN_LEFT {
        return 0;
    }
    if cols > availcols {
        return 0;
    }
    if align == NCALIGN_CENTER {
        return (availcols - cols) / 2;
    }
    availcols - cols // NCALIGN_RIGHT
}

/// 'input' may be NULL if the caller is uninterested in event details.
/// If no event is ready, returns 0.
// TODO: use pakr-signals
#[inline]
pub fn notcurses_getc_nblock(nc: &mut Notcurses, input: &mut NcInput) -> char {
    unsafe {
        let mut sigmask = sigset_t { __val: [0; 16] };
        sigfillset(&mut sigmask);
        let ts = timespec {
            tv_sec: 0,
            tv_nsec: 0,
        };
        core::char::from_u32_unchecked(notcurses_getc(nc, &ts, &mut sigmask, input))
    }
}

/// 'input' may be NULL if the caller is uninterested in event details.
/// Blocks until an event is processed or a signal is received.
#[inline]
pub fn notcurses_getc_nblocking(nc: &mut Notcurses, input: &mut NcInput) -> char {
    unsafe {
        let mut sigmask = sigset_t { __val: [0; 16] };
        sigemptyset(&mut sigmask);
        core::char::from_u32_unchecked(notcurses_getc(nc, null(), &mut sigmask, input))
    }
}

/// notcurses_stdplane(), plus free bonus dimensions written to non-NULL y/x!
#[inline]
pub fn notcurses_stddim_yx(nc: &mut Notcurses, y: &mut i32, x: &mut i32) -> NcPlane {
    unsafe {
        let s = notcurses_stdplane(nc);
        ncplane_dim_yx(s, y, x);
        *s
    }
}

/// Return our current idea of the terminal dimensions in rows and cols.
#[inline]
pub fn notcurses_term_dim_yx(nc: &Notcurses, rows: &mut i32, cols: &mut i32) {
    unsafe {
        ncplane_dim_yx(notcurses_stdplane_const(nc), rows, cols);
    }
}

#[cfg(test)]
mod test {
    use serial_test::serial;

    use crate::{notcurses_stop, Notcurses};

    /*
    #[test]
    #[serial]
    fn () {
    }
    */

    // Test the bindgen functions ----------------------------------------------

    #[test]
    #[serial]
    #[ignore]
    // FIXME: always return null
    fn notcurses_at_yx() {
        unsafe {
            let nc = Notcurses::new();
            let mut sm = 0;
            let mut ch = 0;
            let res = crate::notcurses_at_yx(nc, 0, 0, &mut sm, &mut ch);
            notcurses_stop(nc);
            assert![!res.is_null()];

            //print!("[{}] ", res);
        }
    }

    // TODO: a multiplatform way of dealing with C FILE
    //
    // links:
    // https://www.reddit.com/r/rust/comments/8sfjp6/converting_between_file_and_stdfsfile/
    // https://stackoverflow.com/questions/38360996/how-do-i-access-fields-of-a-mut-libcfile
    #[test]
    #[serial]
    // FIXME: need a solution to deal with _IO_FILE from Rust
    #[ignore]
    fn notcurses_debug() {
        unsafe {
            let nc = Notcurses::new();

            // https://doc.rust-lang.org/stable/std/primitive.pointer.html
            let mut _p: *mut i8 = &mut 0;
            let mut _size: *mut usize = &mut 0;

            // https://docs.rs/libc/0.2.80/libc/fn.open_memstream.html
            let mut file = libc::open_memstream(&mut _p, _size)
                as *mut _ as *mut crate::bindgen::_IO_FILE;

            crate::notcurses_debug(nc, file);
            notcurses_stop(nc);

            // as _IO_FILE struct;
            let mut debug0 = *file;
            println!("{:#?}", debug0);

            // as enum libc::FILE
            let mut debug1 = file as *mut _ as *mut libc::FILE;
            println!("{:#?}", debug1);
        }
    }

    #[test]
    #[serial]
    fn notcurses_canchangecolor() {
        unsafe {
            let nc = Notcurses::new();
            let res = crate::notcurses_canchangecolor(nc);
            notcurses_stop(nc);
            print!("[{}] ", res);
        }
    }

    #[test]
    #[serial]
    fn notcurses_canfade() {
        unsafe {
            let nc = Notcurses::new();
            let res = crate::notcurses_canfade(nc);
            notcurses_stop(nc);
            print!("[{}] ", res);
        }
    }

    #[test]
    #[serial]
    fn notcurses_canopen_images() {
        unsafe {
            let nc = Notcurses::new();
            let res = crate::notcurses_canopen_images(nc);
            notcurses_stop(nc);
            print!("[{}] ", res);
        }
    }

    #[test]
    #[serial]
    fn notcurses_canopen_videos() {
        unsafe {
            let nc = Notcurses::new();
            let res = crate::notcurses_canopen_videos(nc);
            notcurses_stop(nc);
            print!("[{}] ", res);
        }
    }

    #[test]
    #[serial]
    fn notcurses_cansixel() {
        unsafe {
            let nc = Notcurses::new();
            let res = crate::notcurses_cansixel(nc);
            notcurses_stop(nc);
            print!("[{}] ", res);
        }
    }

    #[test]
    #[serial]
    fn notcurses_cantruecolor() {
        unsafe {
            let nc = Notcurses::new();
            let res = crate::notcurses_cantruecolor(nc);
            notcurses_stop(nc);
            print!("[{}] ", res);
        }
    }

    #[test]
    #[serial]
    fn notcurses_canutf8() {
        unsafe {
            let nc = Notcurses::new();
            let res = crate::notcurses_canutf8(nc);
            notcurses_stop(nc);
            print!("[{}] ", res);
        }
    }

}
