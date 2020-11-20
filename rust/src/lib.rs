//! `libnotcurses-sys` is an unsafe Rust wrapper for the notcurses C library
//!
//! It follows very closely the C API, adapted to the Rust API Guidelines.
//!
//! For a higher lever API, safer and more idiomatic, take a look at
//! [notcurses-rs](https://github.com/dankamongmen/notcurses-rs)
//!
//! ### notcurses C API docs:
//!
//! - [Doxygen Documentation](https://nick-black.com/notcurses/html/index.html)
//! - [API reference (man pages)](https://nick-black.com/notcurses/)
//! - [Wiki](https://nick-black.com/dankwiki/index.php/Notcurses)
//! - [The Book Guide (pdf)](https://nick-black.com/htp-notcurses.pdf)
//! - [USAGE.md](https://github.com/dankamongmen/notcurses/blob/master/USAGE.md)
//!
#![allow(non_upper_case_globals, non_camel_case_types, non_snake_case)]
#![allow(clippy::too_many_arguments)]

pub mod bindings;
pub mod types;

#[doc(inline)]
pub use bindings::*;
#[doc(inline)]
pub use types::*;

#[macro_use]
mod macros;

mod cells;
mod channel;
mod direct;
mod input;
mod key;
mod keycodes;
mod notcurses;
mod palette;
mod pixel;
mod plane;
mod visual;
mod widgets;

pub use cells::*;
pub use channel::*;
pub use direct::*;
pub use input::*;
pub use key::*;
pub use keycodes::*;
pub use notcurses::*;
pub use palette::*;
pub use pixel::*;
pub use plane::*;
pub use visual::*;
pub use widgets::*;
