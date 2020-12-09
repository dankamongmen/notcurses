//! `NcResult` for error handling

/// `i32` value used to return errors, when value < 0, (usually -1).
/// See also [`NCRESULT_OK`] and [`NCRESULT_ERR`].
pub type NcResult = i32;

/// ERROR value, for the functions that return an [`NcResult`]
pub const NCRESULT_OK: i32 = 0;

/// OK value, for the functions that return [`NcResult`]
pub const NCRESULT_ERR: i32 = -1;
