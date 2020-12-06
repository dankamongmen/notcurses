//! `NcMetric`

// TODO: clarify, update and visibilize doc-comments

// The number of columns is one fewer, as the STRLEN expressions must leave
// an extra byte open in case 'µ' (U+00B5, 0xC2 0xB5) shows up.

// This is the true number of columns;
//
// to set up a printf()-style maximum field width,
// you should use [IB]PREFIXFMT (see below).
pub const NCMETRIC_PREFIXCOLUMNS: u32 = crate::bindings::ffi::PREFIXCOLUMNS;

// The maximum number of columns used by a mult == 1000 (standard)
// ncmetric() call.
pub const NCMETRIC_BPREFIXCOLUMNS: u32 = crate::bindings::ffi::BPREFIXCOLUMNS;

// IPREFIXCOLUMNS is the maximum number of columns used by a mult == 1024
// (digital information) ncmetric().
pub const NCMETRIC_IPREFIXCOLUMNS: u32 = crate::bindings::ffi::IPREFIXCOLUMNS;

//
// Does not include a '\0' (xxx.xxU)
pub const NCMETRIC_PREFIXSTRLEN: u32 = crate::bindings::ffi::PREFIXSTRLEN;

// The maximum number of columns used by a mult == 1024 call making use of
// the 'i' suffix.
// Does not include a '\0' (xxxx.xxUi), i == prefix
pub const NCMETRIC_BPREFIXSTRLEN: u32 = crate::bindings::ffi::BPREFIXSTRLEN;

// Does not include a '\0' (xxxx.xxU)
pub const NCMETRIC_IPREFIXSTRLEN: u32 = crate::bindings::ffi::IPREFIXSTRLEN;

// TODO:?
// WCHAR_MAX_UTF8BYTES
