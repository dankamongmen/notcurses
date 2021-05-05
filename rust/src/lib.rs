//! `libnotcurses-sys` is a Rust wrapper for the [notcurses
//! C library](https://www.github.com/dankamongmen/notcurses/)
//!
//! *This is a work in progress.*
//!
//! This library is built with several layers of zero-overhead abstractions
//! over the C functions and pointers accessed through FFI.
//!
//! # How to use this library
//!
//! There are basically two ways: The [**Rust way**](#1-the-rust-way),
//! and the [**C way**](#2-the-c-way). (Or a mix of both).
//!
//! ## 1. The Rust way
//!
//! Where you use the safely wrapped types, with its methods and constructors,
//! and painless error handling, like this:
//!
//! ### Example
//!
//! ```rust
//! use libnotcurses_sys::*;
//!
//! fn main() -> NcResult<()> {
//!     let mut nc = Notcurses::with_flags(NCOPTION_NO_ALTERNATE_SCREEN)?;
//!     let plane = nc.stdplane();
//!     plane.putstr("hello world")?;
//!     nc.render()?;
//!     nc.stop()?;
//!     Ok(())
//! }
//! ```
//!
//! Specifically [`Nc`] and [`NcD`] are safe wrappers over [`Notcurses`]
//! and [`NcDirect`], respectively.
//!
//! `Nc` and `NcD` both implement the [Drop], [AsRef], [AsMut],
//! [Deref][std::ops::Deref] and [DerefMut][std::ops::DerefMut] traits.
//!
//! Their destructors are called automatically at the end of their scope.
//!
//! Methods are directly implemented for `Notcurses` and `NcDirect`, and are
//! made automatically available to `Nc` & `NcD`, minus some function overrides,
//! like their destructors, plus the static methods that have to be recreated.
//!
//! The rest of the types that allocate, like [`NcPlane`], [`NcMenu`],
//! [`NcReader`]… have no higher level wrapping struct, they don't
//! implement Drop, so they have to be `*.destroy()`ed manually.
//!
//! But they do implement methods and use [`NcResult`] as the return type,
//! for handling errors in the way we are used to in Rust.
//!
//! For the types that don't allocate, most are based on primitives like `i32`,
//! `u32`, `u64`… without a name in the C library. In Rust they are type aliased
//! (e.g.: [`NcChannel`], [`NcChannelPair`], [`NcRgb`], [`NcColor`]…), to
//! leverage type checking, and they implement methods through [traits](#traits)
//! (e.g. [`NcChannelMethods`] must be in scope to use the `NcChannel` methods.
//!
//!
//! ## 2. The C way
//!
//! You can always use the C API functions directly if you prefer,
//! in a very similar way as the C library is used.
//!
//! It requires the use of unsafe, since most functions are wrapped directly
//! by `bindgen` marked as such.
//!
//! Error handling is done this way by checking the returned [`NcIntResult`],
//! or in case of receiving a pointer, by comparing it
//! to [null_mut()][core::ptr::null_mut].
//!
//! ### Example
//!
//! ```rust
//! use core::ptr::{null, null_mut};
//! use std::process::exit;
//!
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
//!     unsafe {
//!         let nc = notcurses_init(&options, null_mut());
//!         if nc.is_null() {
//!             exit(1);
//!         }
//!         let plane = notcurses_stdplane(nc);
//!         let cols = ncplane_putstr(&mut *plane, "hello world");
//!         if cols < NCRESULT_OK {
//!             notcurses_stop(nc);
//!             exit(cols.abs());
//!         }
//!         if notcurses_stop(nc) < NCRESULT_OK {
//!             exit(2);
//!         }
//!     }
//! }
//!
//! ```
//!
//! ### The `notcurses` C API docs
//!
//! - [API reference (man pages)](https://notcurses.com/)
//! - [Wiki Page](https://nick-black.com/dankwiki/index.php/Notcurses)
//! - [The Book Guide (pdf)](https://nick-black.com/htp-notcurses.pdf)
//! - [USAGE.md](https://github.com/dankamongmen/notcurses/blob/master/USAGE.md)
//! - [HACKING.md](https://github.com/dankamongmen/notcurses/blob/master/doc/HACKING.md)
//! - [Doxygen Documentation](https://nick-black.com/notcurses/html/index.html)
//!
#![allow(non_upper_case_globals, non_camel_case_types, non_snake_case)]
#![allow(clippy::too_many_arguments, clippy::needless_doctest_main)]

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
