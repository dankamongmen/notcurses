//! `Notcurses`

// functions already exported by bindgen : 39
// ------------------------------------------
// (#) unit tests: 10 / 29
// ------------------------------------------
//   notcurses_at_yx
//   notcurses_bottom
// # notcurses_canchangecolor
// # notcurses_canfade
// # notcurses_canopen_images
// # notcurses_canopen_videos
// # notcurses_cansixel
// # notcurses_cantruecolor
// # notcurses_canutf8
//   notcurses_cursor_disable
//   notcurses_cursor_enable
// # notcurses_debug
// # notcurses_drop_planes
//   notcurses_getc
// # notcurses_init
//   notcurses_inputready_fd
//   notcurses_lex_blitter
//   notcurses_lex_margins
//   notcurses_lex_scalemode
//   notcurses_mouse_disable
//   notcurses_mouse_enable
//   notcurses_palette_size
//   notcurses_refresh
//   notcurses_render
//   notcurses_render_to_buffer
//   notcurses_render_to_file
//   notcurses_stats
//   notcurses_stats_alloc
//   notcurses_stats_reset
//   notcurses_stdplane
//   notcurses_stdplane_const
// # notcurses_stop
//   notcurses_str_blitter
//   notcurses_str_scalemode
//   notcurses_supported_styles
//   notcurses_top
//   notcurses_ucs32_to_utf8
//   notcurses_version
//   notcurses_version_components
//
// functions manually reimplemented: 6
// -----------------------------------------
// (+) implement : 6 / 0
// (#) unit tests: 0 / 6
// -----------------------------------------
// # notcurses_align
// + notcurses_getc_blocking
// + notcurses_getc_nblock
// + notcurses_stddim_yx
// + notcurses_stddim_yx_const
// + notcurses_term_dim_yx

#[cfg(test)]
mod test;

mod methods;
mod reimplemented;
pub use reimplemented::*;

/// The main struct of the (full mode) TUI library
///
/// Notcurses builds atop the terminfo abstraction layer to
/// provide reasonably portable vivid character displays.
///
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
