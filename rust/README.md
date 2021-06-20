[![Crate](https://img.shields.io/crates/v/libnotcurses-sys.svg)](https://crates.io/crates/libnotcurses-sys)
[![API](https://docs.rs/libnotcurses-sys/badge.svg)](https://dankamongmen.github.io/notcurses/rustdoc/libnotcurses_sys/)
[![MSRV: 1.48.0](https://flat.badgen.net/badge/MSRV/1.48.0/purple)](https://blog.rust-lang.org/2020/11/19/Rust-1.48.html)

`libnotcurses-sys` is a low-level Rust wrapper for the
[notcurses C library](https://www.github.com/dankamongmen/notcurses/)

This library is built with several layers of zero-overhead abstractions
over the C functions and pointers accessed through FFI.

# How to use this library

There are basically two ways: The [**Rust way**](#1-the-rust-way),
and the [**C way**](#2-the-c-way). (Or a mix of both).

## 1. The Rust way

Where you use the more safely wrapped types, with its methods and constructors,
and painless error handling, like this:

```rust
use libnotcurses_sys::*;

fn main() -> NcResult<()> {
    let mut nc = Nc::with_flags(NCOPTION_NO_ALTERNATE_SCREEN)?;
    let plane = nc.stdplane();
    plane.putstr("hello world")?;
    nc.render()?;
    nc.stop()?;
    Ok(())
}
```

Although you still have to manually call the `stop()` method for `Nc`
and `NcDirect` objects, and the `destroy()` method for the rest of types that
allocate, (like `NcPlane`, `NcMenu`…) at the end of their scope, since the Drop
trait is not implemented for any wrapping type in libnotcurses-sys.

But they do implement methods and use `NcResult` as the return type,
for handling errors in the way we are used to in Rust.

For the types that don't allocate, most are based on primitives like `i32`,
`u32`, `u64`… without a name in the C library. In Rust they are type aliased
(e.g.: `NcChannel`, `NcChannelPair`, `NcRgb`, `NcColor`…), to
leverage type checking, and they implement methods through traits
(e.g. `NcChannelMethods` must be in scope to use the `NcChannel` methods.

## 2. The C way

You can always use the C API functions directly if you prefer,
in a very similar way as the C library is used.

It requires the use of unsafe, since most functions are wrapped directly
by `bindgen` marked as such.

Error handling is done this way by checking the returned `NcIntResult`,
or in case of receiving a pointer, by comparing it to `null_mut()`.

### Example

```rust
use core::ptr::{null, null_mut};
use std::process::exit;

use libnotcurses_sys::*;

fn main() {
    let options = ffi::notcurses_options {
        termtype: null(),
        renderfp: null_mut(),
        loglevel: 0,
        margin_t: 0,
        margin_r: 0,
        margin_b: 0,
        margin_l: 0,
        flags: NCOPTION_NO_ALTERNATE_SCREEN,
    };
    unsafe {
        let nc = notcurses_init(&options, null_mut());
        if nc == null_mut() {
            exit(1);
        }
        let plane = notcurses_stdplane(nc);
        let cols = ncplane_putstr(&mut *plane, "hello world");
        if cols < NCRESULT_OK {
            notcurses_stop(nc);
            exit(cols.abs());
        }
        if notcurses_stop(nc) < NCRESULT_OK {
            exit(2);
        }
    }
}

```

### The `notcurses` C API docs

- [API reference (man pages)](https://notcurses.com/)
- [Wiki Page](https://nick-black.com/dankwiki/index.php/Notcurses)
- [The Book Guide (pdf)](https://nick-black.com/htp-notcurses.pdf)
- [USAGE.md](https://github.com/dankamongmen/notcurses/blob/master/USAGE.md)
- [HACKING.md](https://github.com/dankamongmen/notcurses/blob/master/doc/HACKING.md)
- [Doxygen Documentation](https://nick-black.com/notcurses/html/index.html)
- [FOSDEM 2021 presentation](https://fosdem.org/2021/schedule/event/notcurses/)

