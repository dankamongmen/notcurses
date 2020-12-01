//! Idiomatic Rust Type Aliases
//!
//! These types wrap the ones used in the C library, including structs,
//! constants, and primitives when used as parameters or return values.
//!
//! This is in order to:
//!
//! - Enforce type checking.
//! - Follow the
//! [Rust API Guidelines](https://rust-lang.github.io/api-guidelines/naming.html).
//! - Add or improve the doc-comments to the type.
//! - Add constructors to the type aliased structures.
//!
#![allow(dead_code)]

mod cell;
mod channel;
mod file;
mod misc;
mod plane;
mod terminal;
mod widgets;

pub use cell::{
    NcCell, NcChar, NcCharBackstop, NcStyleMask, NCCELL_ALPHA_BLEND, NCCELL_ALPHA_HIGHCONTRAST,
    NCCELL_ALPHA_OPAQUE, NCCELL_ALPHA_TRANSPARENT, NCCELL_BGDEFAULT_MASK, NCCELL_BG_ALPHA_MASK,
    NCCELL_BG_PALETTE, NCCELL_BG_RGB_MASK, NCCELL_FGDEFAULT_MASK, NCCELL_FG_ALPHA_MASK,
    NCCELL_FG_PALETTE, NCCELL_FG_RGB_MASK, NCCELL_NOBACKGROUND_MASK, NCCELL_WIDEASIAN_MASK,
    NCSTYLE_BLINK, NCSTYLE_BOLD, NCSTYLE_DIM, NCSTYLE_INVIS, NCSTYLE_ITALIC, NCSTYLE_MASK,
    NCSTYLE_NONE, NCSTYLE_PROTECT, NCSTYLE_REVERSE, NCSTYLE_STANDOUT, NCSTYLE_STRUCK,
    NCSTYLE_UNDERLINE,
};
pub use channel::{
    NcAlphaBits, NcBlitSet, NcChannel, NcChannels, NcColor, NcFadeCtx, NcPalette, NcPaletteIndex,
    NcPixel, NcRgb, NCCHANNEL_ALPHA_MASK,
};
pub use file::{NcFile, FILE_LIBC, FILE_NC};
pub use misc::{
    NcResult, NCMETRIC_BPREFIXCOLUMNS, NCMETRIC_BPREFIXSTRLEN, NCMETRIC_IPREFIXCOLUMNS,
    NCMETRIC_IPREFIXSTRLEN, NCMETRIC_PREFIXCOLUMNS, NCMETRIC_PREFIXSTRLEN, NCRESULT_ERR,
    NCRESULT_OK,
};
pub use plane::{
    NCBLIT_1x1, NCBLIT_2x1, NCBLIT_2x2, NCBLIT_3x2, NCBLIT_4x1, NCBLIT_8x1, NcAlign, NcBlitter,
    NcFdPlane, NcFdPlaneOptions, NcPlane, NcPlaneOptions, NcScale, NcVisual, NcVisualOptions,
    NCALIGN_CENTER, NCALIGN_LEFT, NCALIGN_RIGHT, NCALIGN_UNALIGNED, NCBLIT_BRAILLE, NCBLIT_DEFAULT,
    NCBLIT_SIXEL, NCPLANE_OPTION_HORALIGNED, NCSCALE_NONE, NCSCALE_SCALE, NCSCALE_STRETCH,
    NCVISUAL_OPTION_BLEND, NCVISUAL_OPTION_NODEGRADE,
};
pub use terminal::{
    NcDirect, NcDirectFlags, NcInput, NcLogLevel, Notcurses, NotcursesOptions,
    NCDIRECT_OPTION_INHIBIT_CBREAK, NCDIRECT_OPTION_INHIBIT_SETLOCALE, NCLOGLEVEL_DEBUG,
    NCLOGLEVEL_ERROR, NCLOGLEVEL_FATAL, NCLOGLEVEL_INFO, NCLOGLEVEL_PANIC, NCLOGLEVEL_SILENT,
    NCLOGLEVEL_TRACE, NCLOGLEVEL_VERBOSE, NCLOGLEVEL_WARNING, NCOPTION_INHIBIT_SETLOCALE,
    NCOPTION_NO_ALTERNATE_SCREEN, NCOPTION_NO_FONT_CHANGES, NCOPTION_NO_QUIT_SIGHANDLERS,
    NCOPTION_NO_WINCH_SIGHANDLER, NCOPTION_SUPPRESS_BANNERS, NCOPTION_VERIFY_SIXEL,
};
pub use widgets::{
    NcMenu, NcMenuItem, NcMenuOptions, NcMenuSection, NcMultiSelector, NcMultiSelectorItem,
    NcMultiSelectorOptions, NcPlotF64, NcPlotOptions, NcPlotU64, NcReader, NcReaderOptions, NcReel,
    NcReelOptions, NcSelector, NcSelectorItem, NcSelectorOptions, NcStats, NcTablet,
    NCMENU_OPTION_BOTTOM, NCMENU_OPTION_HIDING, NCPLOT_OPTION_DETECTMAXONLY,
    NCPLOT_OPTION_EXPONENTIALD, NCPLOT_OPTION_LABELTICKSD, NCPLOT_OPTION_NODEGRADE,
    NCPLOT_OPTION_VERTICALI, NCREADER_OPTION_CURSOR, NCREADER_OPTION_HORSCROLL,
    NCREADER_OPTION_NOCMDKEYS, NCREADER_OPTION_VERSCROLL, NCREEL_OPTION_CIRCULAR,
    NCREEL_OPTION_INFINITESCROLL,
};
