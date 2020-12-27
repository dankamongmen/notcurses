//! `NcPlane`

// functions already exported by bindgen : 108 (5 + 103)
// -----------------------------------------------------
// (X) wont:  6
// (D) depr:  4
// (#) test: 13
// (W) wrap: 77 of 98
// -------------------------------------------
//W  ncpile_bottom
//W# ncpile_create
//W  ncpile_rasterize
//W  ncpile_render
//W  ncpile_top
//W  ncplane_above
//W  ncplane_at_cursor
//W  ncplane_at_cursor_cell
//W  ncplane_at_yx
//W  ncplane_at_yx_cell
//W  ncplane_base
//W  ncplane_below
//W  ncplane_box
//W  ncplane_center_abs
//W# ncplane_channels
//W  ncplane_contents
//W  ncplane_create
//W# ncplane_cursor_move_yx
//W# ncplane_cursor_yx
//W  ncplane_destroy
//W# ncplane_dim_yx
//W  ncplane_dup
//W# ncplane_erase
//W  ncplane_fadein
//W  ncplane_fadein_iteration
//W  ncplane_fadeout
//W  ncplane_fadeout_iteration
//W  ncplane_format
//W  ncplane_gradient
//W  ncplane_greyscale
//W  ncplane_highgradient
//W  ncplane_highgradient_sized
//   ncplane_hline_interp
//W# ncplane_home
//W  ncplane_mergedown
//W  ncplane_mergedown_simple
//W  ncplane_move_above
//W  ncplane_move_below
//W  ncplane_move_bottom
//W  ncplane_move_top
//W  ncplane_move_yx
// X ncplane_new                       // deprecated
//W# ncplane_notcurses
//W# ncplane_notcurses_const
//W  ncplane_off_styles
//W  ncplane_on_styles
//W  ncplane_parent
//W  ncplane_parent_const
//   ncplane_polyfill_yx
//W  ncplane_pulse
//   ncplane_putchar_stained
//   ncplane_putc_yx
// X ncplane_putegc_stained            // unneeded
// X ncplane_putegc_yx                 // unneeded
//   ncplane_putnstr_aligned
//   ncplane_putnstr_yx
//   ncplane_putstr_aligned
//   ncplane_putstr_stained
//   ncplane_putstr_yx
//   ncplane_puttext
// X ncplane_putwegc_stained           // unneeded
// X ncplane_putwstr_stained           // unneeded
//   ncplane_qrcode
//W  ncplane_reparent
//W  ncplane_reparent_family
//W# ncplane_resize
//W  ncplane_resizecb
//   ncplane_resize_maximize
//W  ncplane_resize_realign
//   ncplane_rgba
//W  ncplane_rotate_ccw
//W  ncplane_rotate_cw
//W  ncplane_set_base
//W  ncplane_set_base_cell
//W# ncplane_set_bchannel
//W  ncplane_set_bg_alpha
//W  ncplane_set_bg_default
//W  ncplane_set_bg_palindex
//W  ncplane_set_bg_rgb
//W  ncplane_set_bg_rgb8
// X ncplane_set_bg_rgb8_clipped       // unneeded
//W# ncplane_set_channels
//W# ncplane_set_fchannel
//W  ncplane_set_fg_alpha
//W  ncplane_set_fg_default
//W  ncplane_set_fg_palindex
//W  ncplane_set_fg_rgb
//W  ncplane_set_fg_rgb8
// X ncplane_set_fg_rgb8_clipped       // unneeded
//W  ncplane_set_resizecb
//W  ncplane_set_scrolling
//W  ncplane_set_styles
//   ncplane_set_userptr
//W  ncplane_stain
//W  ncplane_styles
// X ncplane_styles_off                // deprecated
// X ncplane_styles_on                 // deprecated
// X ncplane_styles_set                // deprecated
//W  ncplane_translate
//W  ncplane_translate_abs
//   ncplane_userptr
//   ncplane_vline_interp
//   ncplane_vprintf_aligned
//   ncplane_vprintf_stained
//   ncplane_vprintf_yx
//W  ncplane_x
//W  ncplane_y
//W  ncplane_yx
//
// functions manually reimplemented: 39
// ------------------------------------------
// (X) wont:  9
// (+) done: 32 / 0
// (W) wrap: 24
// (#) test:  5
// ------------------------------------------
//W+ ncplane_align
//W+ ncplane_bchannel
//W+ ncplane_bg_alpha
//W# ncplane_bg_default_p
//W+ ncplane_bg_rgb
//W+ ncplane_bg_rgb8
//W+ ncplane_box_sized
//W# ncplane_dim_x
//W# ncplane_dim_y
//W+ ncplane_double_box
//W+ ncplane_double_box_sized
//W+ ncplane_fchannel
//W+ ncplane_fg_alpha
//W# ncplane_fg_default_p
//W+ ncplane_fg_rgb
//W+ ncplane_fg_rgb8
//W+ ncplane_gradient_sized
// + ncplane_hline
//W+ ncplane_perimeter
//W+ ncplane_perimeter_double
//W+ ncplane_perimeter_rounded
// + ncplane_putc
// + ncplane_putchar
// + ncplane_putchar_yx
// X ncplane_putegc                    // unneeded
// + ncplane_putnstr
//W+ ncplane_putstr
// X ncplane_putwc                     // unneeded
// X ncplane_putwc_stained             // unneeded
// X ncplane_putwc_yx                  // unneeded
// X ncplane_putwegc                   // unneeded
// X ncplane_putwegc_yx                // unneeded
// X ncplane_putwstr                   // unneeded
// X ncplane_putwstr_aligned           // unneeded
// X ncplane_putwstr_yx                // unneeded
//W# ncplane_resize_simple
// + ncplane_rounded_box
// + ncplane_rounded_box_sized
// + ncplane_vline
// + ncplane_vprintf
//
// NOTE: TODO: Still remains all the ncplane_printf* functions/macros (at the end)

#[cfg(test)]
mod test;

mod helpers;
mod methods;
mod reimplemented;

#[allow(unused_imports)]
pub(crate) use helpers::*;
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
/// # Piles
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
///
/// # Methods & Associated Functions
///
/// - [Constructors & Destructors](#ncplane-constructors-and-destructors)
///
/// Methods:
/// - [`NcAlphaBits`](#ncplane-methods-ncalphabits)
/// - [`NcChannel` & `NcChannelPair`](#ncplane-methods-ncchannel)
/// - [`NcColor`, `NcRgb` & default color](#ncplane-methods-nccolor-ncrgb--default-color)
/// - [`NcStyleMask` & `NcPaletteIndex`](#ncplane-methods-ncstylemask--paletteindex)
/// - [`NcCell` & `NcEgc`](#ncplane-methods-nccell--ncegc)
/// - [cursor](#ncplane-methods-cursor)
/// - [`NcPlane` & `Notcurses`](#ncplane-methods-ncplane--notcurses)
/// - [boxes & perimeters](#ncplane-methods-boxes--perimeters)
/// - [Size, position & alignment](#ncplane-methods-size-position--alignment)
/// - [fading, gradients & greyscale](#ncplane-methods-fading-gradients--greyscale)
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
///
/// See [Sixel in Wikipedia](https://en.wikipedia.org/wiki/Sixel).
pub const NCBLIT_SIXEL: NcBlitter = crate::bindings::ffi::ncblitter_e_NCBLIT_SIXEL;
