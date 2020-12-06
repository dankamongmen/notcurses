//! `NcPlane`

// functions already exported by bindgen : 105
// ------------------------------------------
// (#) 13 / 92 unit tests
// (W) 1 wrapped as a method or function
// ------------------------------------------
//   ncpile_bottom
// # ncpile_create
//   ncpile_rasterize
//   ncpile_render
//   ncpile_top
//   ncplane_above
//   ncplane_at_cursor
//   ncplane_at_yx
//   ncplane_base
//   ncplane_below
//   ncplane_box
//   ncplane_center_abs
// # ncplane_channels
//   ncplane_contents
//   ncplane_create
// # ncplane_cursor_move_yx
//W# ncplane_cursor_yx
//   ncplane_destroy
//W# ncplane_dim_yx
//   ncplane_dup
// # ncplane_erase
//   ncplane_fadein
//   ncplane_fadein_iteration
//   ncplane_fadeout
//   ncplane_fadeout_iteration
//   ncplane_format
//   ncplane_gradient
//   ncplane_greyscale
//   ncplane_highgradient
//   ncplane_highgradient_sized
//   ncplane_hline_interp
// # ncplane_home
//   ncplane_mergedown
//   ncplane_mergedown_simple
//   ncplane_move_above
//   ncplane_move_below
//   ncplane_move_bottom
//   ncplane_move_top
//   ncplane_move_yx
//   ncplane_new
// # ncplane_notcurses
// # ncplane_notcurses_const
//   ncplane_off_styles
//   ncplane_on_styles
//   ncplane_parent
//   ncplane_parent_const
//   ncplane_polyfill_yx
//   ncplane_pulse
//   ncplane_putchar_stained
//   ncplane_putc_yx
//   ncplane_putegc_stained
//   ncplane_putegc_yx
//   ncplane_putnstr_aligned
//   ncplane_putnstr_yx
//   ncplane_putstr_aligned
//   ncplane_putstr_stained
//   ncplane_putstr_yx
//   ncplane_puttext
//   ncplane_putwegc_stained
//   ncplane_putwstr_stained
//   ncplane_qrcode
//   ncplane_reparent
//   ncplane_reparent_family
// # ncplane_resize
//   ncplane_resizecb
//   ncplane_resize_realign
//   ncplane_rgba
//   ncplane_rotate_ccw
//   ncplane_rotate_cw
//   ncplane_set_base
//   ncplane_set_base_cell
// # ncplane_set_bchannel
//   ncplane_set_bg_alpha
//   ncplane_set_bg_default
//   ncplane_set_bg_palindex
//   ncplane_set_bg_rgb
//   ncplane_set_bg_rgb8
//   ncplane_set_bg_rgb8_clipped
// # ncplane_set_channels
// # ncplane_set_fchannel
//   ncplane_set_fg_alpha
//   ncplane_set_fg_default
//   ncplane_set_fg_palindex
//   ncplane_set_fg_rgb
//   ncplane_set_fg_rgb8
//   ncplane_set_fg_rgb8_clipped
//   ncplane_set_resizecb
//   ncplane_set_scrolling
//   ncplane_set_styles
//   ncplane_set_userptr
//   ncplane_stain
//   ncplane_styles
//   ncplane_styles_off
//   ncplane_styles_on
//   ncplane_styles_set
//   ncplane_translate
//   ncplane_translate_abs
//   ncplane_userptr
//   ncplane_vline_interp
//   ncplane_vprintf_aligned
//   ncplane_vprintf_stained
//   ncplane_vprintf_yx
//   ncplane_x
//   ncplane_y
//   ncplane_yx
//
// functions manually reimplemented: 42
// ------------------------------------------
// (X) wont:  8
// (+) done: 34 / 0
// (#) test:  5 / 29
// ------------------------------------------
// + ncplane_align
// + ncplane_at_cursor_cell
// + ncplane_at_yx_cell
// + ncplane_bchannel
// + ncplane_bg_alpha
// # ncplane_bg_default_p
// + ncplane_bg_rgb
// + ncplane_bg_rgb8
// + ncplane_box_sized
//W# ncplane_dim_x
//W# ncplane_dim_y
// + ncplane_double_box
// + ncplane_double_box_sized
// + ncplane_fchannel
// + ncplane_fg_alpha
// # ncplane_fg_default_p
// + ncplane_fg_rgb
// + ncplane_fg_rgb8
// + ncplane_gradient_sized       // u64|u32 https://github.com/dankamongmen/notcurses/issues/920
// + ncplane_hline
// + ncplane_perimeter
// + ncplane_perimeter_double
// + ncplane_perimeter_rounded
// + ncplane_putc
// + ncplane_putchar
// + ncplane_putchar_yx
// + ncplane_putegc
// + ncplane_putnstr
//W+ ncplane_putstr
// X ncplane_putwc                // I don't think these will be needed from Rust. See:
// X ncplane_putwc_stained
// X ncplane_putwc_yx             // https://locka99.gitbooks.io/a-guide-to-porting-c-to-rust/content/features_of_rust/strings.html
// X ncplane_putwegc              //
// X ncplane_putwegc_yx           //
// X ncplane_putwstr              //
// X ncplane_putwstr_aligned      //
// X ncplane_putwstr_yx           //
// # ncplane_resize_simple
// + ncplane_rounded_box
// + ncplane_rounded_box_sized
// + ncplane_vline
// + ncplane_vprintf
//
// NOTE: TODO: Still remains all the ncplane_printf* functions/macros (at the end)

#[cfg(test)]
mod test;

mod methods;
mod reimplemented;
pub use reimplemented::*;

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
pub type NcPlane = crate::bindings::ffi::ncplane;

/// Options struct for [`NcPlane`]
pub type NcPlaneOptions = crate::bindings::ffi::ncplane_options;

/// Horizontal alignment relative to the parent plane. Set alignment in 'x'.
pub const NCPLANE_OPTION_HORALIGNED: u64 = crate::bindings::ffi::NCPLANE_OPTION_HORALIGNED as u64;

/// I/O wrapper to dump file descriptor to [`NcPlane`]
///
/// `type in C: ncfdplane (struct)`
pub type NcFdPlane = crate::bindings::ffi::ncfdplane;

/// Options struct for [`NcFdPlane`]
///
/// `type in C: ncplane_options (struct)`
pub type NcFdPlaneOptions = crate::bindings::ffi::ncfdplane_options;

/// Alignment within a plane or terminal.
/// Left/right-justified, or centered.
pub type NcAlign = crate::bindings::ffi::ncalign_e;

/// Left alignment within an [`NcPlane`] or terminal.
pub const NCALIGN_LEFT: NcAlign = crate::bindings::ffi::ncalign_e_NCALIGN_LEFT;

/// Right alignment within an [`NcPlane`] or terminal.
pub const NCALIGN_RIGHT: NcAlign = crate::bindings::ffi::ncalign_e_NCALIGN_RIGHT;

/// Center alignment within an [`NcPlane`] or terminal.
pub const NCALIGN_CENTER: NcAlign = crate::bindings::ffi::ncalign_e_NCALIGN_CENTER;

/// Do not align an [`NcPlane`] or terminal.
pub const NCALIGN_UNALIGNED: NcAlign = crate::bindings::ffi::ncalign_e_NCALIGN_UNALIGNED;

/// Blitter Mode (`NCBLIT_*`)
///
/// We never blit full blocks, but instead spaces (more efficient) with the
/// background set to the desired foreground.
///
/// ## Modes
///
/// - [`NCBLIT_1x1`]
/// - [`NCBLIT_2x1`]
/// - [`NCBLIT_2x2`]
/// - [`NCBLIT_3x2`]
/// - [`NCBLIT_4x1`]
/// - [`NCBLIT_8x1`]
/// - [`NCBLIT_BRAILLE`]
/// - [`NCBLIT_DEFAULT`]
/// - [`NCBLIT_SIXEL`]
///
pub type NcBlitter = crate::bindings::ffi::ncblitter_e;

/// [`NcBlitter`] mode using: space, compatible with ASCII
pub const NCBLIT_1x1: NcBlitter = crate::bindings::ffi::ncblitter_e_NCBLIT_1x1;

/// [`NcBlitter`] mode using: halves + 1x1 (space)
/// ▄▀
pub const NCBLIT_2x1: NcBlitter = crate::bindings::ffi::ncblitter_e_NCBLIT_2x1;

/// [`NcBlitter`] mode using: quadrants + 2x1
/// ▗▐ ▖▀▟▌▙
pub const NCBLIT_2x2: NcBlitter = crate::bindings::ffi::ncblitter_e_NCBLIT_2x2;

/// [`NcBlitter`] mode using: sextants
/// 🬀🬁🬂🬃🬄🬅🬆🬇🬈🬉🬊🬋🬌🬍🬎🬏🬐🬑🬒🬓🬔🬕🬖🬗🬘🬙🬚🬛🬜🬝🬞🬟🬠🬡🬢🬣🬤🬥🬦🬧🬨🬩🬪🬫🬬🬭🬮🬯🬰🬱🬲🬳🬴🬵🬶🬷🬸🬹🬺🬻
pub const NCBLIT_3x2: NcBlitter = crate::bindings::ffi::ncblitter_e_NCBLIT_3x2;

/// [`NcBlitter`] mode using: four vertical levels
/// █▆▄▂
pub const NCBLIT_4x1: NcBlitter = crate::bindings::ffi::ncblitter_e_NCBLIT_4x1;

/// [`NcBlitter`] mode using: eight vertical levels
/// █▇▆▅▄▃▂▁
pub const NCBLIT_8x1: NcBlitter = crate::bindings::ffi::ncblitter_e_NCBLIT_8x1;

/// [`NcBlitter`] mode using: 4 rows, 2 cols (braille)
/// ⡀⡄⡆⡇⢀⣀⣄⣆⣇⢠⣠⣤⣦⣧⢰⣰⣴⣶⣷⢸⣸⣼⣾⣿
pub const NCBLIT_BRAILLE: NcBlitter = crate::bindings::ffi::ncblitter_e_NCBLIT_BRAILLE;

/// [`NcBlitter`] mode where the blitter is automatically chosen
pub const NCBLIT_DEFAULT: NcBlitter = crate::bindings::ffi::ncblitter_e_NCBLIT_DEFAULT;

/// [`NcBlitter`] mode (not yet implemented)
pub const NCBLIT_SIXEL: NcBlitter = crate::bindings::ffi::ncblitter_e_NCBLIT_SIXEL;
