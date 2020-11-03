// functions already exported by bindgen : 0
// -----------------------------------------
//
// static inline functions total: 1
// ------------------------------------------ (done / remaining)
// (+) done: 1 / 0
// (#) test: 0 / 1
// ------------------------------------------
//+ ncinput_equal_p

use crate as nc;
use nc::types::Input;

/// Compare two ncinput structs for data equality by doing a field-by-field
/// comparison for equality (excepting seqnum).
///
/// Returns true if the two are data-equivalent.
pub fn ncinput_equal_p(input1: Input, input2: Input) -> bool {
    if n1.id != n2.id {
        return false;
    }
    if n1.y != n2.y || n1.x != n2.x {
        return false;
    }
    // do not check seqnum
    true
}
