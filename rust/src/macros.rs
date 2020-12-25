//! Macros

// General Utility Macros ------------------------------------------------------

/// Sleeps for `$ms` milliseconds.
#[macro_export]
macro_rules! sleep {
    ($ms:expr) => {
        std::thread::sleep(std::time::Duration::from_millis($ms));
    };
}

/// Renders the [Notcurses][crate::Notcurses] object, then sleeps for `$ms`
/// milliseconds and returns the result of [notcurses_render][crate::notcurses_render].
#[macro_export]
macro_rules! rsleep {
    ($nc:expr, $ms:expr) => {{
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

/// Returns Ok(`$ok`) if `$res` >= [NCRESULT_OK][crate::NCRESULT_OK],
/// otherwise returns
/// Err([NcError][crate::NcError]::[new][crate::NcError#method.new](`$res`, `$msg`)).
///
/// `$ok` & `$msg` are optional. By default they will be the unit
/// type `()`, and an empty `&str` `""`, respectively.
///
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
