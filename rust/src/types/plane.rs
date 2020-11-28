#[allow(unused_imports)] // for docblocks
use crate::NCCELL_ALPHA_BLEND;

// NcPlane
/// Fundamental drawing surface.
///
/// Unites a:
/// - CellMatrix
/// - EgcPool
///
/// `type in C: ncplane (struct)`
///
///
/// ## Piles
///
/// A single notcurses context is made up of one or more piles.
///
/// A pile is a set of one or more ncplanes, including the partial orderings
/// made up of their binding and z-axis pointers.
///
/// A pile has a top and bottom ncplane (this might be a single plane),
/// and one or more root planes (planes which are bound to themselves).
///
/// Multiple threads can concurrently operate on distinct piles, even changing
/// one while rendering another.
///
/// Each plane is part of one and only one pile. By default, a plane is part of
/// the same pile containing that plane to which it is bound.
///
/// If ncpile_create is used in the place of ncplane_create, the returned plane
/// becomes the root plane, top, and bottom of a new pile.  As a root plane,
/// it is bound to itself.
///
/// A new pile can also be created by reparenting a plane to itself,
/// though if the plane is already a root plane, this is a no-op.
///
/// When a plane is moved to a different pile (whether new or preexisting),
/// any planes which were bound to it are rebound to its previous parent.
/// If the plane was a root plane of some pile, any bound planes become root
/// planes. The new plane is placed immediately atop its new parent on its new
/// pile's z-axis. When ncplane_reparent_family() is used, all planes bound to
/// the reparented plane are moved along with it. Their relative z-order is maintained.
///
pub type NcPlane = crate::bindings::bindgen::ncplane;

/// Options struct for [`NcPlane`]
pub type NcPlaneOptions = crate::bindings::bindgen::ncplane_options;

/// Horizontal alignment relative to the parent plane. Set alignment in 'x'.
pub const NCPLANE_OPTION_HORALIGNED: u64 =
    crate::bindings::bindgen::NCPLANE_OPTION_HORALIGNED as u64;

/// I/O wrapper to dump file descriptor to [`NcPlane`]
///
/// `type in C: ncfdplane (struct)`
pub type NcFdPlane = crate::bindings::bindgen::ncfdplane;

/// Options struct for [`NcFdPlane`]
///
/// `type in C: ncplane_options (struct)`
pub type NcFdPlaneOptions = crate::bindings::bindgen::ncfdplane_options;

/// Alignment within a plane or terminal.
/// Left/right-justified, or centered.
pub type NcAlign = crate::bindings::bindgen::ncalign_e;

/// Left alignment within an [`NcPlane`] or terminal.
pub const NCALIGN_LEFT: NcAlign = crate::bindings::bindgen::ncalign_e_NCALIGN_LEFT;

/// Right alignment within an [`NcPlane`] or terminal.
pub const NCALIGN_RIGHT: NcAlign = crate::bindings::bindgen::ncalign_e_NCALIGN_RIGHT;

/// Center alignment within an [`NcPlane`] or terminal.
pub const NCALIGN_CENTER: NcAlign = crate::bindings::bindgen::ncalign_e_NCALIGN_CENTER;

/// Do not align an [`NcPlane`] or terminal.
pub const NCALIGN_UNALIGNED: NcAlign = crate::bindings::bindgen::ncalign_e_NCALIGN_UNALIGNED;

/// Blitter Mode (`NCBLIT_*`)
///
/// We never blit full blocks, but instead spaces (more efficient) with the
/// background set to the desired foreground.
pub type NcBlitter = crate::bindings::bindgen::ncblitter_e;

/// [`NcBlitter`] mode using: space, compatible with ASCII
pub const NCBLIT_1x1: NcBlitter = crate::bindings::bindgen::ncblitter_e_NCBLIT_1x1;

/// [`NcBlitter`] mode using: halves + 1x1 (space)
/// ▄▀
pub const NCBLIT_2x1: NcBlitter = crate::bindings::bindgen::ncblitter_e_NCBLIT_2x1;

/// [`NcBlitter`] mode using: quadrants + 2x1
/// ▗▐ ▖▀▟▌▙
pub const NCBLIT_2x2: NcBlitter = crate::bindings::bindgen::ncblitter_e_NCBLIT_2x2;

/// [`NcBlitter`] mode using: sextants
/// 🬀🬁🬂🬃🬄🬅🬆🬇🬈🬉🬊🬋🬌🬍🬎🬏🬐🬑🬒🬓🬔🬕🬖🬗🬘🬙🬚🬛🬜🬝🬞🬟🬠🬡🬢🬣🬤🬥🬦🬧🬨🬩🬪🬫🬬🬭🬮🬯🬰🬱🬲🬳🬴🬵🬶🬷🬸🬹🬺🬻
pub const NCBLIT_3x2: NcBlitter = crate::bindings::bindgen::ncblitter_e_NCBLIT_3x2;

/// [`NcBlitter`] mode using: four vertical levels
/// █▆▄▂
pub const NCBLIT_4x1: NcBlitter = crate::bindings::bindgen::ncblitter_e_NCBLIT_4x1;

/// [`NcBlitter`] mode using: eight vertical levels
/// █▇▆▅▄▃▂▁
pub const NCBLIT_8x1: NcBlitter = crate::bindings::bindgen::ncblitter_e_NCBLIT_8x1;

/// [`NcBlitter`] mode using: 4 rows, 2 cols (braille)
/// ⡀⡄⡆⡇⢀⣀⣄⣆⣇⢠⣠⣤⣦⣧⢰⣰⣴⣶⣷⢸⣸⣼⣾⣿
pub const NCBLIT_BRAILLE: NcBlitter = crate::bindings::bindgen::ncblitter_e_NCBLIT_BRAILLE;

/// [`NcBlitter`] mode where the blitter is automatically chosen
pub const NCBLIT_DEFAULT: NcBlitter = crate::bindings::bindgen::ncblitter_e_NCBLIT_DEFAULT;

/// [`NcBlitter`] mode (not yet implemented)
pub const NCBLIT_SIXEL: NcBlitter = crate::bindings::bindgen::ncblitter_e_NCBLIT_SIXEL;

/// How to scale an [`NcVisual`] during rendering
///
/// - NCSCALE_NONE will apply no scaling.
/// - NCSCALE_SCALE scales a visual to the plane's size,
///   maintaining aspect ratio.
/// - NCSCALE_STRETCH stretches and scales the image in an
///   attempt to fill the entirety of the plane.
///
pub type NcScale = crate::bindings::bindgen::ncscale_e;
/// Maintain original size
pub const NCSCALE_NONE: NcScale = crate::bindings::bindgen::ncscale_e_NCSCALE_NONE;
/// Maintain aspect ratio
pub const NCSCALE_SCALE: NcScale = crate::bindings::bindgen::ncscale_e_NCSCALE_SCALE;
/// Throw away aspect ratio
pub const NCSCALE_STRETCH: NcScale = crate::bindings::bindgen::ncscale_e_NCSCALE_STRETCH;

/// A visual bit of multimedia opened with LibAV|OIIO
pub type NcVisual = crate::bindings::bindgen::ncvisual;
/// Options struct for [`NcVisual`]
pub type NcVisualOptions = crate::bindings::bindgen::ncvisual_options;

/// Use [`NCCELL_ALPHA_BLEND`] with visual
pub const NCVISUAL_OPTION_BLEND: u32 = crate::bindings::bindgen::NCVISUAL_OPTION_BLEND;

/// Fail rather than degrade
pub const NCVISUAL_OPTION_NODEGRADE: u32 = crate::bindings::bindgen::NCVISUAL_OPTION_NODEGRADE;
