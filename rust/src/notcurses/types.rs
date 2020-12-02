//! Types related with `Notcurses`

/// The main struct of the (full mode) TUI library
///
/// Notcurses builds atop the terminfo abstraction layer to
/// provide reasonably portable vivid character displays.
///
pub type Notcurses = crate::bindings::bindgen::notcurses;

/// Options struct for [`Notcurses`]
pub type NotcursesOptions = crate::bindings::bindgen::notcurses_options;

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
pub const NCOPTION_INHIBIT_SETLOCALE: u64 =
    crate::bindings::bindgen::NCOPTION_INHIBIT_SETLOCALE as u64;

/// Do not enter alternate mode.
///
/// If smcup/rmcup capabilities are indicated, Notcurses defaults to making use
/// of the "alternate screen". This flag inhibits use of smcup/rmcup.
pub const NCOPTION_NO_ALTERNATE_SCREEN: u64 =
    crate::bindings::bindgen::NCOPTION_NO_ALTERNATE_SCREEN as u64;

/// Do not modify the font.
///
/// Notcurses might attempt to change the font slightly, to support certain
/// glyphs (especially on the Linux console). If this is set, no such
/// modifications will be made. Note that font changes will not affect anything
/// but the virtual console/terminal in which Notcurses is running.
pub const NCOPTION_NO_FONT_CHANGES: u64 = crate::bindings::bindgen::NCOPTION_NO_FONT_CHANGES as u64;

/// Do not handle SIG{ING, SEGV, ABRT, QUIT}
///
/// We typically install a signal handler for SIG{INT, SEGV, ABRT, QUIT} that
/// restores the screen, and then calls the old signal handler. Set to inhibit
/// registration of these signal handlers.
pub const NCOPTION_NO_QUIT_SIGHANDLERS: u64 =
    crate::bindings::bindgen::NCOPTION_NO_QUIT_SIGHANDLERS as u64;

/// Do not handle SIGWINCH
///
/// We typically install a signal handler for SIGWINCH that generates a resize
/// event in the notcurses_getc() queue. Set to inhibit this handler
pub const NCOPTION_NO_WINCH_SIGHANDLER: u64 =
    crate::bindings::bindgen::NCOPTION_NO_WINCH_SIGHANDLER as u64;

/// Do not print banners
///
/// Notcurses typically prints version info in notcurses_init() and performance
/// info in notcurses_stop(). This inhibits that output.
pub const NCOPTION_SUPPRESS_BANNERS: u64 =
    crate::bindings::bindgen::NCOPTION_SUPPRESS_BANNERS as u64;

/// Test for Sixel support
///
/// Checking for Sixel support requires writing an escape, and then reading an
/// inline reply from the terminal. Since this can interact poorly with actual
/// user input, it's not done unless Sixel will actually be used. Set this flag
/// to unconditionally test for Sixel support in notcurses_init().
pub const NCOPTION_VERIFY_SIXEL: u64 = crate::bindings::bindgen::NCOPTION_VERIFY_SIXEL as u64;

// NcLogLevel ------------------------------------------------------------------

/// Log level for [`NotcursesOptions`]
///
/// These log levels consciously map cleanly to those of libav; Notcurses itself
/// does not use this full granularity. The log level does not affect the opening
/// and closing banners, which can be disabled via the `NotcursesOptions`
/// `NCOPTION_SUPPRESS_BANNERS`.
/// Note that if stderr is connected to the same terminal on which we're
/// rendering, any kind of logging will disrupt the output.
pub type NcLogLevel = crate::bindings::bindgen::ncloglevel_e;

/// this is honestly a bit much
pub const NCLOGLEVEL_DEBUG: NcLogLevel = crate::bindings::bindgen::ncloglevel_e_NCLOGLEVEL_DEBUG;

/// we can't keep doin' this, but we can do other things
pub const NCLOGLEVEL_ERROR: NcLogLevel = crate::bindings::bindgen::ncloglevel_e_NCLOGLEVEL_ERROR;

/// we're hanging around, but we've had a horrible fault
pub const NCLOGLEVEL_FATAL: NcLogLevel = crate::bindings::bindgen::ncloglevel_e_NCLOGLEVEL_FATAL;

/// "detailed information
pub const NCLOGLEVEL_INFO: NcLogLevel = crate::bindings::bindgen::ncloglevel_e_NCLOGLEVEL_INFO;

/// print diagnostics immediately related to crashing
pub const NCLOGLEVEL_PANIC: NcLogLevel = crate::bindings::bindgen::ncloglevel_e_NCLOGLEVEL_PANIC;

/// default. print nothing once fullscreen service begins
pub const NCLOGLEVEL_SILENT: NcLogLevel = crate::bindings::bindgen::ncloglevel_e_NCLOGLEVEL_SILENT;

/// there's probably a better way to do what you want
pub const NCLOGLEVEL_TRACE: NcLogLevel = crate::bindings::bindgen::ncloglevel_e_NCLOGLEVEL_TRACE;

/// "detailed information
pub const NCLOGLEVEL_VERBOSE: NcLogLevel =
    crate::bindings::bindgen::ncloglevel_e_NCLOGLEVEL_VERBOSE;

/// you probably don't want what's happening to happen
pub const NCLOGLEVEL_WARNING: NcLogLevel =
    crate::bindings::bindgen::ncloglevel_e_NCLOGLEVEL_WARNING;
