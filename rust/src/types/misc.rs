//! Miscellaneous types and constants
//!

// error handling --------------------------------------------------------------

/// `i32` value used to return errors, when value < 0, (usually -1)
pub type IntResult = i32;

// ncmetric --------------------------------------------------------------------

// TODO: clarify, update and visibilize doc-comments

// The number of columns is one fewer, as the STRLEN expressions must leave
// an extra byte open in case 'µ' (U+00B5, 0xC2 0xB5) shows up.

// This is the true number of columns;
//
// to set up a printf()-style maximum field width,
// you should use [IB]PREFIXFMT (see below).
pub const PREFIXCOLUMNS: u32 = crate::bindings::PREFIXCOLUMNS;

// The maximum number of columns used by a mult == 1000 (standard)
// ncmetric() call.
pub const BPREFIXCOLUMNS: u32 = crate::bindings::BPREFIXCOLUMNS;

// IPREFIXCOLUMNS is the maximum number of columns used by a mult == 1024
// (digital information) ncmetric().
pub const IPREFIXCOLUMNS: u32 = crate::bindings::IPREFIXCOLUMNS;

//
// Does not include a '\0' (xxx.xxU)
pub const PREFIXSTRLEN: u32 = crate::bindings::PREFIXSTRLEN;

// The maximum number of columns used by a mult == 1024 call making use of
// the 'i' suffix.
// Does not include a '\0' (xxxx.xxUi), i == prefix
pub const BPREFIXSTRLEN: u32 = crate::bindings::BPREFIXSTRLEN;

// Does not include a '\0' (xxxx.xxU)
pub const IPREFIXSTRLEN: u32 = crate::bindings::IPREFIXSTRLEN;

// TODO:?
// WCHAR_MAX_UTF8BYTES
