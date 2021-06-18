//! `NcCapabilities`

use crate::NcPalette;

use core::mem::size_of;

/// Capabilities, derived from terminfo, environment variables, and queries.
pub type NcCapabilities = crate::bindings::ffi::nccapabilities;

/// Can we set the "hardware" palette?
///
/// Requires the "ccc" terminfo capability, and that the number of colors
/// supported is at least the size of our `NcPalette` structure.
#[inline]
pub fn nccapability_canchangecolor(caps: &NcCapabilities) -> bool {
    if !caps.can_change_colors {
        return false;
    }
    // CHECK this does the same as:
    // if(caps.colors < sizeof(p->chans) / sizeof(*p->chans)){
    //
    if (caps.colors as usize) < size_of::<NcPalette>() / size_of::<u32>() {
        return false;
    }
    true
}
