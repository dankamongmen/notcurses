//! Error handling with `Error`, `NcResult` & `NcIntResult` for error handling

use std::{self, error, fmt};

/// The [`i32`] value used to return errors by the underlying C API.
///
/// A value < 0 means error, (usually -1).
///
/// # Defined constants:
///
/// - [`NCRESULT_OK`]
/// - [`NCRESULT_ERR`]
/// - [`NCRESULT_MAX`]
pub type NcIntResult = i32;

/// OK value, for the functions that return [`NcIntResult`].
pub const NCRESULT_OK: i32 = 0;

/// ERROR value, for the functions that return an [`NcIntResult`].
pub const NCRESULT_ERR: i32 = -1;

/// MAX value, for the functions that return [`NcIntResult`].
pub const NCRESULT_MAX: i32 = i32::MAX;

/// The error type for the Rust methods API.
#[derive(Debug, Clone, Default)]
pub struct NcError {
    /// [NcIntResult].
    pub int: i32,
    pub msg: String,
}

impl fmt::Display for NcError {
    fn fmt(&self, f: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        write!(f, "NcError {}: {}", self.int, self.msg)
    }
}

impl error::Error for NcError {
    fn description(&self) -> &str {
        &self.msg
    }
}

impl NcError {
    /// New NcError.
    pub fn new(int: NcIntResult) -> Self {
        Self {
            int,
            ..Default::default()
        }
    }
    /// New NcError with message.
    pub fn with_msg(int: NcIntResult, msg: &str) -> Self {
        Self {
            int,
            msg: msg.to_string(),
        }
    }
}

/// The result type for the Rust methods API.
pub type NcResult<T> = Result<T, NcError>;
