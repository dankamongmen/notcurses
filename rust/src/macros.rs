//! Macros
//!
//! NOTE: Use full paths everywhere. Don't assume anything will be in scope.

#[allow(unused_imports)]
// enjoy briefer doc comments
use crate::{NcError, NCRESULT_ERR, NCRESULT_OK};

// General Utility Macros ------------------------------------------------------

/// Sleeps for `$ms` milliseconds.
#[macro_export]
macro_rules! sleep {
    ($ms:expr) => {
        std::thread::sleep(std::time::Duration::from_millis($ms));
    };
}

/// Renders the `$nc` [Notcurses][crate::Notcurses] object,
/// then sleeps for `$ms` milliseconds.
#[macro_export]
macro_rules! rsleep {
    ($nc:expr, $ms:expr) => {{
        // Rust style, with methods & NcResult
        crate::$nc.render();
        std::thread::sleep(std::time::Duration::from_millis($ms));
    }};
}

/// Renders the `$nc` [Notcurses][crate::Notcurses] object,
/// then sleeps for `$ms` milliseconds and returns the result of
/// [notcurses_render][crate::notcurses_render].
#[macro_export]
macro_rules! rsleep_c {
    ($nc:expr, $ms:expr) => {{
        // C style, with functions & NcIntResult
        let mut res: crate::NcIntResult = 0;
        unsafe {
            res = crate::notcurses_render($nc);
        }
        std::thread::sleep(std::time::Duration::from_millis($ms));
        res
    }};
}

/// Converts `&str` to `*mut CString`, for when `*const c_char` is needed.
#[macro_export]
macro_rules! cstring {
    ($s:expr) => {
        std::ffi::CString::new($s).unwrap().as_ptr();
    };
}

/// Simple wrapper around [libc::printf].
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
    ($res:expr, $ok:expr, $msg:expr) => {
        if $res >= crate::NCRESULT_OK {
            return Ok($ok);
        } else {
            return Err(crate::NcError::with_msg($res, $msg));
        }
    };
    ($res:expr, $ok:expr) => {
        error![$res, $ok, ""];
    };
    ($res:expr) => {
        error![$res, (), ""];
    };
}

/// Returns an Ok(&mut T) from a `*mut T` pointer,
/// or an Err([NcError]) if the pointer is null.
///
/// In other words:
/// Returns Ok(&mut *`$ptr`) if `$ptr` != `null()`, otherwise returns
/// Err([NcError]]::[new][NcError#method.new]([NCRESULT_ERR], `$msg`)).
///
/// `$msg` is optional. By default it will be an empty `&str` `""`.
#[macro_export]
macro_rules! error_ptr {
    ($ptr:expr, $msg:expr) => {
        if $ptr != core::ptr::null_mut() {
            return Ok(unsafe { &mut *$ptr });
        } else {
            return Err(crate::NcError::with_msg(crate::NCRESULT_ERR, $msg));
        }
    };
    ($ptr:expr) => {
        error![$ptr, (), ""];
    };
}

/// Returns an Ok([`String`]) from a `*const` pointer to a C string,
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
            return Ok(unsafe { (&*$str).to_string() });
        } else {
            return Err(crate::NcError::with_msg(crate::NCRESULT_ERR, $msg));
        }
    };
    ($str:expr) => {
        error![$str, (), ""];
    };
}
