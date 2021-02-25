//! `Notcurses`

// --- -------------------------------------------------------------------------
// col 0:
// --------------
//          1  X: wont do
//          2  ~: WIP
//
// col 1:  48
// --------------
//         42  f: ffi function imported by bindgen
//             F: ffi function wrapped safely
//          6  r: static function reimplemented in Rust
//
// col 2:  45
// --------------
//         38  m: impl as a `Notcurses` method
//          7  M: impl for the `FullMode` wrapper struct too
//
// col 3:  14
// --------------
//         14  t: tests done for the ffi or reimplemented funtion
//             T: tests done also for the m method
//             Ŧ: tests done also for the M method wrapper struct
// --- -------------------------------------------------------------------------
//
// fm  notcurses_at_yx
// fm  notcurses_bottom
// fmt notcurses_canchangecolor
// fmt notcurses_canfade
// fmt notcurses_canopen_images
// fmt notcurses_canopen_videos
// fmt notcurses_canpixel
// fmt notcurses_cansextant
// fmt notcurses_cantruecolor
// fmt notcurses_canutf8
// fm  notcurses_cursor_disable
// fm  notcurses_cursor_enable
// fmt notcurses_debug
// fmt notcurses_drop_planes
// fm  notcurses_getc
// fMt notcurses_init
// fm  notcurses_inputready_fd
// fM  notcurses_lex_blitter
// fM  notcurses_lex_margins
// fM  notcurses_lex_scalemode
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
// fMt notcurses_stop
// fM  notcurses_str_blitter
// fM  notcurses_str_scalemode
// fm  notcurses_supported_styles
// fm  notcurses_top
//X    notcurses_ucs32_to_utf8 (not needed in rust)
// fMt notcurses_version
// fM  notcurses_version_components
//
// rMt notcurses_align
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
mod wrapper;

#[allow(unused_imports)]
pub(crate) use helpers::*;
pub use reimplemented::*;
pub use wrapper::*;

/// Notcurses builds atop the terminfo abstraction layer to
/// provide reasonably portable vivid character displays.
///
/// This is the internal type safely wrapped by [FullMode].
pub type Notcurses = crate::bindings::ffi::notcurses;

/// Options struct for [`Notcurses`]
pub type NotcursesOptions = crate::bindings::ffi::notcurses_options;

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
/// If smcup/rmcup capabilities are indicated, Notcurses defaults to making use
/// of the "alternate screen". This flag inhibits use of smcup/rmcup.
pub const NCOPTION_NO_ALTERNATE_SCREEN: u64 =
    crate::bindings::ffi::NCOPTION_NO_ALTERNATE_SCREEN as u64;

/// Do not modify the font.
///
/// Notcurses might attempt to change the font slightly, to support certain
/// glyphs (especially on the Linux console). If this is set, no such
/// modifications will be made. Note that font changes will not affect anything
/// but the virtual console/terminal in which Notcurses is running.
pub const NCOPTION_NO_FONT_CHANGES: u64 = crate::bindings::ffi::NCOPTION_NO_FONT_CHANGES as u64;

/// Do not handle SIG{ING, SEGV, ABRT, QUIT}
///
/// A signal handler will usually be installed for SIGINT, SIGQUIT, SIGSEGV,
/// SIGTERM, and SIGABRT, cleaning up the terminal on such exceptions.
/// With this flag, the handler will not be installed.
pub const NCOPTION_NO_QUIT_SIGHANDLERS: u64 =
    crate::bindings::ffi::NCOPTION_NO_QUIT_SIGHANDLERS as u64;

/// Do not handle SIGWINCH
///
/// A signal handler will usually be installed for SIGWINCH, resulting in
/// NCKEY_RESIZE events being generated on input.
/// With this flag, the handler will not be installed.
pub const NCOPTION_NO_WINCH_SIGHANDLER: u64 =
    crate::bindings::ffi::NCOPTION_NO_WINCH_SIGHANDLER as u64;

/// Do not print banners
///
/// Notcurses typically prints version info in notcurses_init() and performance
/// info in notcurses_stop(). This inhibits that output.
pub const NCOPTION_SUPPRESS_BANNERS: u64 = crate::bindings::ffi::NCOPTION_SUPPRESS_BANNERS as u64;

/// Test for Sixel support
///
/// Checking for Sixel support requires writing an escape, and then reading an
/// inline reply from the terminal. Since this can interact poorly with actual
/// user input, it's not done unless Sixel will actually be used. Set this flag
/// to unconditionally test for Sixel support in notcurses_init().
pub const NCOPTION_VERIFY_SIXEL: u64 = crate::bindings::ffi::NCOPTION_VERIFY_SIXEL as u64;

// NcLogLevel ------------------------------------------------------------------

/// Log level for [`NotcursesOptions`]
///
/// These log levels consciously map cleanly to those of libav; Notcurses itself
/// does not use this full granularity. The log level does not affect the opening
/// and closing banners, which can be disabled via the `NotcursesOptions`
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
