#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![no_std]
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
