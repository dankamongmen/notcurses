// functions already exported by bindgen : 0
// -----------------------------------------
//
// static inline functions total: 1
// ------------------------------------------ (done / remaining)
// (+) done: 1 / 0
// (#) test: 0 / 1
// ------------------------------------------
//+ ncinput_equal_p

/// Reads and decodes input events
///
/// Reads from stdin and decodes the input to stdout,
/// including synthesized events and mouse events.
///
/// To exit, generate EOF (usually Ctrl+‘d’).
pub type NcInput = crate::bindings::bindgen::ncinput;

/// Compare two ncinput structs for data equality by doing a field-by-field
/// comparison for equality (excepting seqnum).
///
/// Returns true if the two are data-equivalent.
pub fn ncinput_equal_p(n1: NcInput, n2: NcInput) -> bool {
    if n1.id != n2.id {
        return false;
    }
    if n1.y != n2.y || n1.x != n2.x {
        return false;
    }
    // do not check seqnum
    true
}

impl NcInput {
    pub fn new() -> NcInput {
        NcInput {
            id: 0,
            y: 0,
            x: 0,
            alt: false,
            shift: false,
            ctrl: false,
            seqnum: 0,
        }
    }
}
