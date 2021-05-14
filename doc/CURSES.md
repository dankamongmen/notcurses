# Notcurses viz Curses/NCURSES

The biggest difference, of course, is that Notcurses is neither an
implementation of X/Open (aka XSI) Curses, nor part of SUS4-2018.

## Differences from Curses

The detailed differences between Notcurses and NCURSES (a high-quality, ubiquitous
implementation of Curses) probably can't be fully enumerated, and if they
could, no one would want to read them. With that said, some design decisions
might surprise NCURSES programmers:

* There is no distinct `PANEL` type. The z-buffer is a fundamental property,
  and all drawable surfaces are ordered along the z axis. There is no
  equivalent to `update_panels()`.
* Scrolling is disabled by default, and cannot be globally enabled (but
  see Direct Mode).
* The Curses `cchar_t` has a fixed-size array of `wchar_t`. The Notcurses
  `nccell` instead supports a UTF-8 encoded extended grapheme cluster of
  arbitrary length. The only supported encodings are ASCII via `ANSI_X3.4-1968`
  and Unicode via `UTF-8`.
* The cursor is disabled by default, when supported (`civis` capability).
* Echoing of input is disabled by default, and `cbreak` mode is used by default.
* Colors are usually specified as 24 bits in 3 components (RGB). If necessary,
  these will be quantized for the actual terminal. There are no "color pairs",
  but indexed palettes are supported.
* There is no distinct "pad" concept (these are NCURSES `WINDOW`s created with
  the `newpad()` function). All drawable surfaces can exceed the display size.
* Multiple threads can freely call into Notcurses, so long as they're not
  accessing the same data. In particular, it is always safe to concurrently
  mutate different `ncplane`s in different threads.
* NCURSES has thread-ignorant and thread-semi-safe versions, trace-enabled and
  traceless versions, and versions with and without support for wide characters.
  Notcurses is one library: no tracing, UTF-8, thread safety.
* There is no `ESCDELAY` concept; Notcurses expects that all bytes of an
  escape sequence arrive at the same time. This improves latency and simplifies
  the API.
* It is an error in NCURSES to print to the bottommost, rightmost coordinate of
  the screen when scrolling is disabled (because the cursor cannot be advanced).
  Failure to advance the cursor does not result in an error in Notcurses (but
  attempting to print at the cursor when it has been advanced off the plane *does*).
* Notcurses has no support for soft labels (`slk_init()`, etc.), subwindows
  which share memory with their parents, nor the NCURSES tracing functionality
  (`trace(3NCURSES)`).
* Notcurses doesn't implement any of the `curs_util(3x)` functions, including
  window serialization/deserialization via `putwin()`/`getwin()`.
* Notcurses doesn't interact with `LINES` nor `COLUMNS` environment variables.

## Adapting NCURSES programs

Do you really want to do such a thing? NCURSES and the Curses API it implements
are far more portable and better-tested than Notcurses is ever likely to be.
Will your program really benefit from Notcurses's advanced features? If not,
it's probably best left as it is.

Otherwise, most NCURSES concepts have clear partners in Notcurses. Any functions
making implicit use of `stdscr` ought be replaced with their explicit
equivalents. `stdscr` ought then be replaced with the result of
`notcurses_stdplane()` (the standard plane). `PANEL`s become `ncplane`s; the
Panels API is otherwise pretty close. Anything writing a bare character will
become a simple `nccell`; multibyte or wide characters become complex `nccell`s.
Color no longer uses "color pairs". You can easily enough hack together a
simple table mapping your colors to RGB values, and color pairs to foreground
and background indices into said table. That'll work for the duration of a
porting effort, certainly.

I have adapted two large (~5k lines of C UI code each) programs from NCURSES to
Notcurses, and found it a fairly painless process. It was helpful to introduce
a shim layer, e.g. `compat_mvwprintw` for NCURSES's `mvwprintw`:

```c
static int
compat_mvwprintw(struct ncplane* nc, int y, int x, const char* fmt, ...){
  va_list va;
  va_start(va, fmt);
  if(ncplane_vprintf_yx(nc, y, x, fmt, va) < 0){
    va_end(va);
    return ERR;
  }
  va_end(va);
  return OK;
}
```

These are pretty obvious, implementation-wise.

### Some details

* `cbreak()`/`nocbreak()`/`echo()`/`noecho()`/`nl()`/`nonl()`: termios
  properties are not exposed as granularly by Notcurses. Rendered mode
  always enters cbreak mode. Direct mode enters cbreak mode by default,
  but `NCDIRECT_OPTION_INHIBIT_CBREAK` will inhibit this.
* `raw()`/`noraw()`: The line discipline conversions (e.g. Ctrl+C) can be
  disabled at any time with `notcurses_linesigs_disable()`, and turned back on
  with `notcurses_linesigs_enable()`.
* `keypad()`: The keypad is always enabled, if the `smkx`
  capability is advertised.
* `halfdelay()`/`nodelay()`/`timeout()`/`wtimeout()`: No such global controls
  are supported. Use `notcurses_getc()` with a timeout if you want a timeout.
  Use `notcurses_getc_nblock()` if you want an immediate return.
* `intrflush()`/`qiflush()`/`noqiflush()`: No such functionality is supported.
* `typeahead()`: No such functionality is supported.
