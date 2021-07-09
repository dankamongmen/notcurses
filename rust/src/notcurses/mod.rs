//! `Nc`

// total: 53
// ---------------------------------------------------
// (X)  1 : wont do
// (…)  4 : TODO / WIP
//
// (f) 45 : unsafe ffi function exported by bindgen
// (w)  0 : safely wrapped ffi function
// (r)  6 : static function manually reimplemented
//
// (m) 38 : method implemented
//
// (t) 13 : unit test done for the function
// (T)  0 : unit test done also for the method
// ---------------------------------------------------
// fm  notcurses_at_yx
// fm  notcurses_bottom
// fm  notcurses_canbraille
// fmt notcurses_canchangecolor
// fmt notcurses_canfade
// fmt notcurses_canopen_images
// fmt notcurses_canopen_videos
// fmt notcurses_cansextant
// fmt notcurses_cantruecolor
// fmt notcurses_canutf8
// fm  notcurses_check_pixel_support
//~f   notcurses_core_init
// fm  notcurses_cursor_disable
// fm  notcurses_cursor_enable
// f   notcurses_cursor_yx
// fmt notcurses_debug
//~f   notcurses_detected_terminal
// fmt notcurses_drop_planes
// fm  notcurses_getc
// fmt notcurses_init
// fm  notcurses_inputready_fd
// fm  notcurses_lex_blitter
// fm  notcurses_lex_margins
// fm  notcurses_lex_scalemode
// fm  notcurses_linesigs_disable
// fm  notcurses_linesigs_enable
// fm  notcurses_mouse_disable
// fm  notcurses_mouse_enable
// fm  notcurses_palette_size
// fm  notcurses_refresh
// fm  notcurses_render
// fm  notcurses_render_to_buffer
// fm  notcurses_render_to_file
// fm  notcurses_stats
// fm  notcurses_stats_alloc
// fm  notcurses_stats_reset
// fm  notcurses_stdplane
// fm  notcurses_stdplane_const
// fmt notcurses_stop
// fm  notcurses_str_blitter
// fm  notcurses_str_scalemode
// fm  notcurses_supported_styles
// fm  notcurses_top
//X    notcurses_ucs32_to_utf8 (not needed in rust)
// fmt notcurses_version
// fm  notcurses_version_components
// rmt notcurses_align
// rm  notcurses_getc_blocking
// rm  notcurses_getc_nblock
//~r   notcurses_stddim_yx           // multiple mutable references errors
//~r   notcurses_stddim_yx_const     //
// rm  notcurses_term_dim_yx

#[cfg(test)]
mod test;

mod helpers;
mod methods;
mod reimplemented;

#[allow(unused_imports)]
pub(crate) use helpers::*;
pub use reimplemented::*;

/// The full **notcurses** context.
///
/// It's built atop the terminfo abstraction layer to provide reasonably
/// portable vivid character displays.
pub type Nc = crate::bindings::ffi::notcurses;

#[deprecated]
#[doc(hidden)]
pub type Notcurses = Nc;

/// Options struct for [`Notcurses`]
pub type NcOptions = crate::bindings::ffi::notcurses_options;

#[deprecated]
#[doc(hidden)]
pub type NotcursesOptions = NcOptions;

/// Do not call setlocale()
///
/// notcurses_init() will call setlocale() to inspect the current locale. If
/// that locale is "C" or "POSIX", it will call setlocale(LC_ALL, "") to set
/// the locale according to the LANG environment variable. Ideally, this will
/// result in UTF8 being enabled, even if the client app didn't call
/// setlocale() itself. Unless you're certain that you're invoking setlocale()
/// prior to notcurses_init(), you should not set this bit. Even if you are
/// invoking setlocale(), this behavior shouldn't be an issue unless you're
/// doing something weird (setting a locale not based on LANG).
pub const NCOPTION_INHIBIT_SETLOCALE: u64 = crate::bindings::ffi::NCOPTION_INHIBIT_SETLOCALE as u64;

/// Do not enter alternate mode.
///
/// If smcup/rmcup capabilities are indicated, notcurses defaults to making use
/// of the "alternate screen". This flag inhibits use of smcup/rmcup.
pub const NCOPTION_NO_ALTERNATE_SCREEN: u64 =
    crate::bindings::ffi::NCOPTION_NO_ALTERNATE_SCREEN as u64;

/// Do not try to clear any preexisting bitmaps.
///
/// Note that they might still get cleared even if this is set, and they might
/// not get cleared even if this is not set.
pub const NCOPTION_NO_CLEAR_BITMAPS: u64 = crate::bindings::ffi::NCOPTION_NO_CLEAR_BITMAPS as u64;

/// Do not modify the font.
///
/// Notcurses might attempt to change the font slightly, to support certain
/// glyphs (especially on the Linux console). If this is set, no such
/// modifications will be made. Note that font changes will not affect anything
/// but the virtual console/terminal in which notcurses is running.
pub const NCOPTION_NO_FONT_CHANGES: u64 = crate::bindings::ffi::NCOPTION_NO_FONT_CHANGES as u64;

/// Do not handle SIG{ING, SEGV, ABRT, QUIT}.
///
/// A signal handler will usually be installed for SIGINT, SIGQUIT, SIGSEGV,
/// SIGTERM, and SIGABRT, cleaning up the terminal on such exceptions.
/// With this flag, the handler will not be installed.
pub const NCOPTION_NO_QUIT_SIGHANDLERS: u64 =
    crate::bindings::ffi::NCOPTION_NO_QUIT_SIGHANDLERS as u64;

/// Do not handle SIGWINCH.
///
/// A signal handler will usually be installed for SIGWINCH, resulting in
/// NCKEY_RESIZE events being generated on input.
/// With this flag, the handler will not be installed.
pub const NCOPTION_NO_WINCH_SIGHANDLER: u64 =
    crate::bindings::ffi::NCOPTION_NO_WINCH_SIGHANDLER as u64;

/// Initialize the standard plane's virtual cursor to match the physical cursor
/// at context creation time.
///
/// Together with [`NCOPTION_NO_ALTERNATE_SCREEN`] and a scrolling standard plane,
/// this facilitates easy scrolling-style programs in rendered mode.
pub const NCOPTION_PRESERVE_CURSOR: u64 = crate::bindings::ffi::NCOPTION_PRESERVE_CURSOR as u64;

/// Do not print banners.
///
/// Notcurses typically prints version info in notcurses_init() and performance
/// info in notcurses_stop(). This inhibits that output.
pub const NCOPTION_SUPPRESS_BANNERS: u64 = crate::bindings::ffi::NCOPTION_SUPPRESS_BANNERS as u64;

// NcLogLevel ------------------------------------------------------------------

/// Log level for [`NcOptions`]
///
/// These log levels consciously map cleanly to those of libav; notcurses itself
/// does not use this full granularity. The log level does not affect the opening
/// and closing banners, which can be disabled via the `NcOptions`
/// `NCOPTION_SUPPRESS_BANNERS`.
/// Note that if stderr is connected to the same terminal on which we're
/// rendering, any kind of logging will disrupt the output.
pub type NcLogLevel = crate::bindings::ffi::ncloglevel_e;

/// this is honestly a bit much
pub const NCLOGLEVEL_DEBUG: NcLogLevel = crate::bindings::ffi::ncloglevel_e_NCLOGLEVEL_DEBUG;

/// we can't keep doin' this, but we can do other things
pub const NCLOGLEVEL_ERROR: NcLogLevel = crate::bindings::ffi::ncloglevel_e_NCLOGLEVEL_ERROR;

/// we're hanging around, but we've had a horrible fault
pub const NCLOGLEVEL_FATAL: NcLogLevel = crate::bindings::ffi::ncloglevel_e_NCLOGLEVEL_FATAL;

/// "detailed information
pub const NCLOGLEVEL_INFO: NcLogLevel = crate::bindings::ffi::ncloglevel_e_NCLOGLEVEL_INFO;

/// print diagnostics immediately related to crashing
pub const NCLOGLEVEL_PANIC: NcLogLevel = crate::bindings::ffi::ncloglevel_e_NCLOGLEVEL_PANIC;

/// default. print nothing once fullscreen service begins
pub const NCLOGLEVEL_SILENT: NcLogLevel = crate::bindings::ffi::ncloglevel_e_NCLOGLEVEL_SILENT;

/// there's probably a better way to do what you want
pub const NCLOGLEVEL_TRACE: NcLogLevel = crate::bindings::ffi::ncloglevel_e_NCLOGLEVEL_TRACE;

/// "detailed information
pub const NCLOGLEVEL_VERBOSE: NcLogLevel = crate::bindings::ffi::ncloglevel_e_NCLOGLEVEL_VERBOSE;

/// you probably don't want what's happening to happen
pub const NCLOGLEVEL_WARNING: NcLogLevel = crate::bindings::ffi::ncloglevel_e_NCLOGLEVEL_WARNING;

// NcAlign -- ------------------------------------------------------------------

/// Alignment within a plane or terminal.
/// Left/right-justified, or centered.
///
/// ## Defined constants
///
/// - [NCALIGN_UNALIGNED]
/// - [NCALIGN_LEFT]
/// - [NCALIGN_CENTER]
/// - [NCALIGN_RIGHT]
pub type NcAlign = crate::bindings::ffi::ncalign_e;

/// Left alignment within an [`NcPlane`][crate::NcPlane] or terminal.
pub const NCALIGN_LEFT: NcAlign = crate::bindings::ffi::ncalign_e_NCALIGN_LEFT;

/// Right alignment within an [`NcPlane`][crate::NcPlane] or terminal.
pub const NCALIGN_RIGHT: NcAlign = crate::bindings::ffi::ncalign_e_NCALIGN_RIGHT;

/// Center alignment within an [`NcPlane`][crate::NcPlane] or terminal.
pub const NCALIGN_CENTER: NcAlign = crate::bindings::ffi::ncalign_e_NCALIGN_CENTER;

/// Do not align an [`NcPlane`][crate::NcPlane] or terminal.
pub const NCALIGN_UNALIGNED: NcAlign = crate::bindings::ffi::ncalign_e_NCALIGN_UNALIGNED;
