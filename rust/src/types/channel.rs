#[allow(unused_imports)] // for docblocks
use crate::NcChar;

// NcChannel
//
/// 32 bits of context-dependent info
/// containing RGB + 2 bits of alpha + extra
///
/// ```txt
/// ~~AA~~~~ RRRRRRRR GGGGGGGG BBBBBBBB
/// ```
///
/// It is:
/// - an RGB value
/// - plus 2 bits of alpha
/// - plus context-dependent info
///
/// The context details are documented in [`NcChannels`]
///
/// `type in C: channel (uint32_t)`
///
pub type NcChannel = u32;

/// Extract these bits to get a channel's alpha value
pub const NCCHANNEL_ALPHA_MASK: u32 = crate::bindings::CHANNEL_ALPHA_MASK;

// NcAlphaBits
//
/// 2 bits of alpha (surrounded by context dependent bits).
/// It is part of an [`NcChannel`].
///
/// ```txt
/// ~~AA~~~~ -------- -------- --------
/// ```
///
/// `type in C: no data type`
///
pub type NcAlphaBits = u32;

// NcChannels
//
/// 64 bits containing a foreground and background [`NcChannel`]
///
/// ```txt
/// ~~AA~~~~|RRRRRRRR|GGGGGGGG|BBBBBBBB|~~AA~~~~|RRRRRRRR|GGGGGGGG|BBBBBBBB
/// ↑↑↑↑↑↑↑↑↑↑↑↑ foreground ↑↑↑↑↑↑↑↑↑↑↑|↑↑↑↑↑↑↑↑↑↑↑↑ background ↑↑↑↑↑↑↑↑↑↑↑
/// ```
///
/// Detailed info (specially on the context-dependent bits on each
/// `NcChannel`'s 4th byte):
///
/// ```txt
///                             ~foreground channel~
/// part of a wide glyph:                                ↓bits view↓               ↓hex mask↓
/// 1······· ········ ········ ········ ········ ········ ········ ········  =  8······· ········
///
/// foreground is *not* "default color":
/// ·1······ ········ ········ ········ ········ ········ ········ ········  =  4······· ········
///
/// foreground alpha (2bits):
/// ··11···· ········ ········ ········ ········ ········ ········ ········  =  3······· ········
///
/// foreground uses palette index:
/// ····1··· ········ ········ ········ ········ ········ ········ ········  =  ·8······ ········
///
/// glyph is entirely foreground:
/// ·····1·· ········ ········ ········ ········ ········ ········ ········  =  ·4······ ········
///
/// reserved, must be 0:
/// ······00 ········ ········ ········ ········ ········ ········ ········  =  ·3······ ········
///
/// foreground in 3x8 RGB (rrggbb):
/// ········ 11111111 11111111 11111111 ········ ········ ········ ········  =  ··FFFFFF ········
///
///                             ~background channel~
/// reserved, must be 0:                                 ↓bits view↓               ↓hex mask↓
/// ········ ········ ········ ········ 0······· ········ ········ ········  =  ········ 8·······
///
/// background is *not* "default color":
/// ········ ········ ········ ········ ·1······ ········ ········ ········  =  ········ 4·······
///
/// background alpha (2 bits):
/// ········ ········ ········ ········ ··11···· ········ ········ ········  =  ········ 3·······
///
/// background uses palette index:
/// ········ ········ ········ ········ ····1··· ········ ········ ········  =  ········ ·8······
///
/// reserved, must be 0:
/// ········ ········ ········ ········ ·····000 ········ ········ ········  =  ········ ·7······
///
/// background in 3x8 RGB (rrggbb):
/// 0········ ········ ········ ········ ········11111111 11111111 11111111  =  ········ ··FFFFFF
/// ```
///
/// At render time, these 24-bit values are quantized down to terminal
/// capabilities, if necessary. There's a clear path to 10-bit support should
/// we one day need it, but keep things cagey for now. "default color" is
/// best explained by color(3NCURSES). ours is the same concept. until the
/// "not default color" bit is set, any color you load will be ignored.
///
/// `type in C: channels (uint64_t)`
///
pub type NcChannels = u64;

// NcRgb
//
/// 24 bits broken into 3x 8bpp channels.
///
/// ```txt
/// -------- RRRRRRRR GGGGGGGG BBBBBBBB
/// ```
///
/// `type in C: no data type`
///
pub type NcRgb = u32;

// NcColor
//
/// 8 bits representing a R/G/B color or alpha channel
///
/// ```txt
/// CCCCCCCC (1 Byte)
/// ```
///
/// `type in C: no data type`
///
pub type NcColor = u8;

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
// NOTE: the order of the colors is different than in NcChannel.
pub type NcPixel = u32;

/// NcPalette structure consisting of an array of 256 [`NcChannel`]s
///
/// Some terminals only support 256 colors, but allow the full
/// palette to be specified with arbitrary RGB colors. In all cases, it's more
/// performant to use indexed colors, since it's much less data to write to the
/// terminal. If you can limit yourself to 256 colors, that's probably best.
///
/// `type in C: ncpalette256 (struct)`
///
pub type NcPalette = crate::palette256;

/// 8-bit value used for indexing into a [`NcPalette`]
///
pub type NcPaletteIndex = u8;

/// Context for a palette fade operation
pub type NcFadeCtx = crate::ncfadectx;

/// the [`NcChar`] which form the various levels
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
pub type NcBlitSet = crate::blitset;
