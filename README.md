# Notcurses: blingful TUIs and character graphics

* **What it is**: a library facilitating complex TUIs on modern terminal
    emulators, supporting vivid colors, multimedia, threads, and Unicode to the
    maximum degree possible. [Things](https://www.youtube.com/watch?v=cYhZ7myXyyg) can be done with
    Notcurses that simply can't be done with NCURSES. It is furthermore
    fast as shit.

* **What it is not**: a source-compatible X/Open Curses implementation, nor a
    replacement for NCURSES on existing systems.

<p align="center">
<a href="https://www.youtube.com/watch?v=cYhZ7myXyyg"><img src="https://img.youtube.com/vi/cYhZ7myXyyg/0.jpg" alt="setting the standard (hype video)"/></a>
</p>

birthed screaming into this world by [nick black](https://nick-black.com/dankwiki/index.php/Hack_on) (<nickblack@linux.com>)
* c++ wrappers by [marek habersack](http://twistedcode.net/blog/) (<grendel@twistedcode.net>)
* rust wrappers by José Luis Cruz (<joseluis@andamira.net>)
* python wrappers by igo95862 (<igo95862@yandex.ru>)
* [zig wrappers](https://github.com/dundalek/notcurses-zig-example) by [Jakub Dundalek](https://dundalek.com/) (<dundalek@gmail.com>)

for more information, see [dankwiki](https://nick-black.com/dankwiki/index.php/Notcurses)
and the [man pages](https://notcurses.com). in addition, there is
[Doxygen](https://notcurses.com/html/) output. there is a
[mailing list](https://groups.google.com/forum/#!forum/notcurses) which can be reached
via notcurses@googlegroups.com. i wrote a coherent
[guidebook](https://nick-black.com/htp-notcurses.pdf), which is available for
free download (or [paperback purchase](https://amazon.com/dp/B086PNVNC9)).

i've not yet added many documented examples, but [src/poc/](https://github.com/dankamongmen/notcurses/tree/master/src/poc)
and [src/pocpp/](https://github.com/dankamongmen/notcurses/tree/master/src/pocpp)
contain many small C and C++ programs respectively. `notcurses-demo` covers
most of the functionality of Notcurses.

**If you're running Notcurses applications in a Docker, please consult
"[Environment notes](#environment-notes)" below.** If you need Notcurses on
Ubuntu Focal (20.04 LTS), you can run:

```sh
sudo add-apt-repository ppa:dankamongmen/notcurses
sudo apt-get update
```

![Linux](https://img.shields.io/badge/-Linux-grey?logo=linux)
![FreeBSD](https://img.shields.io/badge/-FreeBSD-grey?logo=freebsd)
[![Build](https://drone.dsscaw.com:4443/api/badges/dankamongmen/notcurses/status.svg)](https://drone.dsscaw.com:4443/dankamongmen/notcurses)
[![pypi_version](https://img.shields.io/pypi/v/notcurses?label=pypi)](https://pypi.org/project/notcurses)
[![crates.io](https://img.shields.io/crates/v/libnotcurses-sys.svg)](https://crates.io/crates/libnotcurses-sys)
[![Sponsor](https://img.shields.io/badge/-Sponsor-red?logo=github)](https://github.com/sponsors/dankamongmen)

<a href="https://repology.org/project/notcurses/versions">
<img src="https://repology.org/badge/vertical-allrepos/notcurses.svg" alt="Packaging status" align="right">
</a>

## Introduction

Notcurses abandons the X/Open Curses API bundled as part of the Single UNIX
Specification. For some necessary background, consult Thomas E. Dickey's
superb and authoritative [NCURSES FAQ](https://invisible-island.net/ncurses/ncurses.faq.html#xterm_16MegaColors).
As such, Notcurses is not a drop-in Curses replacement.

Wherever possible, Notcurses makes use of the Terminfo library shipped with
NCURSES, benefiting greatly from its portability and thoroughness.

Notcurses opens up advanced functionality for the interactive user on
workstations, phones, laptops, and tablets, possibly at the expense of e.g.
some industrial and retail terminals. Fundamentally, Curses assumes the minimum
and allows you (with effort) to step up, whereas Notcurses assumes the maximum
and steps down (by itself) when necessary. The latter approach probably breaks
on some older hardware, but the former approach results in new software looking
like old hardware.

Why use this non-standard library?

* Thread safety, and efficient use in parallel programs, has been a design
  consideration from the beginning.

* A more orderly surface than that codified by X/Open: Exported identifiers are
  prefixed to avoid common namespace collisions. The library object exports a
  minimal set of symbols. Where reasonable, `static inline` header-only code is
  used. This facilitates compiler optimizations, and reduces loader time.
  Notcurses can be built without its multimedia functionality, requiring a
  significantly lesser set of dependencies.

* All APIs natively support the Universal Character Set (Unicode). The `nccell`
  API is based around Unicode's [Extended Grapheme Cluster](https://unicode.org/reports/tr29/) concept.

* Visual features including images, fonts, video, high-contrast text, sprites,
  and transparent regions. All APIs natively support 24-bit color, quantized
  down as necessary for the terminal.

* It's Apache2-licensed in its entirety, as opposed to the
  [drama in several acts](https://invisible-island.net/ncurses/ncurses-license.html)
  that is the NCURSES license (the latter is [summarized](https://invisible-island.net/ncurses/ncurses-license.html#issues_freer)
  as "a restatement of MIT-X11").

Much of the above can be had with NCURSES, but they're not what NCURSES was
*designed* for. On the other hand, if you're targeting industrial or critical
applications, or wish to benefit from its time-tested reliability and
portability, you should by all means use that fine library.

## Requirements

Minimum versions generally indicate the oldest version I've tested with; it
may well be possible to use still older versions. Let me know of any successes!

* (build) CMake 3.14.0+ and a C11 compiler
* (OPTIONAL) (OpenImageIO, testing, C++ bindings): A C++17 compiler
* (build+runtime) From [NCURSES](https://invisible-island.net/ncurses/announce.html): terminfo 6.1+
* (build+runtime) GNU [libunistring](https://www.gnu.org/software/libunistring/) 0.9.10+
* (build+runtime) GNU [Readline](https://www.gnu.org/software/readline/) 8.0+
* (OPTIONAL) (build+runtime) From QR-Code-generator: [libqrcodegen](https://github.com/nayuki/QR-Code-generator) 1.5.0+
* (OPTIONAL) (build+runtime) From [FFmpeg](https://www.ffmpeg.org/): libswscale 5.0+, libavformat 57.0+, libavutil 56.0+
* (OPTIONAL) (build+runtime) [OpenImageIO](https://github.com/OpenImageIO/oiio) 2.15.0+, requires C++
* (OPTIONAL) (testing) [Doctest](https://github.com/onqtam/doctest) 2.3.5+
* (OPTIONAL) (documentation) [pandoc](https://pandoc.org/index.html) 1.19.2+
* (OPTIONAL) (python bindings): Python 3.7+, [CFFI](https://pypi.org/project/cffi/) 1.13.2+, [pypandoc](https://pypi.org/project/pypandoc/) 1.5+
* (OPTIONAL) (rust bindings): rust 1.47.0+, [bindgen](https://crates.io/crates/bindgen) 0.55.1+, pkg-config 0.3.18+, cty 0.2.1+
* (runtime) Linux 5.3+, FreeBSD 11+, or DragonFly BSD 5.9+

[Here's more information](INSTALL.md) on building and installation.

## Included tools

Seven binaries are installed as part of Notcurses:
* `ncls`: an `ls` that displays multimedia in the terminal
* `ncneofetch`: a [neofetch](https://github.com/dylanaraps/neofetch) ripoff
* `ncplayer`: renders visual media (images/videos)
* `nctetris`: a tetris clone
* `notcurses-demo`: some demonstration code
* `notcurses-input`: decode and print keypresses
* `notcurses-tester`: unit testing

To run `notcurses-demo` from a checkout, provide the `data` directory via
the `-p` argument. Demos requiring data files will otherwise abort. The base
delay used in `notcurses-demo` can be changed with `-d`, accepting a
floating-point multiplier. Values less than 1 will speed up the demo, while
values greater than 1 will slow it down.

`notcurses-tester` likewise requires that `data`, populated with the necessary
data files, be specified with `-p`. It can be run by itself, or via `make test`.

## Documentation

With `-DUSE_PANDOC=on` (the default), a full set of man pages and XHTML
will be built from `doc/man`. The following Markdown documentation is included
directly:

* Per-release [News](NEWS.md) for packagers, developers, and users.
* The `TERM` environment variable and [various terminal emulators](TERMINALS.md).
* Notes on [contributing](doc/CONTRIBUTING.md) and [hacking](doc/HACKING.md).
* There's a semi-complete [reference guide](USAGE.md).
* A list of [other TUI libraries](doc/OTHERS.md).
* Abbreviated [history](doc/HISTORY.md) and thanks.
* [Differences from](doc/CURSES.md) Curses and adapting Curses programs.

## Environment notes

* If your `TERM` variable is wrong, or that terminfo definition is out-of-date,
  you're going to have a very bad time. Use *only* `TERM` values appropriate
  for your terminal. If this variable is undefined, or Notcurses can't load the
  specified Terminfo entry, it will refuse to start, and you will
  [not be going to space today](https://xkcd.com/1133/).

* Ensure your `LANG` environment variable is set to a UTF8-encoded locale, and
  that this locale has been generated. This usually means
  `"[language]_[Countrycode].UTF-8"`, i.e. `en_US.UTF-8`. The first part
  (`en_US`) ought exist as a directory or symlink in `/usr/share/locales`.
  This usually requires editing `/etc/locale.gen` and running `locale-gen`.
  On Debian systems, this can be accomplished with `dpkg-reconfigure locales`,
  and enabling the desired locale. The default locale is stored somewhere like
  `/etc/default/locale`.

* If your terminal has an option about default interpretation of "ambiguous-width
  characters" (this is actually a technical term from Unicode), ensure it is
  set to **Wide**, not narrow (if that doesn't work, ensure it is set to
  **Narrow**, heh).

### TrueColor detection

Notcurses primarily loads control sequences from `terminfo(5)`, using the
database entry specified by the `TERM` environment variable. 24-bit "TrueColor"
color support (or at least the ability to specify 3 8-bit channels as arguments
to `setaf` and `setbf`) is indicated by the `rgb` terminfo capability. Many
terminals with RGB support do not advertise the `rgb` capability. If you
believe your terminal to support 24-bit TrueColor, this can be indicated by
exporting the `COLORTERM` environment variable as `truecolor` or `24bit`.
Note that some terminals accept a 24-bit specification, but map it down to
fewer colors.

### Fonts

Fonts end up being a whole thing, little of which is pleasant. I'll write this
up someday **FIXME**.

It is worth knowing that several terminals draw the block characters directly,
rather than loading them from a font.

### FAQs

If things break or seem otherwise lackluster, **please** consult the
[Environment Notes](#environment_notes) section! You **need** to have a correct
`TERM` and `LANG` definition, and probably want `COLORTERM`.

* **Q:** The demo fails in the middle of `intro`. **A:** Check that your `TERM`
value is correct for your terminal. `intro` does a palette fade, which is prone
to breaking under incorrect `TERM` values. If you're not using `xterm`, your
`TERM` should not be `xterm`!

* **Q:** Can I have Notcurses without this huge multimedia stack? **A:** Yes! Build with `-DUSE_MULTIMEDIA=none`.

* **Q:** Notcurses looks like absolute crap in `screen`. **A:** `screen` doesn't support RGB colors (at least as of 4.08.00); if you have `COLORTERM` defined, you'll have a bad time. If you have a `screen` that was compiled with `--enable-colors256`, try exporting `TERM=screen-256color` as opposed to `TERM=screen`.

* **Q:** Notcurses looks like absolute crap in `mosh`. **A**: Yeah it sure does. I'm not yet sure what's up.

* **Q:** Why didn't you just use Sixel? **A:** Many terminal emulators don't support Sixel. Sixel doesn't work well with mouse selection. Sixel tends to be horribly inefficient, and has a limited color palette. With that said, both Sixel and the Kitty bitmap protocol are well-supported.

* **Q:** I'm not seeing `NCKEY_RESIZE` until I press some other key. **A:** You've almost certainly failed to mask `SIGWINCH` in some thread, and that thread is receiving the signal instead of the thread which called `notcurses_getc_blocking()`. As a result, the `poll()` is not interrupted. Call `pthread_sigmask()` before spawning any threads.

* **Q:** Using the C++ wrapper, how can I ensure that the `NotCurses` destructor is run when I return from `main()`? **A:** As noted in the [C++ FAQ](https://isocpp.org/wiki/faq/dtors#artificial-block-to-control-lifetimes), wrap it in an artificial scope (this assumes your `NotCurses` is scoped to `main()`).

* **Q:** How do I hide a plane I want to make visible later? **A:** In order of least to most performant: move it offscreen using `ncplane_move_yx()`, move it underneath an opaque plane with `ncplane_move_below()`, or move it off-pile with `ncplane_reparent()`.

* **Q:** Why isn't there an `ncplane_box_yx()`? Do you hate orthogonality, you dullard? **A:** `ncplane_box()` and friends already have far too many arguments, you monster.

* **Q:** Why doesn't Notcurses support 10- or 16-bit color? **A:** Notcurses supports 24 bits of color, spread across three eight-bit channels. You presumably mean 10-bit-per-channel color. Notcurses will support it when a terminal supports it.

* **Q:** The name is dumb. **A:** That's not a question?

* **Q:** I'm not finding qrcodegen on BSD, despite having installed `graphics/qr-code-generator`. **A:** Try `cmake -DCMAKE_REQUIRED_INCLUDES=/usr/local/include`. This is passed by `bsd.port.mk`.

* **Q:** Do you support [musl](https://musl.libc.org/)? **A:** I try to! You'll need at least 1.20.

* **Q:** I only seem to blit in ASCII, and/or can't emit Unicode beyond ASCII in general. **A:** Your `LANG` environment variable is underdefined or incorrectly defined, or the necessary locale is not present on your machine (it is also possible that you explicitly supplied `NCOPTION_INHIBIT_SETLOCALE`, but never called `setlocale(3)`, in which case don't do that).

* **Q:** I pretty much always need an `ncplane` when using a `nccell`. Why doesn't the latter hold a pointer to the former? **A:** Besides the massive redundancy this would entail, `nccell` needs to remain as small as possible, and you almost always have the `ncplane` handy if you've got a reference to a valid `nccell` anyway.

* **Q:** I ran `notcurses-demo`, but my table numbers don't match the Notcurses banner numbers, you charlatan. **A:** `notcurses-demo` renders several frames beyond the actual demos.

* **Q:** When my program exits, I don't have a cursor, or text is invisible, or colors are weird, <i>ad nauseam</i>. **A:** Ensure you're calling `notcurses_stop()`/`ncdirect_stop()` on all exit paths, including fatal signals.

* **Q:** How can I use Direct Mode in conjunction with libreadline? **A:** Pass `NCDIRECT_OPTION_CBREAK` to `ncdirect_init()`, and ensure you do not pass `NCDIRECT_OPTION_NO_READLINE`. If you'd like, set `rl_readline_name` and `rl_attempted_completion_function` prior to calling `ncdirect_init()`. With that said, consider using a Notcurses `ncreader`.

* **Q:** Will there ever be Java wrappers? **A:** I should hope not. If you want a Java solution, try Autumn Lamonte's [Jexer](https://jexer.sourceforge.io/).

* **Q:** Given that the glyph channel is initialized as transparent for a plane, shouldn't the foreground and background be initialized as transparent, also? **A:** Probably (they are instead initialized to default opaque). This would change some of the most longstanding behavior of Notcurses, though, so it isn't happening.

* **Q:** Why does my right-to-left text appear left-to-right? **A:** Notcurses doesn't honor the BiDi state machine, and in fact forces left-to-right with BiDi codes. Likewise, ultra-wide glyphs will have interesting effects. ﷽!

* **Q:** I get linker errors when statically linking. **A:** Are you linking all necessary libraries? Use `pkg-config --static --libs notcurses` to discover them.

* **Q:** Can I avoid manually exporting `COLORTERM=24bit` everywhere? **A:** Sure. Add `SendEnv COLORTERM` to `.ssh/config`, and `AcceptEnv COLORTERM` to `sshd_config` on the remote server. Yes, this will probably require root on the remote server. Don't blame me, man; I didn't do it.

* **Q:** How about *arbitrary image manipulation here* functionality? **A:** I'm not going to beat ImageMagick et al. on image manipulation, but you can load an `ncvisual` from RGBA memory using `ncvisual_from_rgba()`.

## Useful links

* [BiDi in Terminal Emulators](https://terminal-wg.pages.freedesktop.org/bidi/)
* [The Xterm FAQ](https://invisible-island.net/xterm/xterm.faq.html)
  * [XTerm Control Sequences](https://invisible-island.net/xterm/ctlseqs/ctlseqs.pdf)
* [The NCURSES FAQ](https://invisible-island.net/ncurses/ncurses.faq.html)
* [ECMA-35 Character Code Structure and Extension Techniques](https://www.ecma-international.org/publications/standards/Ecma-035.htm) (ISO/IEC 2022)
* [ECMA-43 8-bit Coded Character Set Structure and Rules](https://www.ecma-international.org/publications/standards/Ecma-043.htm)
* [ECMA-48 Control Functions for Coded Character Sets](https://www.ecma-international.org/publications/standards/Ecma-048.htm) (ISO/IEC 6429)
* [Unicode 13.1 Full Emoji List](https://unicode.org/emoji/charts/full-emoji-list.html)
* [Unicode Standard Annex #29 Text Segmentation](http://www.unicode.org/reports/tr29)
* [Unicode Standard Annex #15 Normalization Forms](https://unicode.org/reports/tr15/)
* [The TTY demystified](http://www.linusakesson.net/programming/tty/)
* [Dark Corners of Unicode](https://eev.ee/blog/2015/09/12/dark-corners-of-unicode/)
* [UTF-8 Decoder Capability and Stress Test](https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt)
* [Emoji: how do you get from U+1F355 to 🍕?](https://meowni.ca/posts/emoji-emoji-emoji/)
* [Glyph Hell: An introduction to glyphs, as used and defined in the FreeType engine](http://chanae.walon.org/pub/ttf/ttf_glyphs.htm)
* My wiki's [Sixel page](https://nick-black.com/dankwiki/index.php?title=Sixel) and Kitty's [extensions](https://sw.kovidgoyal.net/kitty/protocol-extensions.html).

### Useful man pages
* Linux: [console_codes(4)](http://man7.org/linux/man-pages/man4/console_codes.4.html)
* Linux: [termios(3)](http://man7.org/linux/man-pages/man3/termios.3.html)
* Linux: [ioctl_tty(2)](http://man7.org/linux/man-pages/man2/ioctl_tty.2.html)
* Linux: [ioctl_console(2)](http://man7.org/linux/man-pages/man2/ioctl_console.2.html)
* Portable: [terminfo(5)](http://man7.org/linux/man-pages/man5/terminfo.5.html)
* Portable: [user_caps(5)](http://man7.org/linux/man-pages/man5/user_caps.5.html)

### Grotesque vanity and meaningless metrics

[![stargazers over time](https://starcharts.herokuapp.com/dankamongmen/notcurses.svg)](https://starcharts.herokuapp.com/dankamongmen/notcurses)

> “Our fine arts were developed, their types and uses were established, in times
very different from the present, by men whose power of action upon things was
insignificant in comparison with ours. But the amazing growth of our
techniques, the adaptability and precision they have attained, the ideas and
habits they are creating, make it a certainty that _profound changes are
impending in the ancient craft of the Beautiful_.” —Paul Valéry
