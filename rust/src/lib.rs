//! `libnotcurses-sys` is a Rust wrapper for the [notcurses
//! C library](https://www.github.com/dankamongmen/notcurses/)
//!
//! *This is a work in progress.*
//!
//! # How to use this library
//!
//! Since this library is built with several layers of zero-overhead
//! abstractions over the FFI functions, there are multiple ways to use it.
//!
//! But basically there are two ways:
//!
//! ## 1. The Rust way
//!
//! Use the safely wrapped types, their methods and constructors:
//!
//! ```rust
//! use libnotcurses_sys::*;
//!
//! fn main() -> NcResult<()> {
//!     let mut nc = FullMode::with_flags(NCOPTION_NO_ALTERNATE_SCREEN)?;
//!     let plane = nc.stdplane()?;
//!     plane.putstr("hello world")?;
//!     nc.render()?;
//!     Ok(())
//! }
//! ```
//! Specifically, and for example:
//!
//! [`FullMode`] is the safe wrapper over [`NcNotcurses`], which is the
//! `&mut` reference over the raw `*mut` pointer received from FFI.
//!
//! FullMode implements the [Drop], [AsRef], [AsMut], [Deref][std::ops::Deref]
//! & [DerefMut][std::ops::DerefMut] traits.
//!
//! Most methods are directly implemented for NcNotcurses,
//! and automatically available also from FullMode.
//!
//! The destructor ([notcurses_stop()]) is called automatically at the end
//! of its scope, so you don't ever have to call it by hand.
//!
//! The Rust style methods manage errors by means of returning an
//! [`NcResult`]`<T, `[`NcError`]`>`, for painless handling.
//!
//! ## 2. The C way
//!
//! You can also use the C API functions directly in a very similar way
//! as the underlying C library is used.
//!
//! ```rust
//! use core::ptr::{null, null_mut};
//! use libnotcurses_sys::*;
//!
//! fn main() {
//!     let options = ffi::notcurses_options {
//!         termtype: null(),
//!         renderfp: null_mut(),
//!         loglevel: 0,
//!         margin_t: 0,
//!         margin_r: 0,
//!         margin_b: 0,
//!         margin_l: 0,
//!         flags: NCOPTION_NO_ALTERNATE_SCREEN,
//!     };
//!     // NOTE: there's missing manual checking of return values for errors.
//!     unsafe {
//!         let nc = notcurses_init(&options, null_mut());
//!         let plane = notcurses_stdplane(nc);
//!         ncplane_putstr(&mut *plane, "hello world");
//!         notcurses_stop(nc);
//!     }
//! }
//!
//! ```
//! It requires the use of unsafe.
//!
//! The C style functions handle errors by the means of returning an i32 value
//! aliased to [NcIntResult].
//!
//! ## The `notcurses` C API docs
//!
//! For reference:
//!
//! - [Doxygen Documentation](https://nick-black.com/notcurses/html/index.html)
//! - [API reference (man pages)](https://nick-black.com/notcurses/)
//! - [Wiki](https://nick-black.com/dankwiki/index.php/Notcurses)
//! - [The Book Guide (pdf)](https://nick-black.com/htp-notcurses.pdf)
//! - [USAGE.md](https://github.com/dankamongmen/notcurses/blob/master/USAGE.md)
//!
#![allow(non_upper_case_globals, non_camel_case_types, non_snake_case)]
#![allow(clippy::too_many_arguments)]

mod bindings;
#[doc(inline)]
pub use bindings::*;

mod r#box;
mod cells;
mod channel;
mod dimension;
mod direct;
mod error;
mod fade;
mod file;
mod input;
mod macros;
mod metric;
mod notcurses;
mod palette;
mod pixel;
mod plane;
mod resizecb;
mod signal;
mod stats;
mod time;
mod visual;
mod widgets;

pub use crate::input::*;
pub use cells::*;
pub use channel::*;
pub use dimension::*;
pub use direct::*;
pub use error::*;
pub use fade::*;
pub use file::*;
pub use macros::*;
pub use metric::*;
pub use notcurses::*;
pub use palette::*;
pub use pixel::*;
pub use plane::*;
pub use r#box::*;
pub use resizecb::*;
pub use signal::*;
pub use stats::*;
pub use time::*;
pub use visual::*;
pub use widgets::*;
