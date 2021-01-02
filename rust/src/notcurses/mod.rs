//! `NcNotcurses`

// functions already exported by bindgen : 42
// ------------------------------------------
// (#) test: 10
// (W) wrap: 41 / 1
// ------------------------------------------
//W  notcurses_at_yx
//W  notcurses_bottom
//W# notcurses_canchangecolor
//W# notcurses_canfade
//W# notcurses_canopen_images
//W# notcurses_canopen_videos
//W# notcurses_cansixel
//W# notcurses_cansextant
//W# notcurses_cantruecolor
//W# notcurses_canutf8
//W  notcurses_cursor_disable
//W  notcurses_cursor_enable
//W# notcurses_debug
//W# notcurses_drop_planes
//W  notcurses_getc
//W# notcurses_init
//W  notcurses_inputready_fd
//W  notcurses_lex_blitter
//W  notcurses_lex_margins
//W  notcurses_lex_scalemode
//W  notcurses_linesigs_disable
//W  notcurses_linesigs_enable
//W  notcurses_mouse_disable
//W  notcurses_mouse_enable
//W  notcurses_palette_size
//W  notcurses_refresh
//W  notcurses_render
//W  notcurses_render_to_buffer
//W  notcurses_render_to_file
//W  notcurses_stats
//W  notcurses_stats_alloc
//W  notcurses_stats_reset
//W  notcurses_stdplane
//W  notcurses_stdplane_const
//W# notcurses_stop
//W  notcurses_str_blitter
//W  notcurses_str_scalemode
//W  notcurses_supported_styles
//W  notcurses_top
//~  notcurses_ucs32_to_utf8 (not needed in rust)
//W# notcurses_version
//W  notcurses_version_components
//
// functions manually reimplemented: 6
// -----------------------------------------
// (+) done: 6 / 0
// (#) test: 1
// (W) wrap: 4 / 0
// -----------------------------------------
//W# notcurses_align
//W+ notcurses_getc_blocking
//W+ notcurses_getc_nblock
//~+ notcurses_stddim_yx           // multiple mutable references errors
//~+ notcurses_stddim_yx_const     //
//W+ notcurses_term_dim_yx

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

/// NcNotcurses builds atop the terminfo abstraction layer to
/// provide reasonably portable vivid character displays.
///
/// This is the internal type safely wrapped by [Notcurses].
///
pub type NcNotcurses = crate::bindings::ffi::notcurses;

/// Options struct for [`NcNotcurses`]
pub type NcNotcursesOptions = crate::bindings::ffi::notcurses_options;

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
/// If smcup/rmcup capabilities are indicated, NcNotcurses defaults to making use
/// of the "alternate screen". This flag inhibits use of smcup/rmcup.
pub const NCOPTION_NO_ALTERNATE_SCREEN: u64 =
    crate::bindings::ffi::NCOPTION_NO_ALTERNATE_SCREEN as u64;

/// Do not modify the font.
///
/// NcNotcurses might attempt to change the font slightly, to support certain
/// glyphs (especially on the Linux console). If this is set, no such
/// modifications will be made. Note that font changes will not affect anything
/// but the virtual console/terminal in which NcNotcurses is running.
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
/// NcNotcurses typically prints version info in notcurses_init() and performance
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

/// Log level for [`NcNotcursesOptions`]
///
/// These log levels consciously map cleanly to those of libav; NcNotcurses itself
/// does not use this full granularity. The log level does not affect the opening
/// and closing banners, which can be disabled via the `NcNotcursesOptions`
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
