//! Macros
//!
//
// NOTE: Use full paths everywhere. Don't assume anything will be in scope.

#[allow(unused_imports)]
// enjoy briefer doc comments
use crate::{NcDirect, NcError, NcResult, Notcurses, NCRESULT_ERR, NCRESULT_OK};

// Sleep, Render & Flush Macros ------------------------------------------------

/// Sleeps for `$ns` seconds + `$ms` milliseconds
/// + `$us` microseconds + `$ns` nanoseconds
#[macro_export]
macro_rules! sleep {
    ($s:expr) => {
        std::thread::sleep(std::time::Duration::from_secs($s));
    };

    ($s:expr, $ms:expr) => {
        std::thread::sleep(std::time::Duration::from_millis($s * 1000 + $ms));
    };
    ($s:expr, $ms:expr, $us:expr) => {
        std::thread::sleep(std::time::Duration::from_micros(
            $s * 1_000_000 + $ms * 1_000 + $us,
        ));
    };
    ($s:expr, $ms:expr, $us:expr, $ns:expr) => {
        std::thread::sleep(std::time::Duration::from_nanos(
            $s * 1_000_000_000 + $ms * 1_000_000 + $us * 1_000 + $ns,
        ));
    };
}

/// Notcurses.[render][Notcurses#method.render]\(`nc`\)? plus [sleep]!(`sleep_args`).
///
/// Renders the `$nc` [Notcurses] object and, if there's no error,
/// calls the sleep macro with the rest of the arguments.
///
/// Returns [NcResult].
#[macro_export]
macro_rules! rsleep {
    ($nc:expr, $( $sleep_args:expr),+ ) => {
        // Rust style, with methods & NcResult
        Notcurses::render($nc)?;
        sleep![$( $sleep_args ),+];
    };
    ($nc:expr, $( $sleep_args:expr),+ ,) => {
        rsleep![$nc, $( $sleep_args ),* ]
    };
}

/// NcDirect.[flush][NcDirect#method.flush]\(`ncd`\)? plus [sleep]!(`sleep_args`).
///
/// Flushes the `$ncd` [NcDirect] object and, if there's no error,
/// calls the sleep macro with the rest of the arguments.
///
/// Returns [NcResult].
#[macro_export]
macro_rules! fsleep {
    ($ncd:expr, $( $sleep_args:expr),+ ) => {
        // Rust style, with methods & NcResult
        NcDirect::flush($ncd)?;
        sleep![$( $sleep_args ),+];
    };
    ($ncd:expr, $( $sleep_args:expr),+ ,) => {
        rsleep![$ncd, $( $sleep_args ),* ]
    };
}

// String & Print Macros -------------------------------------------------------

/// Converts an `&str` into `*const c_char`.
#[macro_export]
macro_rules! cstring {
    ($s:expr) => {
        std::ffi::CString::new($s).unwrap().as_ptr();
    };
}

/// Converts an `&str` into `*mut c_char`.
#[macro_export]
macro_rules! cstring_mut {
    ($s:expr) => {
        std::ffi::CString::new($s).unwrap().into_raw();
    };
}

/// Converts a `*const c_char` into an `&str`.
#[macro_export]
macro_rules! rstring {
    ($s:expr) => {
        unsafe { std::ffi::CStr::from_ptr($s).to_str().unwrap() }
        // possible alternative
        // unsafe { std::ffi::CStr::from_ptr($s).to_string_lossy() }
    };
}

/// Wrapper around [libc::printf].
#[macro_export]
macro_rules! printf {
    ($s:expr) => {
        unsafe { libc::printf(cstring![$s]) }
    };
    ($s:expr $(, $opt:expr)*) => {
        unsafe { libc::printf(cstring![$s], $($opt),*) }
    };
}

// Error Wrappers Macros -------------------------------------------------------

/// Returns an Ok(<`$ok`>),
/// or an Err([NcError]) if `$res` < [NCRESULT_OK].
///
/// In other words:
/// Returns Ok(`$ok`) if `$res` >= [NCRESULT_OK], otherwise returns
/// Err([NcError]::[new][NcError#method.new](`$res`, `$msg`)).
///
/// `$ok` & `$msg` are optional. By default they will be the unit
/// type `()`, and an empty `&str` `""`, respectively.
#[macro_export]
macro_rules! error {
    ($res:expr, $msg:expr, $ok:expr) => {
        if $res >= crate::NCRESULT_OK {
            return Ok($ok);
        } else {
            return Err(crate::NcError::with_msg($res, $msg));
        }
    };
    ($res:expr, $msg:expr) => {
        error![$res, $msg, ()];
    };
    ($res:expr) => {
        error![$res, "", ()];
    };
}

/// Returns an Ok(&T) from a `*const T` pointer,
/// or an Err([NcError]) if the pointer is null.
///
/// In other words:
/// Returns Ok(&*`$ptr`) if `$ptr` != `null()`, otherwise returns
/// Err([NcError]]::[new][NcError#method.new]([NCRESULT_ERR], `$msg`)).
///
/// `$msg` is optional. By default it will be an empty `&str` `""`.
#[macro_export]
macro_rules! error_ref {
    ($ptr:expr, $msg:expr) => {
        if $ptr != core::ptr::null() {
            #[allow(unused_unsafe)]
            return Ok(unsafe { &*$ptr });
        } else {
            return Err(crate::NcError::with_msg(crate::NCRESULT_ERR, $msg));
        }
    };
    ($ptr:expr) => {
        error_ref![$ptr, ""];
    };
}

/// Returns an Ok(&mut T) from a `*mut T` pointer,
/// or an Err([NcError]) if the pointer is null.
///
/// In other words:
/// Returns Ok(&mut *`$ptr`) if `$ptr` != `null_mut()`, otherwise returns
/// Err([NcError]]::[new][NcError#method.new]([NCRESULT_ERR], `$msg`)).
///
/// `$msg` is optional. By default it will be an empty `&str` `""`.
#[macro_export]
macro_rules! error_ref_mut {
    ($ptr:expr, $msg:expr) => {
        if $ptr != core::ptr::null_mut() {
            #[allow(unused_unsafe)]
            return Ok(unsafe { &mut *$ptr });
        } else {
            return Err(crate::NcError::with_msg(crate::NCRESULT_ERR, $msg));
        }
    };
    ($ptr:expr) => {
        error_ref_mut![$ptr, ""];
    };
}

/// Returns an Ok(String) from a `*const` pointer to a C string,
/// or an Err([NcError]) if the pointer is null.
///
/// In other words:
/// Returns Ok((&*`$str`).to_string()) if `$str` != `null()`, otherwise returns
/// Err([NcError]]::[new][NcError#method.new]([NCRESULT_ERR], `$msg`)).
///
/// `$msg` is optional. By default it will be an empty `&str` `""`.
#[macro_export]
macro_rules! error_str {
    ($str:expr, $msg:expr) => {
        if $str != core::ptr::null_mut() {
            #[allow(unused_unsafe)]
            return Ok(unsafe { (&*$str).to_string() });
        } else {
            return Err(crate::NcError::with_msg(crate::NCRESULT_ERR, $msg));
        }
    };
    ($str:expr) => {
        error_str![$str, ""];
    };
}
