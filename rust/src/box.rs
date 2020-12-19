//! `NcBoxMask`

/// Controls the drawing of borders, gradients and corners.
///
/// NcBoxMax is defined in the least significant byte, where bits [3, 0] are
/// are a border mask, and bits [7, 4] are a gradient mask.
///
/// The drawing of the corners is defined in the second byte.
///
/// ## Diagram
///
/// ```txt
/// NCBOXMASK_TOP    0x0001  0b00000001
/// NCBOXMASK_RIGHT  0x0002  0b00000010
/// NCBOXMASK_BOTTOM 0x0004  0b00000100
/// NCBOXMASK_LEFT   0x0008  0b00001000
///
/// NCBOXGRAD_TOP    0x0010  0b00010000
/// NCBOXGRAD_RIGHT  0x0020  0b00100000
/// NCBOXGRAD_BOTTOM 0x0040  0b01000000
/// NCBOXGRAD_LEFT   0x0080  0b10000000
///
/// NCBOXCORNER_MASK  0x0300  0b00000111_00000000
///
/// NCBOXCORNER_SHIFT 8
/// ```
///
/// ## Bit masks
///
/// - [NCBOXMASK_TOP]
/// - [NCBOXMASK_RIGHT]
/// - [NCBOXMASK_BOTTOM]
/// - [NCBOXMASK_LEFT]
///
/// - [NCBOXGRAD_TOP]
/// - [NCBOXGRAD_RIGHT]
/// - [NCBOXGRAD_BOTTOM]
/// - [NCBOXGRAD_LEFT]
///
/// - [NCBOXCORNER_MASK]
/// - [NCBOXCORNER_SHIFT]
///
pub type NcBoxMask = u32;

/// [NcBoxMask] top gradient mask.
pub const NCBOXGRAD_TOP: NcBoxMask = crate::bindings::ffi::NCBOXGRAD_TOP;
/// [NcBoxMask] right gradient mask.
pub const NCBOXGRAD_RIGHT: NcBoxMask = crate::bindings::ffi::NCBOXGRAD_RIGHT;
/// [NcBoxMask] bottom gradient mask.
pub const NCBOXGRAD_BOTTOM: NcBoxMask = crate::bindings::ffi::NCBOXGRAD_BOTTOM;
/// [NcBoxMask] left gradient mask.
pub const NCBOXGRAD_LEFT: NcBoxMask = crate::bindings::ffi::NCBOXGRAD_LEFT;

/// [NcBoxMask] top border mask.
pub const NCBOXMASK_TOP: NcBoxMask = crate::bindings::ffi::NCBOXMASK_TOP;
/// [NcBoxMask] right border mask.
pub const NCBOXMASK_RIGHT: NcBoxMask = crate::bindings::ffi::NCBOXMASK_RIGHT;
/// [NcBoxMask] bottom border mask.
pub const NCBOXMASK_BOTTOM: NcBoxMask = crate::bindings::ffi::NCBOXMASK_BOTTOM;
/// [NcBoxMask] left border mask.
pub const NCBOXMASK_LEFT: NcBoxMask = crate::bindings::ffi::NCBOXMASK_LEFT;

/// [NcBoxMask] corner mask to control the drawing of boxes corners.
///
/// By default, vertexes are drawn whether their connecting edges are drawn
/// or not. The value of the bits control this, and are interpreted as the
/// number of connecting edges necessary to draw a given corner.
///
/// At 0 (the default), corners are always drawn. At 3, corners are never drawn
/// (since at most 2 edges can touch a box's corner).
pub const NCBOXCORNER_MASK: NcBoxMask = crate::bindings::ffi::NCBOXCORNER_MASK;

/// The number of bits [NCBOXCORNER_MASK] is shifted in [NcBoxMask].
pub const NCBOXCORNER_SHIFT: NcBoxMask = crate::bindings::ffi::NCBOXCORNER_SHIFT;
