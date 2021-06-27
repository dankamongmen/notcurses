//! `NcPlane`

// functions already exported by bindgen : 117
// -------------------------------------------
// (X) wont: 10
// (D) depr:  4
// (#) test: 13
// (W) wrap: 83
// -------------------------------------------
//W  ncpile_bottom
//W# ncpile_create
//W  ncpile_rasterize
//W  ncpile_render
//W  ncpile_top
//W  ncplane_above
//W  ncplane_abs_x
//W  ncplane_abs_y
//W  ncplane_abs_yx
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
//   ncplane_erase_region
//W  ncplane_fadein
//W  ncplane_fadein_iteration
//W  ncplane_fadeout
//W  ncplane_fadeout_iteration
//W  ncplane_format
//W  ncplane_gradient
//W  ncplane_greyscale
//   ncplane_halign
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
//W  ncplane_pixelgeom
//   ncplane_polyfill_yx
//W  ncplane_pulse
//   ncplane_putchar_stained
//   ncplane_putc_yx
// X ncplane_putegc_stained            // unneeded
// X ncplane_putegc_yx                 // unneeded
//   ncplane_putnstr_aligned
//   ncplane_putnstr_yx
//W  ncplane_putstr_aligned
//W  ncplane_putstr_stained
//W  ncplane_putstr_yx
//   ncplane_puttext
// X ncplane_putwegc_stained           // unneeded
// X ncplane_putwstr_stained           // unneeded
//   ncplane_qrcode
//W  ncplane_reparent
//W  ncplane_reparent_family
//W# ncplane_resize
//W  ncplane_resizecb
//W  ncplane_resize_marginalize
//W  ncplane_resize_maximize
//W  ncplane_resize_realign
//W  ncplane_rgba
//W  ncplane_rotate_ccw
//W  ncplane_rotate_cw
//W  ncplane_scrolling_p
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
//   ncplane_valign
//   ncplane_vline_interp
// X ncplane_vprintf_aligned
// X ncplane_vprintf_stained
// X ncplane_vprintf_yx
//W  ncplane_x
//W  ncplane_y
//W  ncplane_yx
//
// functions manually reimplemented: 40
// ------------------------------------------
// (X) wont:  9
// (+) done: 33 / 0
// (W) wrap: 25
// (#) test:  5
// ------------------------------------------
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
//W+ ncplane_halign
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
//W+ ncplane_halign
// + ncplane_vline
// X ncplane_vprintf

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
/// # About planes and piles
///
/// A given notcurses context is made up of one or more piles.
///
/// A pile is made up of [`NcPlane`]s, totally ordered on a z-axis.
///
/// You can't manage the piles directly, but only the `NcPlanes`.
///
/// A pile is destroyed when all its planes are destroyed or moved to other
/// piles.
///
/// A pile has a top and bottom plane (this might be a single `NcPlane`),
/// and one or more root planes (`NcPlane`s which are bound to themselves).
///
/// Multiple threads can concurrently operate on distinct piles, rendering or
/// mutating it, while another thread concurrently renders or mutates another.
///
/// Each `NcPlane` is part of one and only one pile. By default, an `NcPlane` is
/// part of the same pile that contains the `NcPlane` to which it is bound.
///
/// When an `NcPlane` is created without being bound to another `NcPlane`, then
/// it becomes the root plane, top, and bottom of a new pile. As a root plane,
/// it is bound to itself.
///
/// A new pile can also be created by reparenting an `NcPlane` to itself,
/// though if the plane is already a root plane, this is a no-op.
///
/// When an `NcPlane` is moved to a different pile (whether new or preexisting),
/// any `NcPlane`s which were bound to it are rebound to its previous parent.
/// If the `NcPlane` was a root plane of some pile, any bound planes become root
/// planes. The new `NcPlane` is placed immediately atop its new parent on its
/// new pile's z-axis.
///
/// When [`NcPlane::reparent_family`][NcPlane#method.reparent_family] is used,
/// all `NcPlanes` bound to the reparented `NcPlane` are moved along with it.
/// Their relative z-order is maintained.
//
/// Rendering reduces a pile of `NcPlane`s to a single `NcPlane`, proceeding
/// from the top to the bottom along a pile's z-axis. The result is a matrix of
/// [`NcCell`][crate::NcCell]s. Rasterizing takes this matrix, together with the
/// current state of the visual area, and produces a stream of optimized control
/// sequences and `EGC`s for the terminal. By writing this stream to the
/// terminal, the physical display is synced to some pile's `NcPlane`s.
///
/// [`NcPlane.render`][crate::NcPlane#method.render] performs the first of these
/// tasks for the pile of which the plane is a part. The output is maintained
/// internally; calling `render` again on the same pile will replace this state
/// with a fresh render. Multiple piles can be concurrently rendered.
/// [`NcPlane.rasterize`][crate::NcPlane#method.rasterize] performs
/// rasterization, and writes the result to the terminal. It is a blocking call,
/// and only one rasterization operation may proceed at a time.
///
/// It is necessary to call `NcPlane.rasterize` to generate any visible output;
/// the various *output calls* only draw to the virtual `NcPlane`s. Most of the
/// notcurses `statistics` are updated as a result of a render, and screen
/// geometry is refreshed (similarly to
/// [`Notcurses.refresh`][crate::Notcurses#method.refresh]) following the render.
///
/// # Methods & Associated Functions
///
/// - [Constructors & Destructors](#ncplane-constructors--destructors)
///
/// Methods:
/// - [`NcAlphaBits`](#ncplane-methods-ncalphabits)
/// - [`NcChannel` & `NcChannels`](#ncplane-methods-ncchannel)
/// - [`NcComponent`, `NcRgb` & default color](#ncplane-methods-nccomponent-ncrgb--default-color)
/// - [`NcStyle` & `NcPaletteIndex`](#ncplane-methods-ncstylemask--paletteindex)
/// - [`NcCell` & strings](#ncplane-methods-nccell--strings)
/// - [cursor](#ncplane-methods-cursor)
/// - [`NcPlane` & `Notcurses`](#ncplane-methods-ncplane--notcurses)
/// - [boxes & perimeters](#ncplane-methods-boxes--perimeters)
/// - [Size, position & alignment](#ncplane-methods-size-position--alignment)
/// - [fading, gradients & greyscale](#ncplane-methods-fading-gradients--greyscale)
///
pub type NcPlane = crate::bindings::ffi::ncplane;

/// Options struct for [`NcPlane`]
pub type NcPlaneOptions = crate::bindings::ffi::ncplane_options;

/// Horizontal alignment relative to the parent plane. Use NcAlign for 'x'.
pub const NCPLANE_OPTION_HORALIGNED: u64 = crate::bindings::ffi::NCPLANE_OPTION_HORALIGNED as u64;

/// Vertical alignment relative to the parent plane. Use NcAlign for 'y'.
pub const NCPLANE_OPTION_VERALIGNED: u64 = crate::bindings::ffi::NCPLANE_OPTION_VERALIGNED as u64;

/// Maximize relative to the parent plane, modulo the provided margins.
///
/// The margins are best-effort; the plane will always be at least 1 column by
/// 1 row. If the margins can be effected, the plane will be sized to all
/// remaining space. 'y' and 'x' are overloaded as the top and left margins
/// when this flag is used. 'rows' and 'cols' must be 0 when this flag is
/// used. This flag is exclusive with both of the alignment flags.
pub const NCPLANE_OPTION_MARGINALIZED: u64 =
    crate::bindings::ffi::NCPLANE_OPTION_MARGINALIZED as u64;

/// I/O wrapper to dump file descriptor to [`NcPlane`]
///
/// `type in C: ncfdplane (struct)`
pub type NcFdPlane = crate::bindings::ffi::ncfdplane;

/// Options struct for [`NcFdPlane`]
///
/// `type in C: ncplane_options (struct)`
pub type NcFdPlaneOptions = crate::bindings::ffi::ncfdplane_options;
