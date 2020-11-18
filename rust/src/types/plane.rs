// NcPlane
/// Fundamental drawing surface.
///
/// Unites a:
///
/// - CellMatrix
/// - EgcPool
///
/// `type in C: ncplane (struct)`
pub type NcPlane = crate::ncplane;

/// Options struct for [`NcPlane`](type.NcPlane.html)
pub type NcPlaneOptions = crate::ncplane_options;

/// Horizontal alignment relative to the parent plane. Set alignment in 'x'.
pub const NCPLANE_OPTION_HORALIGNED: u64 = crate::bindings::NCPLANE_OPTION_HORALIGNED as u64;

/// I/O wrapper to dump file descriptor to [`NcPlane`](type.NcPlane.html)
///
/// `type in C: ncfdplane (struct)`
pub type NcFdPlane = crate::ncfdplane;

/// Options struct for [`NcFdPlane`](type.NcFdPlane.html)
///
/// `type in C: ncplane_options (struct)`
pub type NcFdPlaneOptions = crate::ncfdplane_options;

/// Alignment within a plane or terminal.
/// Left/right-justified, or centered.
pub type NcAlign = crate::ncalign_e;
/// Align an NcPlane or NcTerm
pub const NCALIGN_LEFT: NcAlign = crate::ncalign_e_NCALIGN_LEFT;
///
pub const NCALIGN_RIGHT: NcAlign = crate::ncalign_e_NCALIGN_RIGHT;
///
pub const NCALIGN_CENTER: NcAlign = crate::ncalign_e_NCALIGN_CENTER;
///
pub const NCALIGN_UNALIGNED: NcAlign = crate::ncalign_e_NCALIGN_UNALIGNED;

/// Blitter Mode (`NCBLIT_*`)
///
/// We never blit full blocks, but instead spaces (more efficient) with the
/// background set to the desired foreground.
pub type NcBlitter = crate::ncblitter_e;

/// space, compatible with ASCII
pub const NCBLIT_1x1: NcBlitter = crate::ncblitter_e_NCBLIT_1x1;

/// halves + 1x1 (space)
/// ▄▀
pub const NCBLIT_2x1: NcBlitter = crate::ncblitter_e_NCBLIT_2x1;

/// quadrants + 2x1
/// ▗▐ ▖▀▟▌▙
pub const NCBLIT_2x2: NcBlitter = crate::ncblitter_e_NCBLIT_2x2;

/// sextants (NOT 2x2)
/// 🬀🬁🬂🬃🬄🬅🬆🬇🬈🬉🬊🬋🬌🬍🬎🬏🬐🬑🬒🬓🬔🬕🬖🬗🬘🬙🬚🬛🬜🬝🬞🬟🬠🬡🬢🬣🬤🬥🬦🬧🬨🬩🬪🬫🬬🬭🬮🬯🬰🬱🬲🬳🬴🬵🬶🬷🬸🬹🬺🬻
pub const NCBLIT_3x2: NcBlitter = crate::ncblitter_e_NCBLIT_3x2;

/// four vertical levels
/// █▆▄▂
pub const NCBLIT_4x1: NcBlitter = crate::ncblitter_e_NCBLIT_4x1;

/// eight vertical levels
/// █▇▆▅▄▃▂▁
pub const NCBLIT_8x1: NcBlitter = crate::ncblitter_e_NCBLIT_8x1;

/// 4 rows, 2 cols (braille)
/// ⡀⡄⡆⡇⢀⣀⣄⣆⣇⢠⣠⣤⣦⣧⢰⣰⣴⣶⣷⢸⣸⣼⣾⣿
pub const NCBLIT_BRAILLE: NcBlitter = crate::ncblitter_e_NCBLIT_BRAILLE;

/// the blitter is automatically chosen
pub const NCBLIT_DEFAULT: NcBlitter = crate::ncblitter_e_NCBLIT_DEFAULT;

/// not yet implemented
pub const NCBLIT_SIXEL: NcBlitter = crate::ncblitter_e_NCBLIT_SIXEL;

/// How to scale an [`NcVisual`](type.NcVisual.html) during rendering
///
/// - NCSCALE_NONE will apply no scaling.
/// - NCSCALE_SCALE scales a visual to the plane's size,
///   maintaining aspect ratio.
/// - NCSCALE_STRETCH stretches and scales the image in an
///   attempt to fill the entirety of the plane.
///
pub type NcScale = crate::ncscale_e;
/// Maintain original size
pub const NCSCALE_NONE: NcScale = crate::ncscale_e_NCSCALE_NONE;
/// Maintain aspect ratio
pub const NCSCALE_SCALE: NcScale = crate::ncscale_e_NCSCALE_SCALE;
/// Throw away aspect ratio
pub const NCSCALE_STRETCH: NcScale = crate::ncscale_e_NCSCALE_STRETCH;

/// A visual bit of multimedia opened with LibAV|OIIO
pub type NcVisual = crate::ncvisual;
/// Options struct for [`NcVisual`](type.NcVisual.html)
pub type NcVisualOptions = crate::ncvisual_options;

/// Use [`CELL_ALPHA_BLEND`](constant.CELL_ALPHA_BLEND.html) with visual
pub const NCVISUAL_OPTION_BLEND: u32 = crate::bindings::NCVISUAL_OPTION_BLEND;

/// Fail rather than degrade
pub const NCVISUAL_OPTION_NODEGRADE: u32 = crate::bindings::NCVISUAL_OPTION_NODEGRADE;
