// Rgb
//
/// 24 bits broken into 3x 8bpp channels.
///
/// ```txt
/// -------- RRRRRRRR GGGGGGGG BBBBBBBB
/// ```
///
/// `type in C: no data type`
///
pub type Rgb = u32;

// Color
//
/// 8 bits representing a R/G/B color or alpha channel
///
/// ```txt
/// CCCCCCCC (1 Byte)
/// ```
///
/// `type in C: no data type`
///
pub type Color = u8;

// NcPixel (RGBA)
/// 32 bits broken into RGB + 8-bit alpha
///
/// ```txt
/// AAAAAAAA GGGGGGGG BBBBBBBB RRRRRRRR
/// ```
///
/// NcPixel has 8 bits of alpha,  more or less linear, contributing
/// directly to the usual alpha blending equation.
///
/// We map the 8 bits of alpha to 2 bits of alpha via a level function:
/// https://nick-black.com/dankwiki/index.php?title=Notcurses#Transparency.2FContrasting
///
/// `type in C: ncpixel (uint32_t)`
///
// NOTE: the order of the colors is different than in Channel.
pub type NcPixel = u32;

/// Palette structure consisting of an array of 256 [`Channel`](type.Channel.html)s
///
/// Some terminals only support 256 colors, but allow the full
/// palette to be specified with arbitrary RGB colors. In all cases, it's more
/// performant to use indexed colors, since it's much less data to write to the
/// terminal. If you can limit yourself to 256 colors, that's probably best.
///
/// `type in C: ncpalette256 (struct)`
///
pub type Palette = crate::palette256;

/// 8-bit value used for indexing into a [`Palette`](type.Palette.html)
///
pub type PaletteIndex = u8;

/// Context for a palette fade operation
pub type NcFadeCtx = crate::ncfadectx;

/// the [`Egc`](type.Egc.html)s which form the various levels
/// of a given geometry.
///
/// If the geometry is wide, things are arranged with the rightmost side
/// increasing most quickly, i.e. it can be indexed as height arrays of
/// 1 + height glyphs.
/// i.e. The first five braille EGCs are all 0 on the left,
/// [0..4] on the right.
///
/// `type in C: blitset (struct)`
///
pub type BlitSet = crate::blitset;
