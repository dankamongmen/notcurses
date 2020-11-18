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
    Cell, Egc, EgcBackstop, StyleMask, CELL_ALPHA_BLEND, CELL_ALPHA_HIGHCONTRAST,
    CELL_ALPHA_OPAQUE, CELL_ALPHA_TRANSPARENT, CELL_BGDEFAULT_MASK, CELL_BG_ALPHA_MASK,
    CELL_BG_PALETTE, CELL_BG_RGB_MASK, CELL_FGDEFAULT_MASK, CELL_FG_ALPHA_MASK, CELL_FG_PALETTE,
    CELL_FG_RGB_MASK, CELL_NOBACKGROUND_MASK, CELL_WIDEASIAN_MASK,
};
pub use channel::{
    AlphaBits, BlitSet, Channel, Channels, Color, NcFadeCtx, NcPixel, Palette, PaletteIndex, Rgb,
    CHANNEL_ALPHA_MASK,
};
pub use file::{NcFile, LIBC_FILE, NC_FILE};
pub use misc::{
    IntResult, BPREFIXCOLUMNS, BPREFIXSTRLEN, IPREFIXCOLUMNS, IPREFIXSTRLEN, PREFIXCOLUMNS,
    PREFIXSTRLEN,
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
