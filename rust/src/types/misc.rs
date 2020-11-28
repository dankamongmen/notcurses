//! Miscellaneous types and constants
//!

// error handling --------------------------------------------------------------

/// `i32` value used to return errors, when value < 0, (usually -1)
pub type NcResult = i32;

// time -----------------------------------------------------------------------

pub type NcTime = crate::bindings::bindgen::timespec;

// ncmetric --------------------------------------------------------------------

// TODO: clarify, update and visibilize doc-comments

// The number of columns is one fewer, as the STRLEN expressions must leave
// an extra byte open in case 'µ' (U+00B5, 0xC2 0xB5) shows up.

// This is the true number of columns;
//
// to set up a printf()-style maximum field width,
// you should use [IB]PREFIXFMT (see below).
pub const NCMETRIC_PREFIXCOLUMNS: u32 = crate::bindings::bindgen::PREFIXCOLUMNS;

// The maximum number of columns used by a mult == 1000 (standard)
// ncmetric() call.
pub const NCMETRIC_BPREFIXCOLUMNS: u32 = crate::bindings::bindgen::BPREFIXCOLUMNS;

// IPREFIXCOLUMNS is the maximum number of columns used by a mult == 1024
// (digital information) ncmetric().
pub const NCMETRIC_IPREFIXCOLUMNS: u32 = crate::bindings::bindgen::IPREFIXCOLUMNS;

//
// Does not include a '\0' (xxx.xxU)
pub const NCMETRIC_PREFIXSTRLEN: u32 = crate::bindings::bindgen::PREFIXSTRLEN;

// The maximum number of columns used by a mult == 1024 call making use of
// the 'i' suffix.
// Does not include a '\0' (xxxx.xxUi), i == prefix
pub const NCMETRIC_BPREFIXSTRLEN: u32 = crate::bindings::bindgen::BPREFIXSTRLEN;

// Does not include a '\0' (xxxx.xxU)
pub const NCMETRIC_IPREFIXSTRLEN: u32 = crate::bindings::bindgen::IPREFIXSTRLEN;

// TODO:?
// WCHAR_MAX_UTF8BYTES
