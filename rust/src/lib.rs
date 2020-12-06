//! `libnotcurses-sys` is a *close to the metal* Rust wrapper for the [notcurses
//! C library](https://www.github.com/dankamongmen/notcurses/)
//!
//! The bindings are still incomplete, and a work in progress.
//!
//! There is also the [notcurses](https://crates.io/crates/notcurses) crate,
//! for a safer, more idiomatic wrapper, and with higher level abstractions,
//! but also still very much more incomplete.
//!
//! # Ways of using this library
//!
//! The *rusty* way is to use the provided methods and constructors:
//! ```rust
//! use libnotcurses_sys::*;
//!
//! fn main() {
//!     let nc = Notcurses::without_altscreen();
//!     let plane = nc.stdplane();
//!     plane.putstr("hello world");
//!     nc.render();
//!     nc.stop();
//! }
//! ```
//!
//! You can also use the C API functions directly over the constructed types.
//!
//! Note that some of the functions will be unsafe. And you may also need
//! to (de)reference mutable pointers.
//! This is mainly due to the interaction between the manually reimplemented
//! static inline functions that uses (mutable) references, and the C API
//! functions automatically wrapped by bindgen that uses (mutable) raw pointers.
//!
//! There are plans to manually wrap all the C API functions, in order to
//! achieve better ergonomics and consistent syntax.
//! ```rust
//! use libnotcurses_sys::*;
//!
//! fn main() {
//!     let options = NotcursesOptions::with_flags(NCOPTION_NO_ALTERNATE_SCREEN);
//!     let nc = Notcurses::with_options(&options);
//!     unsafe {
//!         let plane = notcurses_stdplane(nc);
//!         ncplane_putstr(&mut *plane, "hello world");
//!         notcurses_render(nc);
//!         notcurses_stop(nc);
//!     }
//! }
//! ```
//!
//! You can also use it even more closely to the C API if you wish:
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
//!     unsafe {
//!         let nc = notcurses_init(&options, null_mut());
//!         let plane = notcurses_stdplane(nc);
//!         ncplane_putstr(&mut *plane, "hello world");
//!         notcurses_stop(nc);
//!     }
//! }
//!
//! ```
//! ## Limitations of this library
//!
//! There are several common patterns in Rust that this library doesn't employ,
//! and focuses instead on remaining at a very close distance to the C API.
//!
//! 1. There are no Drop trait implementations, therefore you must manually stop
//! each context before it goes out of scope ([Notcurses], [NcDirect]), and
//! should manually destroy [NcPlane]s, [NcMenu]s… when no longer needed.
//!
//! 2. Instead of handling errors with `Result` (as customary in Rust),
//!   several functions return an [NcResult] with a value of [NCRESULT_ERR],
//!   (as customary in C), or [NCRESULT_OK].
//!
//! The [notcurses]() crate overcomes this limitations by using higher level
//! abstractions, while pulling itself apart from the C API, in ways this
//! library can not do.
//!
//! ### Things this library does do
//!
//! - Type aliases every underlying C type to leverage type checking.
//! - Renames types to enforce regularity and consistency. (e.g. [NcCell])
//! - Has handy macros like [sleep], [rsleep] & [cstring].
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

mod cells;
mod channel;
mod direct;
mod error;
mod file;
mod input;
mod key;
mod keycodes;
mod macros;
mod metric;
mod notcurses;
mod palette;
mod pixel;
mod plane;
mod stats;
mod time;
mod visual;
mod widgets;

pub use cells::*;
pub use channel::*;
pub use direct::*;
pub use error::*;
pub use file::*;
pub use input::*;
pub use key::*;
pub use keycodes::*;
pub use macros::*;
pub use metric::*;
pub use notcurses::*;
pub use palette::*;
pub use pixel::*;
pub use plane::*;
pub use stats::*;
pub use time::*;
pub use visual::*;
pub use widgets::*;
