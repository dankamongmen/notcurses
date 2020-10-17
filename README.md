# Notcurses, a blingful TUI/character graphics library

* **What it is**: a library facilitating complex TUIs on modern terminal
    emulators, supporting vivid colors, multimedia, and Unicode to the maximum
    degree possible. [Things](https://www.youtube.com/watch?v=b4lmMADP1lA) can
    be done with Notcurses that simply can't be done with NCURSES.

* **What it is not**: a source-compatible X/Open Curses implementation, nor a
    replacement for NCURSES on existing systems.

birthed screaming into this world by [nick black](https://nick-black.com/dankwiki/index.php/Hack_on) (<nickblack@linux.com>).
C++ wrappers by [marek habersack](http://twistedcode.net/blog/) (<grendel@twistedcode.net>). the Notcurses API
is stable as of version 2.0.

for more information, see [dankwiki](https://nick-black.com/dankwiki/index.php/Notcurses)
and the [man pages](https://notcurses.com/notcurses). there's also a reference
[in this repo](USAGE.md). in addition, there is
[Doxygen](https://nick-black.com/notcurses/html/) output. there is a
[mailing list](https://groups.google.com/forum/#!forum/notcurses) which can be reached
via notcurses@googlegroups.com. i wrote a coherent
[guidebook](https://nick-black.com/htp-notcurses.pdf), which is available for
free download, or [paperback purchase](https://amazon.com/dp/B086PNVNC9).

i've not yet added many documented examples, but there are many small
C/C++ programs available in [src/poc/](https://github.com/dankamongmen/notcurses/tree/master/src/poc).

**If you're running Notcurses applications in a Docker, please consult
"[Environment notes](#environment-notes)" below.** I track some [TERMS.md](capabilities of terminal
emultators) and also maintain a list of [OTHERS.md](other TUI libraries).

<a href="https://drone.dsscaw.com:4443/dankamongmen/notcurses">
<img src="https://drone.dsscaw.com:4443/api/badges/dankamongmen/notcurses/status.svg" alt="Build status">
</a>

<a href="https://repology.org/project/notcurses/versions">
<img src="https://repology.org/badge/vertical-allrepos/notcurses.svg" alt="Packaging status" align="right">
</a>

* [Introduction](#introduction)
* [Requirements](#requirements)
  * [Building](#building)
* [Included tools](#included-tools)
* [Differences from NCURSES](#differences-from-ncurses)
  * [Features missing relative to NCURSES](#features-missing-relative-to-ncurses)
  * [Adapting NCURSES programs](#adapting-ncurses-programs)
* [Environment notes](#environment-notes)
  * [TrueColor detection](#TrueColor-detection)
  * [Fonts](#fonts)
  * [FAQs](#faqs)
* [Supplemental material](#supplemental-material)
  * [Useful links](#useful-links)
  * [History](#history)
  * [Thanks](#thanks)

## Introduction

Notcurses abandons the X/Open Curses API bundled as part of the Single UNIX
Specification. The latter shows its age, and seems not capable of making use of
terminal functionality such as unindexed 24-bit color ("TrueColor", not to be
confused with the 8-bit indexed 24-bit "extended color" of NCURSES).
For some necessary background, consult Thomas E. Dickey's
superb and authoritative [NCURSES FAQ](https://invisible-island.net/ncurses/ncurses.faq.html#xterm_16MegaColors).
As such, Notcurses is not a drop-in Curses replacement. It is almost certainly
less portable, and definitely tested on less hardware. Sorry about that.
Ultimately, I hope to properly support all terminals *supporting the features
necessary for complex TUIs*. I would argue that teletypes etc. are
fundamentally unsuitable. Most operating systems seem reasonable targets, but I
only have Linux and FreeBSD available for testing.

Whenever possible, Notcurses makes use of the Terminfo library shipped with
NCURSES, benefiting greatly from its portability and thoroughness.

Notcurses opens up advanced functionality for the interactive user on
workstations, phones, laptops, and tablets, at the expense of e.g.
some industrial and retail terminals.

Why use this non-standard library?

* Thread safety, and efficient use in parallel programs, has been a design
  consideration from the beginning.

* A svelter design than that codified by X/Open:
  * Exported identifiers are prefixed to avoid common namespace collisions.
  * The library object exports a minimal set of symbols. Where reasonable,
    `static inline` header-only code is used. This facilitates compiler
    optimizations, and reduces loader time.

* All APIs natively support the Universal Character Set (Unicode). The `cell`
  API is based around Unicode's [Extended Grapheme Cluster](https://unicode.org/reports/tr29/) concept.

* Visual features including images, fonts, video, high-contrast text, sprites,
  and transparent regions. All APIs natively support 24-bit color, quantized
  down as necessary for the terminal.

* It's Apache2-licensed in its entirety, as opposed to the
  [drama in several acts](https://invisible-island.net/ncurses/ncurses-license.html)
  that is the NCURSES license (the latter is [summarized](https://invisible-island.net/ncurses/ncurses-license.html#issues_freer)
  as "a restatement of MIT-X11").

Much of the above can be had with NCURSES, but they're not what NCURSES was
*designed* for. The most fundamental advantage in my mind, though, is
that Notcurses is of the multithreaded era. On the other hand, if you're
targeting industrial or critical applications, or wish to benefit from the
time-tested reliability and portability of Curses, you should by all means use
that fine library.

## Requirements

* (build) A C11 and a C++17 compiler
* (build) CMake 3.14.0+
* (build+runtime) From NCURSES: terminfo 6.1+
* (build+runtime) GNU libunistring 0.9.10+
* (OPTIONAL) (build+runtime) From QR-Code-generator: [libqrcodegen](https://github.com/nayuki/QR-Code-generator) 1.5.0+
* (OPTIONAL) (build+runtime) From [FFmpeg](https://www.ffmpeg.org/): libswscale 5.0+, libavformat 57.0+, libavutil 56.0+
* (OPTIONAL) (build+runtime) [OpenImageIO](https://github.com/OpenImageIO/oiio) 2.15.0+
* (OPTIONAL) (testing) [Doctest](https://github.com/onqtam/doctest) 2.3.5+
* (OPTIONAL) (documentation) [pandoc](https://pandoc.org/index.html) 1.19.2+
* (OPTIONAL) (python bindings): Python 3.7+, [CFFI](https://pypi.org/project/cffi/) 1.13.2+, [pypandoc](https://pypi.org/project/pypandoc/) 1.5+
* (OPTIONAL) (rust bindings): rust 1.40.0+, cargo 0.40.0+, [bindgen](https://crates.io/crates/bindgen) 0.53.0+
* (runtime) Linux 5.3+ or FreeBSD 11+

### Building

* Create a subdirectory, traditionally `build`. Enter the directory.
* `cmake ..`. You might want to set e.g. `CMAKE_BUILD_TYPE`.
* `make`
* `make test`
* `make install`

The default multimedia engine is FFmpeg. You can select a different engine
using `USE_MULTIMEDIA`. Valid values are `ffmpeg`, `oiio` (for OpenImageIO),
or `none`. Without a multimedia engine, Notcurses will be unable to decode
images and videos.

Run unit tests with `make test` following a successful build. If you have unit
test failures, *please* file a bug including the output of

`./notcurses-tester -p ../data`

(`make test` also runs `notcurses-tester`, but hides important output).

Install with `make install` following a successful build.

To watch the bitchin' demo, run `./notcurses-demo -p ../data`. More details can
be found on the `notcurses-demo(1)` man page.

This does not install the Python or Rust wrappers.

#### Build options

To set the C compiler, export `CC`. To set the C++ compiler, export `CXX`. The
`CMAKE_BUILD_TYPE` CMake variable can be defined to any of its standard values,
but must be `Debug` for use of `USE_COVERAGE`.

* `DFSG_BUILD`: leave out all content considered non-free under the Debian Free
                Software Guidelines
* `BUILD_TESTING`: build test targets
* `USE_COVERAGE`: build coverage support (for developers, requires use of Clang)
* `USE_DOCTEST`: build `notcurses-tester` with Doctest, requires `BUILD_TESTING`
* `USE_DOXYGEN`: build interlinked HTML documentation with Doxygen
* `USE_MULTIMEDIA`: `ffmpeg` for FFmpeg, `oiio` for OpenImageIO, `none` for none
* `USE_PANDOC`: build man pages with pandoc
* `USE_POC`: build small, uninstalled proof-of-concept binaries
* `USE_QRCODEGEN`: build qrcode support via libqrcodegen
* `USE_STATIC`: build static libraries (in addition to shared ones)

## Included tools

Six binaries are installed as part of notcurses:
* `notcurses-demo`: some demonstration code
* `notcurses-view`: renders visual media (images/videos)
* `notcurses-input`: decode and print keypresses
* `notcurses-planereels`: play around with ncreels
* `notcurses-tester`: unit testing
* `notcurses-tetris`: a tetris clone
* `ncneofetch`: a [neofetch](https://github.com/dylanaraps/neofetch) ripoff

To run `notcurses-demo` from a checkout, provide the `tests/` directory via
the `-p` argument. Demos requiring data files will otherwise abort. The base
delay used in `notcurses-demo` can be changed with `-d`, accepting a
floating-point multiplier. Values less than 1 will speed up the demo, while
values greater than 1 will slow it down.

`notcurses-tester` expects `../tests/` to exist, and be populated with the
necessary data files. It can be run by itself, or via `make test`.

## Differences from NCURSES

The biggest difference, of course, is that Notcurses is not an implementation
of X/Open (aka XSI) Curses, nor part of SUS4-2018.

The detailed differences between Notcurses and NCURSES probably can't be fully
enumerated, and if they could, no one would want to read them. With that said,
some design decisions might surprise NCURSES programmers:

* There is no distinct `PANEL` type. The z-buffer is a fundamental property,
  and all drawable surfaces are ordered along the z axis. There is no
  equivalent to `update_panels()`.
* Scrolling is disabled by default, and cannot be globally enabled.
* The Curses `cchar_t` has a fixed-size array of `wchar_t`. The notcurses
  `cell` instead supports a UTF-8 encoded extended grapheme cluster of
  arbitrary length. The only supported encodings are ASCII via `ANSI_X3.4-1968`
  and Unicode via `UTF-8`.
* The cursor is disabled by default, when supported (`civis` capability).
* Echoing of input is disabled by default, and `cbreak` mode is used by default.
* Colors are always specified as 24 bits in 3 components (RGB). If necessary,
  these will be quantized for the actual terminal. There are no "color pairs".
* There is no distinct "pad" concept (these are NCURSES `WINDOW`s created with
  the `newpad()` function). All drawable surfaces can exceed the display size.
* Multiple threads can freely call into notcurses, so long as they're not
  accessing the same data. In particular, it is always safe to concurrently
  mutate different `ncplane`s in different threads.
* NCURSES has thread-ignorant and thread-semi-safe versions, trace-enabled and
  traceless versions, and versions with and without support for wide characters.
  Notcurses is one library: no tracing, UTF-8, thread safety.
* There is no `ESCDELAY` concept; Notcurses expects that all bytes of a
  keyboard escape sequence arrive at the same time. This improves latency
  and simplifies the API.
* It is an error in NCURSES to print to the bottommost, rightmost coordinate of
  the screen when scrolling is disabled (because the cursor cannot be advanced).
  Failure to advance the cursor does not result in an error in Notcurses (but
  attempting to print at the cursor when it has been advanced off the plane
  *does*).

### Features missing relative to NCURSES

This isn't "features currently missing", but rather "features I do not intend
to implement".

* There is no support for soft labels (`slk_init()`, etc.).
* There is no concept of subwindows which share memory with their parents.
* There is no tracing functionality ala `trace(3NCURSES)`. Superior external
  tracing solutions exist, such as `bpftrace`.

### Adapting NCURSES programs

Do you really want to do such a thing? NCURSES and the Curses API it implements
are far more portable and better-tested than Notcurses is ever likely to be.
Will your program really benefit from notcurses's advanced features? If not,
it's probably best left as it is.

Otherwise, most NCURSES concepts have clear partners in notcurses. Any functions
making implicit use of `stdscr` ought be replaced with their explicit
equivalents. `stdscr` ought then be replaced with the result of
`notcurses_stdplane()` (the standard plane). `PANEL`s become `ncplane`s; the
Panels API is otherwise pretty close. Anything writing a bare character will
become a simple `cell`; multibyte or wide characters become complex `cell`s.
Color no longer uses "color pairs". You can easily enough hack together a
simple table mapping your colors to RGB values, and color pairs to foreground
and background indices into said table. That'll work for the duration of a
porting effort, certainly.

I have adapted two large (~5k lines of C UI code each) programs from NCURSES to
notcurses, and found it a fairly painless process. It was helpful to introduce
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

## Environment notes

* If your `TERM` variable is wrong, or that terminfo definition is out-of-date,
  you're going to have a very bad time. Use *only* the `TERM` values
  appropriate for your terminal.

* Ensure your `LANG` environment variable is set to a UTF8-encoded locale, and
  that this locale has been generated. This usually means
  `"[language]_[Countrycode].UTF-8"`, i.e. `en_US.UTF-8`. The first part
  (`en_US`) ought exist as a directory or symlink in `/usr/share/locales`.
  This usually requires editing `/etc/locale.gen` and running `locale-gen`.
  On Debian systems, this can be accomplished with `dpkg-reconfigure loclaes`,
  and enabling the desired locale. The default locale is stored somewhere like
  `/etc/default/locale`.

* If your terminal has an option about default interpretation of "ambiguous-width
  characters" (this is actually a technical term from Unicode), ensure it is
  set to **Wide**, not narrow. If that doesn't work, ensure it is set to
  **Narrow**, heh.

* If you can disable BiDi in your terminal, do so while running notcurses
  applications, until I have that handled better. Notcurses doesn't recognize
  the BiDi state machine transitions, and thus merrily continues writing
  left-to-right. Likewise, ultra-wide glyphs will have interesting effects.
  ﷽!

* The unit tests assume dimensions of at least 80x24. They might work in a
  smaller terminal. They might not. Don't file bugs on it.

### TrueColor detection

notcurses aims to use only information found in the terminal's terminfo entry to detect capabilities, TrueColor
being one of them. Support for this is indicated by terminfo having a flag, added in NCURSES 6.1, named `RGB` set
to `true`. However, as of today there are few and far between terminfo entries which have the capability in their
database entry and so TrueColor won't be used in most cases. Terminal emulators have had for years a kludge to
work around this limitation of terminfo in the form of the `COLORTERM` environment variable which, if set to either
`truecolor` or `24bit` does the job of indicating the capability of sending the escapes 48 and 38 together with a
tripartite RGB (0 ≤ c ≤ 255 for all three components) to specify fore- and background colors.
Checking for `COLORTERM` admittedly goes against the goal stated at the top of this section but, for all practical
purposes, makes the detection work quite well **today**.

### Fonts

Fonts end up being a whole thing, little of which is pleasant. I'll write this
up someday **FIXME**.

### FAQs

If things break or seem otherwise lackluster, **please** consult the
[Environment Notes](#environment_notes) section! You **need** to have a correct
`TERM` and `LANG` definition, and probably want `COLORTERM`.

* **Q:** The demo fails in the middle of `intro`. **A:** Check that your `TERM`
value is correct for your terminal. `intro` does a palette fade, which is prone
to breaking under incorrect `TERM` values. If you're not using `xterm`, your
`TERM` should not be `xterm`!

* **Q:** In `xterm`, Alt doesn't work as expected. **A:** Check out the `eightBitInput` resource of `xterm`. Add `XTerm*eightBitInput: false` to your `$HOME/.Xresources`, and run `xrdb -a $HOME/.Xresources`.

* **Q:** Notcurses looks like absolute crap in `screen`. **A:** `screen` doesn't support RGB colors (at least as of 4.08.00); if you have `COLORTERM` defined, you'll have a bad time. If you have a `screen` that was compiled with `--enable-colors256`, try exporting `TERM=screen-256color` as opposed to `TERM=screen`.

* **Q:** Why didn't you just use Sixel? **A:** Many terminal emulators don't support Sixel. Sixel doesn't work well with mouse selection. With that said, I do intend to support Sixel soon, as a backend, when available, for certain types of drawing (see [issue #200](https://github.com/dankamongmen/notcurses/issues/200)).

* **Q:** I'm not seeing `NCKEY_RESIZE` until I press some other key. **A:** You've almost certainly failed to mask `SIGWINCH` in some thread, and that thread is receiving the signal instead of the thread which called `notcurses_getc_blocking()`. As a result, the `poll()` is not interrupted. Call `pthread_sigmask()` before spawning any threads.

* **Q:** One of the demos claimed to spend more than 100% of its runtime rendering. Do you know how to count? **A:** Runtime is wall clock time. A multithreaded demo can spend more than the wall-clock time rendering if multiple threads run concurrently.

* **Q:** Using the C++ wrapper, how can I ensure that the `NotCurses` destructor is run when I return from `main()`? **A:** As noted in the [C++ FAQ](https://isocpp.org/wiki/faq/dtors#artificial-block-to-control-lifetimes), wrap it in an artificial scope (this assumes your `NotCurses` is scoped to `main()`).

* **Q:** How do I hide a plane I want to make visible later? **A:** Either move it above and to the left of the screen (preventing resizes from making it visible), or place it underneath another (opaque) plane (the latter performs better).

* **Q:** Why isn't there an `ncplane_box_yx()`? Do you hate orthogonality, you dullard? **A:** `ncplane_box()` and friends already have far too many arguments, you monster.

* **Q:** Why doesn't Notcurses support 10- or 16-bit color? **A:** Notcurses supports 24 bits of color, spread across three eight-bit channels. You presumably mean 10-bit-per-channel color. Notcurses will support it when a terminal supports it.

* **Q:** The name is dumb. **A:** That's not a question?

* **Q:** I'm not finding qrcodegen on FreeBSD, despite having installed `graphics/qr-code-generator`. **A:** Try `cmake -DCMAKE_REQUIRED_INCLUDES=/usr/local/include`. This is passed by `bsd.port.mk`.

* **Q:** Do you support [musl](https://musl.libc.org/)? **A:** I try to! You'll need at least 1.20.

* **Q:** I only seem to blit in ASCII, and/or can't emit Unicode beyond ASCII in general. **A:** Your `LANG` environment variable is underdefined or incorrectly defined, or the necessary locale is not present on your machine (it is also possible that you explicitly supplied `NCOPTION_INHIBIT_SETLOCALE`, but never called `setlocale(3)`, in which case don't do that).

* **Q:** I pretty much always need an `ncplane` when using a `cell`. Why doesn't the latter hold a pointer to the former? **A:** Besides the massive redundancy this would entail, `cell` needs to remain as small as possible, and you almost always have the `ncplane` handy if you've got a reference to a valid `cell` anyway.

* **Q:**: I ran `notcurses-demo` with a single demo, but my summary numbers don't match that demo's numbers, you charlatan. **A:** `notcurses-demo` renders several frames beyond the actual demos.

* **Q:**: When my program exits, I don't have a cursor, or text is invisible, or colors are weird, <i>ad nauseam</i>. **A:** Ensure you're calling `notcurses_stop()`/`ncdirect_stop()` on all exit paths, including fatal signals.

* **Q:**: How can I use Direct Mode in conjunction with libreadline? **A:** Pass `NCDIRECT_OPTION_CBREAK` to `ncdirect_init()`, call `ncdirect_init()` prior to calling `rl_prep_terminal()`, and call `rl_deprep_terminal()` before calling `ncdirect_stop()`. But you should probably just use a Notcurses `ncreader`, if possible.

* **Q:**: Will there ever be Java wrappers? **A:** I should hope not.

## Supplemental material

### Useful links

* [BiDi in Terminal Emulators](https://terminal-wg.pages.freedesktop.org/bidi/)
* [The Xterm FAQ](https://invisible-island.net/xterm/xterm.faq.html)
  * [XTerm Control Sequences](https://invisible-island.net/xterm/ctlseqs/ctlseqs.pdf)
* [The NCURSES FAQ](https://invisible-island.net/ncurses/ncurses.faq.html)
* [ECMA-35 Character Code Structure and Extension Techniques](https://www.ecma-international.org/publications/standards/Ecma-035.htm) (ISO/IEC 2022)
* [ECMA-43 8-bit Coded Character Set Structure and Rules](https://www.ecma-international.org/publications/standards/Ecma-043.htm)
* [ECMA-48 Control Functions for Coded Character Sets](https://www.ecma-international.org/publications/standards/Ecma-048.htm) (ISO/IEC 6429)
* [Unicode 12.1 Full Emoji List](https://unicode.org/emoji/charts/full-emoji-list.html)
* [Unicode Standard Annex #29 Text Segmentation](http://www.unicode.org/reports/tr29)
* [Unicode Standard Annex #15 Normalization Forms](https://unicode.org/reports/tr15/)
* [The TTY demystified](http://www.linusakesson.net/programming/tty/)
* [Dark Corners of Unicode](https://eev.ee/blog/2015/09/12/dark-corners-of-unicode/)
* [UTF-8 Decoder Capability and Stress Test](https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt)
* [Emoji: how do you get from U+1F355 to 🍕?](https://meowni.ca/posts/emoji-emoji-emoji/)
* [Glyph Hell: An introduction to glyphs, as used and defined in the FreeType engine](http://chanae.walon.org/pub/ttf/ttf_glyphs.htm)

#### Useful man pages
* Linux: [console_codes(4)](http://man7.org/linux/man-pages/man4/console_codes.4.html)
* Linux: [termios(3)](http://man7.org/linux/man-pages/man3/termios.3.html)
* Linux: [ioctl_tty(2)](http://man7.org/linux/man-pages/man2/ioctl_tty.2.html)
* Linux: [ioctl_console(2)](http://man7.org/linux/man-pages/man2/ioctl_console.2.html)
* Portable: [terminfo(5)](http://man7.org/linux/man-pages/man5/terminfo.5.html)
* Portable: [user_caps(5)](http://man7.org/linux/man-pages/man5/user_caps.5.html)

### History

* 2020-10-12: Notcurses [2.0.0 "stankonia"](https://github.com/dankamongmen/notcurses/releases/tag/v2.0.0).
  * A stable API! Semantic versioning is now in effect. This API will be supported going forward.
* 2020-08-30: Notcurses [1.7.0 "don't pull the bang out"](https://github.com/dankamongmen/notcurses/releases/tag/v1.7.0).
* 2020-07-04: Notcurses [1.6.0 "aquemini"](https://github.com/dankamongmen/notcurses/releases/tag/v1.6.0).
* 2020-07-03: Notcurses is [accepted into Alpine Edge](https://gitlab.alpinelinux.org/alpine/aports/-/merge_requests/9924).
* 2020-06-08: Notcurses [1.5.0 "ghetto bird"](https://github.com/dankamongmen/notcurses/releases/tag/v1.5.0).
* 2020-05-13: Notcurses is [accepted into Fedora Core](https://bugzilla.redhat.com/show_bug.cgi?id=1822971).
* 2020-05-10: Notcurses [1.4.0 "the saga continues"](https://github.com/dankamongmen/notcurses/releases/tag/v1.4.0).
* 2020-05-09: Notcurses is [accepted into FreeBSD](https://github.com/dankamongmen/notcurses/issues/575).
* 2020-04-19: Notcurses is [accepted into Debian](https://bugs.debian.org/950492).
* 2020-04-12: Notcurses [1.3.0 "hypnotize"](https://github.com/dankamongmen/notcurses/releases/tag/v1.3.0).
  * A new [hype video](https://vimeo.com/410787965) is never really publicized.
* 2020-04-08: The Notcurses book [is published](https://amazon.com/dp/B086PNVNC9).
* 2020-03-23: Notcurses is featured on [Linux World News](https://lwn.net/Articles/815811/).
* 2020-02-17: Notcurses [1.2.0 "check the résumé, my record's impeccable"](https://github.com/dankamongmen/notcurses/releases/tag/v1.2.0).
* 2019-01-19: Notcurses [1.1.0 "all the hustlas they love it just to see one of us make it"](https://github.com/dankamongmen/notcurses/releases/tag/v1.1.0).
    Much better video support, pulsing planes, palette256.
  * The new [hype video](https://www.youtube.com/watch?v=-H1WkopWJNMk) gets a lot of attention.
* 2019-01-04: Notcurses [1.0.0 "track team, crack fiend, dying to geek"](https://github.com/dankamongmen/notcurses/releases/tag/v1.0.0)
    is released, six days ahead of schedule. 147 issues closed. 702 commits.
* 2019-12-18: Notcurses [0.9.0 "You dig in! You dig out! You get out!"](https://github.com/dankamongmen/notcurses/releases/tag/v0.9.0),
    and also the first contributor besides myself (@grendello). Last major pre-GA release.
* 2019-12-05: Notcurses [0.4.0 "TRAP MUSIC ALL NIGHT LONG"](https://github.com/dankamongmen/notcurses/releases/tag/v0.4.0),
    the first generally usable notcurses.
  * I prepare a [demo](https://www.youtube.com/watch?v=eEv2YRyiEVM), and release it on YouTube.
* November 2019: I begin work on [Outcurses](https://github.com/dankamongmen/ncreels).
    Outcurses is a collection of routines atop NCURSES, including ncreels.
    I study the history of NCURSES, primarily using Thomas E. Dickey's FAQ and
    the mailing list archives.
    * 2019-11-14: I file [Outcurses issue #56](https://github.com/dankamongmen/ncreels/issues/56)
      regarding use of TrueColor in outcurses. This is partially inspired by
      Lexi Summer Hale's essay [everything you ever wanted to know about terminals](http://xn--rpa.cc/irl/term.html).
      I get into contact with Thomas E. Dickey and confirm that what I'm hoping
      to do doesn't really fit in with the codified Curses API.
    * 2019-11-16: I make the [first commit](https://github.com/dankamongmen/notcurses/commit/635d7039d79e4f94ba645e8cb601e3a6d82a6b30)
      to Notcurses.
* September 2019: I extracted fade routines from Growlight and Omphalos, and
    offered them to NCURSES as extensions. They are not accepted, which is
    understandable. I mention that I intend to extract ncreels, and offer to
    include them in the CDK (Curses Development Kit). [Growlight issue #43](https://github.com/dankamongmen/growlight/issues/43)
    is created regarding this extraction. A few minor patches go into NCURSES.
* 2011, 2013: I develop [Growlight](https://github.com/dankamongmen/growlight)
    and [Omphalos](https://github.com/dankamongmen/omphalos), complicated TUIs
    making extensive use of NCURSES.

### Thanks

* Notcurses could never be what it is without decades of tireless, likely
    thankless work by Thomas E. Dickey on NCURSES. His FAQ is a model of
    engineering history. He exemplifies documentation excellence and
    conservative, thoughtful stewardship. The free software community owes
    Mr. Dickey a great debt.
* Robert Edmonds provided tremendous assistance Debianizing the package,
    and David Cantrell did likewise for Fedora. Both are hella engineers.
* Justine Tunney, one of my first friends at Google NYC, was always present
    with support, and pointed out the useful memstream functionality of
    POSIX, eliminating the need for me to cons up something similar.
* I one night read the entirety of Lexi Summer Hale's [essays](http://xn--rpa.cc/irl/index.html),
    and woke up intending to write notcurses.
* NES art was lifted from [The Spriters Resource](https://www.spriters-resource.com/nes/)
    and [NES Sprite](http://nes-sprite.resampled.ru/), the kind of sites that
    make the Internet great. It probably violates any number of copyrights. C'est la vie.
* Mark Ferrari, master of the pixel, for no good reason allowed me to reproduce
    his incredible and groundbreaking color-cycling artwork. Thanks Mark!
* The world map image was made by [Vecteezy](https://www.vecteezy.com/free-vector/world-map),
    and is used according to the terms of their License.
* Finally, the [demoscene](https://en.wikipedia.org/wiki/Demoscene) and general
    l33t scene of the 90s and early twenty-first century endlessly inspired a
    young hax0r. There is great joy in computing; no one will drive us from
    this paradise Turing has created!

> “Our fine arts were developed, their types and uses were established, in times
very different from the present, by men whose power of action upon things was
insignificant in comparison with ours. But the amazing growth of our
techniques, the adaptability and precision they have attained, the ideas and
habits they are creating, make it a certainty that _profound changes are
impending in the ancient craft of the Beautiful_.” —Paul Valéry
