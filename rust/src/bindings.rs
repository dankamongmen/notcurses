//! Curated re-exports of the `bindgen` bindings
//!
//! The full list of bindings is under the
//! [`bindgen`] submodule
//!
//! This module publicly re-exports bindgen generated functions and constants,
//! while privately re-exporting other functions, constants and all structs
//! in order to wrap them up or crate new aliases for them.
//
// WARNING: DO NOT EXECUTE RUSTFMT ON THIS FILE.
// Custom formatting permits easier maintenance.
//

// [clippy & bindgen](https://github.com/rust-lang/rust-bindgen/issues/1470)
#[allow(clippy::all)]
pub mod bindgen {
    //! Automatically generated Rust FFI bindings
    //!
    //! All of the notcurses functions and some of the constants are reexported
    //! by the [`bindings`] module.
    //! While the structs, enums and some other constants are type aliased in
    //! the [`types`] module, in order to follow the
    //! Rust API Guidelines as much as possible.
    //!
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}

// Miscellaneous ---------------------------------------------------------------

pub(crate) use bindgen::{
    // structs
    timespec,
};

#[doc(inline)]
pub use bindgen::{
    // structs
    __va_list_tag,

    // functions
    ncstrwidth,
};

// blitset ---------------------------------------------------------------------

pub(crate) use bindgen::{
    // structs
    blitset
};

// cell ------------------------------------------------------------------------

pub(crate) use bindgen::{
    // structs
    cell,

    // constants
    CELL_ALPHA_BLEND,
    CELL_ALPHA_HIGHCONTRAST,
    CELL_ALPHA_OPAQUE,
    CELL_ALPHA_TRANSPARENT,
    CELL_BGDEFAULT_MASK,
    CELL_BG_ALPHA_MASK,
    CELL_BG_PALETTE,
    CELL_BG_RGB_MASK,
    CELL_FGDEFAULT_MASK,
    CELL_FG_ALPHA_MASK,
    CELL_FG_PALETTE,
    CELL_FG_RGB_MASK,
    CELL_NOBACKGROUND_MASK,
    CELL_WIDEASIAN_MASK,
};

#[doc(inline)]
pub use bindgen::{
    // functions
    cell_duplicate,
    cell_extended_gcluster,
    cell_load,
    cell_release,
    cells_double_box,
    cells_rounded_box,
};

// channel ---------------------------------------------------------------------

pub(crate) use bindgen::{
    // constants
    CHANNEL_ALPHA_MASK
};

// ncalign ---------------------------------------------------------------------

pub(crate) use bindgen::{
    // type definitions
    ncalign_e,

    // constants
    ncalign_e_NCALIGN_CENTER,
    ncalign_e_NCALIGN_LEFT,
    ncalign_e_NCALIGN_RIGHT,
    ncalign_e_NCALIGN_UNALIGNED,
};

// ncblitter -------------------------------------------------------------------

pub(crate) use bindgen::{
    // type definitions
    ncblitter_e,

    // constants
    ncblitter_e_NCBLIT_1x1,
    ncblitter_e_NCBLIT_2x1,
    ncblitter_e_NCBLIT_2x2,
    ncblitter_e_NCBLIT_3x2,
    ncblitter_e_NCBLIT_4x1,
    ncblitter_e_NCBLIT_8x1,
    ncblitter_e_NCBLIT_BRAILLE,
    ncblitter_e_NCBLIT_DEFAULT,
    ncblitter_e_NCBLIT_SIXEL,
};

// ncbox -----------------------------------------------------------------------

#[doc(inline)]
pub use bindgen::{
    // constants
    NCBOXCORNER_MASK,
    NCBOXCORNER_SHIFT,
    NCBOXGRAD_BOTTOM,
    NCBOXGRAD_LEFT,
    NCBOXGRAD_RIGHT,
    NCBOXGRAD_TOP,
    NCBOXMASK_BOTTOM,
    NCBOXMASK_LEFT,
    NCBOXMASK_RIGHT,
    NCBOXMASK_TOP,
};

// ncdirect --------------------------------------------------------------------

pub(crate) use bindgen::{
    // structs
    ncdirect,

    // constants
    NCDIRECT_OPTION_INHIBIT_CBREAK,
    NCDIRECT_OPTION_INHIBIT_SETLOCALE,
};

#[doc(inline)]
pub use bindgen::{
    // functions
    ncdirect_bg_default,
    ncdirect_bg_palindex,
    ncdirect_bg_rgb,
    ncdirect_box,
    ncdirect_canopen_images,
    ncdirect_canutf8,
    ncdirect_clear,
    ncdirect_cursor_disable,
    ncdirect_cursor_down,
    ncdirect_cursor_enable,
    ncdirect_cursor_left,
    ncdirect_cursor_move_yx,
    ncdirect_cursor_pop,
    ncdirect_cursor_push,
    ncdirect_cursor_right,
    ncdirect_cursor_up,
    ncdirect_cursor_yx,
    ncdirect_dim_x,
    ncdirect_dim_y,
    ncdirect_double_box,
    ncdirect_fg_default,
    ncdirect_fg_palindex,
    ncdirect_fg_rgb,
    ncdirect_flush,
    ncdirect_getc,
    ncdirect_hline_interp,
    ncdirect_init,
    ncdirect_inputready_fd,
    ncdirect_palette_size,
    ncdirect_printf_aligned,
    ncdirect_putstr,
    ncdirect_render_image,
    ncdirect_rounded_box,
    ncdirect_stop,
    ncdirect_styles_off,
    ncdirect_styles_on,
    ncdirect_styles_set,
    ncdirect_vline_interp,
};


// ncfadectx -------------------------------------------------------------------

pub(crate) use bindgen::{
    // structs
    ncfadectx,
};

#[doc(inline)]
pub use bindgen::{
    // functions
    ncfadectx_free,
    ncfadectx_iterations,
    ncfadectx_setup,
};

// ncinput ---------------------------------------------------------------------

pub(crate) use bindgen::{
    // structs
    ncinput,
};

// ncloglevel ------------------------------------------------------------------

pub(crate) use bindgen::{
    // type definitions
    ncloglevel_e,

    // constants
    ncloglevel_e_NCLOGLEVEL_DEBUG,
    ncloglevel_e_NCLOGLEVEL_ERROR,
    ncloglevel_e_NCLOGLEVEL_FATAL,
    ncloglevel_e_NCLOGLEVEL_INFO,
    ncloglevel_e_NCLOGLEVEL_PANIC,
    ncloglevel_e_NCLOGLEVEL_SILENT,
    ncloglevel_e_NCLOGLEVEL_TRACE,
    ncloglevel_e_NCLOGLEVEL_VERBOSE,
    ncloglevel_e_NCLOGLEVEL_WARNING,
};

// ncfdplane -------------------------------------------------------------------

pub(crate) use bindgen::{
    // structs
    ncfdplane,
    ncfdplane_options,
};

#[doc(inline)]
pub use bindgen::{
    // functions
    ncfdplane_create,
    ncfdplane_destroy,
    ncfdplane_plane,
};

// ncmenu ----------------------------------------------------------------------

pub(crate) use bindgen::{
    // structs
    ncmenu,
    ncmenu_item,
    ncmenu_options,
    ncmenu_section,

    // constants
    NCMENU_OPTION_BOTTOM,
    NCMENU_OPTION_HIDING,
};

#[doc(inline)]
pub use bindgen::{
    // functions
    ncmenu_create,
    ncmenu_destroy,
    ncmenu_item_set_status,
    ncmenu_mouse_selected,
    ncmenu_nextitem,
    ncmenu_nextsection,
    ncmenu_offer_input,
    ncmenu_plane,
    ncmenu_previtem,
    ncmenu_prevsection,
    ncmenu_rollup,
    ncmenu_selected,
    ncmenu_unroll,
};

// ncmetric --------------------------------------------------------------------

pub(crate) use bindgen::{
    // constants
    PREFIXCOLUMNS,
    PREFIXSTRLEN,
    BPREFIXCOLUMNS,
    BPREFIXSTRLEN,
    IPREFIXCOLUMNS,
    IPREFIXSTRLEN,
};

#[doc(inline)]
pub use bindgen::{
    // structs
    ncmetric
};

// ncmultiselector -------------------------------------------------------------

pub(crate) use bindgen::{
    // structs
    ncmultiselector,
    ncmselector_item,
    ncmultiselector_options,
};

#[doc(inline)]
pub use bindgen::{
    // functions
    ncmultiselector_create,
    ncmultiselector_destroy,
    ncmultiselector_offer_input,
    ncmultiselector_plane,
    ncmultiselector_selected,
};

// ncplane ---------------------------------------------------------------------

pub(crate) use bindgen::{
    // structs
    ncplane,
    ncplane_options,

    // constants
    NCPLANE_OPTION_HORALIGNED,
};

#[doc(inline)]
pub use bindgen::{
    // functions
    ncpile_create,
    ncplane_above,
    ncplane_at_cursor,
    ncplane_at_yx,
    ncplane_base,
    ncplane_below,
    ncplane_box,
    ncplane_center_abs,
    ncplane_channels,
    ncplane_contents,
    ncplane_create,
    ncplane_cursor_move_yx,
    ncplane_cursor_yx,
    ncplane_destroy,
    ncplane_dim_yx,
    ncplane_dup,
    ncplane_erase,
    ncplane_fadein,
    ncplane_fadein_iteration,
    ncplane_fadeout,
    ncplane_fadeout_iteration,
    ncplane_format,
    ncplane_gradient,
    ncplane_greyscale,
    ncplane_highgradient,
    ncplane_highgradient_sized,
    ncplane_hline_interp,
    ncplane_home,
    ncplane_mergedown,
    ncplane_mergedown_simple,
    ncplane_move_above,
    ncplane_move_below,
    ncplane_move_bottom,
    ncplane_move_top,
    ncplane_move_yx,
    ncplane_notcurses,
    ncplane_notcurses_const,
    ncplane_off_styles,
    ncplane_on_styles,
    ncplane_parent,
    ncplane_parent_const,
    ncplane_polyfill_yx,
    ncplane_pulse,
    ncplane_putc_yx,
    ncplane_putchar_stained,
    ncplane_putegc_stained,
    ncplane_putegc_yx,
    ncplane_putnstr_aligned,
    ncplane_putnstr_yx,
    ncplane_putstr_aligned,
    ncplane_putstr_stained,
    ncplane_putstr_yx,
    ncplane_puttext,
    ncplane_putwegc_stained,
    ncplane_putwstr_stained,
    ncplane_qrcode,
    ncplane_reparent,
    ncplane_reparent_family,
    ncplane_resize,
    ncplane_resize_realign,
    ncplane_resizecb,
    ncplane_rgba,
    ncplane_rotate_ccw,
    ncplane_rotate_cw,
    ncplane_set_base,
    ncplane_set_base_cell,
    ncplane_set_bchannel,
    ncplane_set_bg_alpha,
    ncplane_set_bg_default,
    ncplane_set_bg_palindex,
    ncplane_set_bg_rgb,
    ncplane_set_bg_rgb8,
    ncplane_set_bg_rgb8_clipped,
    ncplane_set_channels,
    ncplane_set_fchannel,
    ncplane_set_fg_alpha,
    ncplane_set_fg_default,
    ncplane_set_fg_palindex,
    ncplane_set_fg_rgb,
    ncplane_set_fg_rgb8,
    ncplane_set_fg_rgb8_clipped,
    ncplane_set_scrolling,
    ncplane_set_styles,
    ncplane_set_userptr,
    ncplane_stain,
    ncplane_styles,
    ncplane_styles_off,
    ncplane_styles_on,
    ncplane_styles_set,
    ncplane_translate,
    ncplane_translate_abs,
    ncplane_userptr,
    ncplane_vline_interp,
    ncplane_vprintf_aligned,
    ncplane_vprintf_stained,
    ncplane_vprintf_yx,
    ncplane_x,
    ncplane_y,
    ncplane_yx,
};

// ncplot ----------------------------------------------------------------------

pub(crate) use bindgen::{
    // structs
    ncdplot, // f64
    ncuplot, // u64
    ncplot_options,

    // constants
    NCPLOT_OPTION_DETECTMAXONLY,
    NCPLOT_OPTION_EXPONENTIALD,
    NCPLOT_OPTION_LABELTICKSD,
    NCPLOT_OPTION_NODEGRADE,
    NCPLOT_OPTION_VERTICALI,
};

#[doc(inline)]
pub use bindgen::{
    // functions
    ncdplot_add_sample,
    ncdplot_create,
    ncdplot_destroy,
    ncdplot_plane,
    ncdplot_sample,
    ncdplot_set_sample,

    ncuplot_add_sample,
    ncuplot_create,
    ncuplot_destroy,
    ncuplot_plane,
    ncuplot_sample,
    ncuplot_set_sample,
};

// ncreader --------------------------------------------------------------------

pub(crate) use bindgen::{
    // structs
    ncreader,
    ncreader_options,

    // constants
    NCREADER_OPTION_CURSOR,
    NCREADER_OPTION_HORSCROLL,
    NCREADER_OPTION_NOCMDKEYS,
    NCREADER_OPTION_VERSCROLL,
};

#[doc(inline)]
pub use bindgen::{
    // functions
    ncreader_clear,
    ncreader_contents,
    ncreader_create,
    ncreader_destroy,
    ncreader_move_down,
    ncreader_move_left,
    ncreader_move_right,
    ncreader_move_up,
    ncreader_offer_input,
    ncreader_plane,
    ncreader_write_egc,
};

// ncreel ----------------------------------------------------------------------

pub(crate) use bindgen::{
    // structs
    ncreel,
    ncreel_options,

    // constants
    NCREEL_OPTION_CIRCULAR,
    NCREEL_OPTION_INFINITESCROLL,
};

#[doc(inline)]
pub use bindgen::{
    // functions
    ncreel_add,
    ncreel_create,
    ncreel_del,
    ncreel_destroy,
    ncreel_focused,
    ncreel_next,
    ncreel_offer_input,
    ncreel_plane,
    ncreel_prev,
    ncreel_redraw,
    ncreel_tabletcount,
};

// ncscale ---------------------------------------------------------------------

pub(crate) use bindgen::{
    // type definitions
    ncscale_e,

    // constants
    ncscale_e_NCSCALE_NONE,
    ncscale_e_NCSCALE_SCALE,
    ncscale_e_NCSCALE_STRETCH,
};

// ncselector ------------------------------------------------------------------

pub(crate) use bindgen::{
    // structs
    ncselector,
    ncselector_item,
    ncselector_options,
};

#[doc(inline)]
pub use bindgen::{
    // functions
    ncselector_additem,
    ncselector_create,
    ncselector_delitem,
    ncselector_destroy,
    ncselector_nextitem,
    ncselector_offer_input,
    ncselector_plane,
    ncselector_previtem,
    ncselector_selected,
};

// ncstats ---------------------------------------------------------------------

pub(crate) use bindgen::{
    // structs
    ncstats,
};

// ncstyle ---------------------------------------------------------------------

#[doc(inline)]
pub use bindgen::{
    // constants
    NCSTYLE_BLINK,
    NCSTYLE_BOLD,
    NCSTYLE_DIM,
    NCSTYLE_INVIS,
    NCSTYLE_ITALIC,
    NCSTYLE_MASK,
    NCSTYLE_NONE,
    NCSTYLE_PROTECT,
    NCSTYLE_REVERSE,
    NCSTYLE_STANDOUT,
    NCSTYLE_UNDERLINE,
};

// nctablet --------------------------------------------------------------------

pub(crate) use bindgen::{
    // structs
    nctablet,
};

#[doc(inline)]
pub use bindgen::{
    // functions
    nctablet_plane,
    nctablet_userptr,
};

// ncvisual --------------------------------------------------------------------

pub(crate) use bindgen::{
    // structs
    ncvisual,
    ncvisual_options,

    // constants
    NCVISUAL_OPTION_BLEND,
    NCVISUAL_OPTION_NODEGRADE,
};

#[doc(inline)]
pub use bindgen::{
    // functions
    ncvisual_at_yx,
    ncvisual_decode,
    ncvisual_decode_loop,
    ncvisual_destroy,
    ncvisual_from_bgra,
    ncvisual_from_file,
    ncvisual_from_plane,
    ncvisual_from_rgba,
    ncvisual_geom,
    ncvisual_polyfill_yx,
    ncvisual_render,
    ncvisual_resize,
    ncvisual_rotate,
    ncvisual_set_yx,
    ncvisual_simple_streamer,
    ncvisual_stream,
    ncvisual_subtitle,
};

// notcurses -------------------------------------------------------------------

pub(crate) use bindgen::{
    // structs
    notcurses,
    notcurses_options,

    // constants
    NCOPTION_INHIBIT_SETLOCALE,
    NCOPTION_NO_ALTERNATE_SCREEN,
    NCOPTION_NO_FONT_CHANGES,
    NCOPTION_NO_QUIT_SIGHANDLERS,
    NCOPTION_NO_WINCH_SIGHANDLER,
    NCOPTION_SUPPRESS_BANNERS,
    NCOPTION_VERIFY_SIXEL,
};

#[doc(inline)]
pub use bindgen::{
    // functions
    notcurses_at_yx,
    notcurses_bottom,
    notcurses_canchangecolor,
    notcurses_canfade,
    notcurses_canopen_images,
    notcurses_canopen_videos,
    notcurses_cansixel,
    notcurses_cantruecolor,
    notcurses_canutf8,
    notcurses_cursor_disable,
    notcurses_cursor_enable,
    notcurses_debug,
    notcurses_drop_planes,
    notcurses_getc,
    notcurses_init,
    notcurses_inputready_fd,
    notcurses_lex_blitter,
    notcurses_lex_margins,
    notcurses_lex_scalemode,
    notcurses_mouse_disable,
    notcurses_mouse_enable,
    notcurses_palette_size,
    notcurses_refresh,
    notcurses_render,
    notcurses_render_to_buffer,
    notcurses_render_to_file,
    notcurses_stats,
    notcurses_stats_alloc,
    notcurses_stats_reset,
    notcurses_stdplane,
    notcurses_stdplane_const,
    notcurses_stop,
    notcurses_str_blitter,
    notcurses_str_scalemode,
    notcurses_supported_styles,
    notcurses_top,
    notcurses_ucs32_to_utf8,
    notcurses_version,
    notcurses_version_components,
};

// palette ---------------------------------------------------------------------

pub(crate) use bindgen::{
    // structs
    palette256,
};

#[doc(inline)]
pub use bindgen::{
    // functions
    palette256_free,
    palette256_new,
    palette256_use,

    // constants
    NCPALETTESIZE,
};

// sig -------------------------------------------------------------------------

#[doc(inline)]
pub use bindgen::{
    // type definitions
    sigset_t,

    // structs
    //sigaction,

    // functions
    sigaddset,
    sigdelset,
    sigemptyset,
    sigfillset,
    sigismember,
    sigpending,
    sigprocmask,
    sigsuspend,
};
