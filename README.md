# Notcurses: blingful TUIs and character graphics

**What it is**: a library facilitating complex TUIs on modern terminal
emulators, supporting vivid colors, multimedia, threads, and Unicode to the
maximum degree possible. [Things](https://www.youtube.com/watch?v=dcjkezf1ARY) can be done with
Notcurses that simply can't be done with NCURSES. It is furthermore
fast as shit. **What it is not**: a source-compatible X/Open Curses implementation, nor a
replacement for NCURSES on existing systems.

<p align="center">
<a href="https://www.youtube.com/watch?v=dcjkezf1ARY"><img src="https://raw.githubusercontent.com/dankamongmen/notcurses/gh-pages/notcurses-logo.png" alt="setting the standard (hype video)"/></a>
</p>

for more information, see [dankwiki](https://nick-black.com/dankwiki/index.php/Notcurses)
and the [man pages](https://notcurses.com). in addition, there is
[Doxygen](https://notcurses.com/html/) output. To subscribe to the
[mailing list](https://groups.google.com/forum/#!forum/notcurses), send an email
to notcurses+subscribe@googlegroups.com (the email contents don't matter). i wrote a coherent
[guidebook](https://nick-black.com/htp-notcurses.pdf), which is available for
free download (or [paperback purchase](https://amazon.com/dp/B086PNVNC9)).

i've not yet added many documented examples, but [src/poc/](https://github.com/dankamongmen/notcurses/tree/master/src/poc)
and [src/pocpp/](https://github.com/dankamongmen/notcurses/tree/master/src/pocpp)
contain many small C and C++ programs respectively. `notcurses-demo` covers
most of the functionality of Notcurses.

**If you're running Notcurses applications in a Docker, please consult
"[Environment notes](#environment-notes)" below.**

<a href="https://repology.org/project/notcurses/versions">
<img src="https://repology.org/badge/vertical-allrepos/notcurses.svg" alt="Packaging status" align="right">
</a>

![Linux](https://img.shields.io/badge/-Linux-grey?logo=linux)
![FreeBSD](https://img.shields.io/badge/-FreeBSD-grey?logo=freebsd)
![Windows](https://img.shields.io/badge/-Windows-grey?logo=windows)
![macOS](https://img.shields.io/badge/-macOS-grey?logo=macos)

[![Linux](https://github.com/dankamongmen/notcurses/actions/workflows/ubuntu_test.yml/badge.svg?branch=master)](https://github.com/dankamongmen/notcurses/actions/workflows/ubuntu_test.yml?query=branch%3Amaster)
[![macOS](https://github.com/dankamongmen/notcurses/actions/workflows/macos_test.yml/badge.svg?branch=master)](https://github.com/dankamongmen/notcurses/actions/workflows/macos_test.yml?query=branch%3Amaster)
[![Windows](https://github.com/dankamongmen/notcurses/actions/workflows/windows_test.yml/badge.svg?branch=master)](https://github.com/dankamongmen/notcurses/actions/workflows/windows_test.yml?query=branch%3Amaster)

[![pypi_version](https://img.shields.io/pypi/v/notcurses?label=pypi)](https://pypi.org/project/notcurses)
[![crates.io](https://img.shields.io/crates/v/libnotcurses-sys.svg)](https://crates.io/crates/libnotcurses-sys)

[![Matrix](https://img.shields.io/matrix/notcursesdev:matrix.org?label=matrixchat)](https://app.element.io/#/room/#notcursesdev:matrix.org)
[![Sponsor](https://img.shields.io/badge/-Sponsor-red?logo=github)](https://github.com/sponsors/dankamongmen)

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
  prefixed to avoid common namespace collisions. Where reasonable,
  `static inline` header-only code is used. This facilitates compiler
  optimizations, and reduces loader time. Notcurses can be built without its
  multimedia functionality, requiring a significantly lesser set of dependencies.

* All APIs natively support the Universal Character Set (Unicode). The `nccell`
  API is based around Unicode's [Extended Grapheme Cluster](https://unicode.org/reports/tr29/) concept.

* Visual features including images, fonts, video, high-contrast text, sprites,
  and transparent regions. All APIs natively support 24-bit color, quantized
  down as necessary for the terminal.

* Portable support for bitmapped graphics, using Sixel, Kitty,
  and even the Linux framebuffer console.

* Support for unambiguous [keyboard protocols](https://sw.kovidgoyal.net/kitty/keyboard-protocol/).

* "TUI mode" facilitates high-performance, non-scrolling, full-screen
  applications. "CLI mode" supports scrolling output for shell utilities,
  but with the full power of Notcurses.

* It's Apache2-licensed in its entirety, as opposed to the
  [drama in several acts](https://invisible-island.net/ncurses/ncurses-license.html)
  that is the NCURSES license (the latter is [summarized](https://invisible-island.net/ncurses/ncurses-license.html#issues_freer)
  as "a restatement of MIT-X11").

Much of the above can be had with NCURSES, but they're not what NCURSES was
*designed* for. On the other hand, if you're targeting industrial or critical
applications, or wish to benefit from time-tested reliability and
portability, you should by all means use that fine library.

## Requirements

Minimum versions generally indicate the oldest version I've tested with; it
may well be possible to use still older versions. Let me know of any successes!

* (build) CMake 3.14.0+ and a C11 compiler
* (OPTIONAL) (OpenImageIO, testing, C++ bindings): A C++17 compiler
* (build+runtime) From [NCURSES](https://invisible-island.net/ncurses/announce.html): terminfo 6.1+
* (build+runtime) GNU [libunistring](https://www.gnu.org/software/libunistring/) 0.9.10+
* (OPTIONAL) (build+runtime) [libgpm](https://www.nico.schottelius.org/software/gpm/) 1.20+
* (OPTIONAL) (build+runtime) From QR-Code-generator: [libqrcodegen](https://github.com/nayuki/QR-Code-generator) 1.5.0+
* (OPTIONAL) (build+runtime) From [FFmpeg](https://www.ffmpeg.org/): libswscale 5.0+, libavformat 57.0+, libavutil 56.0+, libavdevice 57.0+
* (OPTIONAL) (build+runtime) [OpenImageIO](https://github.com/OpenImageIO/oiio) 2.15.0+, requires C++
* (OPTIONAL) (testing) [Doctest](https://github.com/onqtam/doctest) 2.3.5+
* (OPTIONAL) (documentation) [pandoc](https://pandoc.org/index.html) 1.19.2+
* (OPTIONAL) (python bindings): Python 3.7+, [CFFI](https://pypi.org/project/cffi/) 1.13.2+, [pypandoc](https://pypi.org/project/pypandoc/) 1.5+
* (runtime) Linux 2.6+, FreeBSD 11+, DragonFly BSD 5.9+, Windows 10 v1093+, or macOS 11.4+

More information on building and installation is available in [INSTALL.md](INSTALL.md).

### Wrappers

If you wish to use a language other than C to work with Notcurses, numerous
wrappers are available. Several are included in this repository, while
others are external.

| Language | Lead(s)                       | Repository |
| -------- | ----------------------------- | ---------- |
| Ada      | Jeremy Grosser                | [JeremyGrosser/notcursesada](https://github.com/JeremyGrosser/notcursesada) |
| C++      | Marek Habersack, nick black   | internal   |
| Dart     | Nelson Fernandez              | [kascote/dart_notcurses](https://github.com/kascote/dart_notcurses) |
| Julia    | Dheepak Krishnamurthy         | [kdheepak/Notcurses.jl](https://github.com/kdheepak/Notcurses.jl) |
| Nim      | Michael S. Bradley, Jr.       | [michaelsbradleyjr/nim-notcurses](https://github.com/michaelsbradleyjr/nim-notcurses) |
| Python   | nick black                    | internal   |
| Python   | igo95862                      | internal   |
| Rust     | José Luis Cruz                | [dankamongmen/libnotcurses-sys](https://github.com/dankamongmen/libnotcurses-sys) |
| Zig      | Jakub Dundalek                | [dundalek/notcurses-zig-example](https://github.com/dundalek/notcurses-zig-example) |

## Included tools

Nine executables are installed as part of Notcurses:
* `ncls`: an `ls` that displays multimedia in the terminal
* `ncneofetch`: a [neofetch](https://github.com/dylanaraps/neofetch) ripoff
* `ncplayer`: renders visual media (images/videos)
* `nctetris`: a tetris clone
* `notcurses-demo`: some demonstration code
* `notcurses-info`: detect and print terminal capabilities/diagnostics
* `notcurses-input`: decode and print keypresses
* `notcurses-tester`: unit testing
* `tfman`: a swank manual browser

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

If you (understandably) want to avoid the large Pandoc stack, but still enjoy
manual pages, I publish a tarball with generated man/XHTML along with
each release. Download it, and install the contents as you deem fit.

## Environment notes

* If your `TERM` variable is wrong, or that terminfo definition is out-of-date,
  you're going to have a very bad time. Use *only* `TERM` values appropriate
  for your terminal. If this variable is undefined, or Notcurses can't load the
  specified Terminfo entry, it will refuse to start, and you will
  [not be going to space today](https://xkcd.com/1133/).

* Notcurses queries the terminal on startup, enabling some advanced features
  based on the determined terminal (and even version). Basic capabilities,
  however, are taken from Terminfo. So if you have, say, Kitty, but
  `TERM=vt100`, you're going to be able to draw RGBA bitmap graphics (despite
  such things being but a dream for a VT100), but *unable* to use the alternate
  screen (despite it being supported by every Kitty version). So `TERM` and an
  up-to-date Terminfo database remain important.

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

* If your terminal supports 3x8bit RGB color via `setaf` and `setbf` (most
  modern terminals), but exports neither the `RGB` nor `Tc` terminfo capability,
  you can export the `COLORTERM` environment variable as `truecolor` or `24bit`.
  Note that some terminals accept a 24-bit specification, but map it down to
  fewer colors. RGB is unconditionally enabled whenever
  [most modern terminals](TERMINALS.md) are identified.

### Fonts

Glyph width, and indeed whether a glyph can be displayed at all, is dependent
in part on the font configuration. Ideally, your font configuration has a
glyph for every Unicode EGC, and each glyph's width matches up with the POSIX
function's `wcswidth()` result for the EGC. If this is not the case, you'll
likely get blanks or � (U+FFFD, REPLACEMENT CHARACTER) for missing characters,
and subsequent characters on the line may be misplaced.

It is worth knowing that several terminals draw the block characters directly,
rather than loading them from a font. This is generally desirable. Quadrants
and sextants are not the place to demonstrate your design virtuosity. To
inspect your environment's rendering of drawing characters, run
`notcurses-info`. The desired output ought look something like this:

<p align="center">
<img src="https://raw.githubusercontent.com/dankamongmen/notcurses/gh-pages/notcurses-info.png" alt="notcurses-info can be used to check Unicode drawing"/>
</p>

## FAQs

If things break or seem otherwise lackluster, **please** consult the
[Environment Notes](#environment-notes) section! You **need** correct
`TERM` and `LANG` definitions, and might want `COLORTERM`.

<details>
 <summary>Can I use Notcurses in my closed-source program?</summary>
 Notcurses is licensed under <a href="https://www.apache.org/licenses/LICENSE-2.0">Apache2</a>,
 a demonstration that I have transcended your petty world of material goods,
 fiat currencies, and closed sources. Implement Microsoft Bob in it. Charge
 rubes for it. Put it in your ballistic missiles so that you have a nice LED
 display of said missile's speed and projected yield; right before impact,
 scroll "FUCK YOU" in all the world's languages, and close it out with a smart
 palette fade. Carve the compiled objects onto bricks and mail them to Richard
 Stallman, taunting him through a bullhorn as you do so.
</details>

<details>
  <summary>Can I write a CLI program (scrolling, fits in with the shell, etc.)
   with Notcurses?</summary>
   Yes! Use the <code>NCOPTION_CLI_MODE</code> flag (an alias for several
   real flags; see <a href="https://notcurses.com/notcurses_init.3.html"><code>notcurses_init(1)</code></a>
   for more information). You still must explicitly render.
</details>

<details>
  <summary>Can I have Notcurses without this huge multimedia stack?</summary>
  Again yes! Build with <code>-DUSE_MULTIMEDIA=none</code>.
</details>

<details>
  <summary>Can I build this individual Notcurses program without aforementioned
  multimedia stack?</summary>
  Almost unbelievably, yes! Use <code>notcurses_core_init()</code> or
  <code>ncdirect_core_init()</code> in place of <code>notcurses_init()</code>/
  <code>ncdirect_init()</code>, and link with <code>-lnotcurses-core</code>.
  Your application will likely start a few milliseconds faster;
  more importantly, it will link against minimal Notcurses installations.
</details>

<details>
  <summary>We're paying by the electron, and have no C++ compiler. Can we still
  enjoy Notcurses goodness?</summary>
  Some of it! You won't be able to build several executables, nor the NCPP C++
  wrappers, nor can you build with the OpenImageIO multimedia backend (OIIO
  ships C++ headers). You'll be able to build the main library, though, as
  well as <code>notcurses-demo</code> (and maybe a few other programs).
  Use <code>-DUSE_CXX=off</code>.
</details>

<details>
  <summary>Do I want ffmpeg or OpenImageIO?</summary>
  While OpenImageIO is a superb library for dealing with single-frame images,
  its video support is less than perfect (blame me; I've been promising Larry
  I'd rewrite it for several months), and in any case implemented
  atop...ffmpeg. ffmpeg is the preferred multimedia backend.
</details>

<details>
  <summary>Does it work with hardware terminals?</summary>
  With the correct <code>TERM</code> value, many hardware terminals are
  supported. In general, if the terminfo database entry indicates mandatory
  delays, Notcurses will not currently support that terminal properly. It's
  known that Notcurses can drive the VT320 and VT340, including Sixel graphics
  on the latter.
</details>

<details>
  <summary>What happens if I try blitting bitmap graphics on a terminal which
  doesn't support them?</summary>
  Notcurses will not make use of bitmap protocols unless the terminal positively
  indicates support for them, even if <code>NCBLIT_PIXEL</code> has been
  requested. Likewise, sextants (<code>NCBLIT_3x2</code>) won't be used without
  Unicode 13 support, etc. <code>ncvisual_blit()</code> will use the best blitter
  available, unless <code>NCVISUAL_OPTION_NODEGRADE</code> is provided (in
  which case it will fail).
</details>

<details>
  <summary>Notcurses looks like absolute crap in <code>screen</code>.</summary>
  <code>screen</code> doesn't support RGB colors (at least as of 4.08.00);
  if you have <code>COLORTERM</code> defined, you'll have a bad time.
    If you have a <code>screen</code> that was compiled with
    <code>--enable-colors256</code>, try exporting
    <code>TERM=screen-256color</code> as opposed to <code>TERM=screen</code>.
</details>

<details>
  <summary>Notcurses looks like absolute crap in <code>mosh</code>.</summary>
  Yeah it sure does. I'm not yet sure what's up.
</details>

<details>
  <summary>Notcurses looks like absolute crap in Windows Terminal.</summary>
  Go to <a href="ms-settings:regionlanguage">Language Setting</a>, click
  "Administrative language settings", click "Change system locale", and check
  the "Beta: Use Unicode UTF-8 for worldwide language support" option. Restart
  the computer. That ought help a little bit. Try playing with fonts—Cascadia
  Code and Cascadia Mono both seem to work well (quadrants and Braille both
  work), whereas Consolas and Courier New both have definite problems.
</details>

<details>
  <summary>I'm getting strange and/or duplicate inputs in Kitty/foot.</summary>
  Notcurses supports Kitty's powerful
  <a href="https://sw.kovidgoyal.net/kitty/keyboard-protocol/">keyboard protocol</a>,
  which includes things like key release events and modifier keypresses by
  themselves. This means, among other things, that a program in these terminals
  will usually immediately get an <code>NC_ENTER</code> <code>NCTYPE_RELEASE</code>
  event, and each keypress will typically result in at least two inputs.
</details>

<details>
  <summary>Why didn't you just render everything to bitmaps?</summary>
  That's not a TUI; it's a slow and inflexible GUI. Many terminal emulators
  don't support bitmaps. They doesn't work well with mouse selection.
  Sixels have a limited color palette. With that said, both Sixel and the
  Kitty bitmap protocol are well-supported.
</details>

<details>
  <summary>My multithreaded program doesn't see <code>NCKEY_RESIZE</code> until
  I press some other key.</summary>
  You've almost certainly failed to mask <code>SIGWINCH</code> in some thread,
  and that thread is receiving the signal instead of the thread which called
  <code>notcurses_getc_blocking()</code>. As a result, the <code>poll()</code>
  is not interrupted. Call <code>pthread_sigmask()</code> before spawning any
  threads.
</details>

<details>
  <summary>Using the C++ wrapper, how can I ensure that the <code>NotCurses</code>
  destructor is run when I return from <code>main()</code>?</summary>
  As noted in the
  <a href="https://isocpp.org/wiki/faq/dtors#artificial-block-to-control-lifetimes">
  C++ FAQ</a>, wrap it in an artificial scope (this assumes your
  <code>NotCurses</code> is scoped to <code>main()</code>).
</details>

<details>
  <summary>How do I hide a plane I want to make visible later?</summary>
  In order of least to most performant: move it offscreen using
  <code>ncplane_move_yx()</code>, move it underneath an opaque plane with
  <code>ncplane_move_below()</code>, or move it off-pile with
  <code>ncplane_reparent()</code>.
</details>

<details>
  <summary>Why isn't there an <code>ncplane_box_yx()</code>? Do you hate
  orthogonality, you dullard?</summary> <code>ncplane_box()</code> and friends
  already have far too many arguments, you monster.
</details>

<details>
  <summary>Why doesn't Notcurses support 10- or 16-bit color?</summary>
  Notcurses supports 24 bits of color, spread across three eight-bit channels.
  You presumably mean 10-bit-per-channel color. I needed those six bits for
  other things. When terminals support it, Notcurses might support it.
</details>

<details>
  <summary>The name is dumb.</summary>
  That's not a question?
</details>

<details>
  <summary>I'm not finding qrcodegen on BSD, despite having installed
  <code>graphics/qr-code-generator</code>.</summary>
  Try <code>cmake -DCMAKE_REQUIRED_INCLUDES=/usr/local/include</code>.
  This is passed by <code>bsd.port.mk</code>.
</details>

<details>
  <summary>Do you support <a href="https://musl.libc.org/">musl</a>?</summary>
  I try to! You'll need at least 1.20.
</details>

<details>
  <summary>I only seem to blit in ASCII, and/or can't emit Unicode beyond ASCII
  in general.</summary>
  Your <code>LANG</code> environment variable is underdefined or incorrectly
  defined, or the necessary locale is not present on your machine (it is also
  possible that you explicitly supplied <code>NCOPTION_INHIBIT_SETLOCALE</code>,
  but never called <code>setlocale(3)</code>, in which case don't do that).
</details>

<details>
  <summary>I pretty much always need an <code>ncplane</code> when using a
  <code>nccell</code>. Why doesn't the latter hold a pointer to the former?
  </summary>
  Besides the massive redundancy this would entail, <code>nccell</code> needs to
  remain as small as possible, and you almost always have the <code>ncplane</code>
  handy if you've got a reference to a valid <code>nccell</code> anyway.
</details>

<details>
 <summary>I ran my Notcurses program under <code>valgrind</code>/ASAN, and
    it shows memory leaks from <code>libtinfo.so</code>, what's up with that?</summary>
  Yeah, the NCURSES Terminfo leaks memory unless compiled a special,
  non-standard way (see the NCURSES FAQ). It shouldn't be a substantial amount;
  you're advised not to worry overmuch about it.
</details>

<details>
  <summary>I ran <code>notcurses-demo</code>, but my table numbers don't match
  the Notcurses banner numbers, you charlatan.</summary>
  <code>notcurses-demo</code> renders several frames beyond the actual demos.
</details>

<details>
  <summary>When my program exits, I don't have a cursor, or text is invisible,
  or colors are weird, <i>ad nauseam</i>.</summary>
  Ensure you're calling <code>notcurses_stop()</code>/<code>ncdirect_stop()</code>
  on all exit paths, including fatal signals (note that, by default, Notcurses
  installs handlers for most fatal signals to do exactly this).
</details>

<details>
  <summary>How can I use Direct Mode in conjunction with libreadline?</summary>
  You can't anymore (you could up until 2.4.1, but the new input system is
  fundamentally incompatible with it). <code>ncdirect_readline()</code> still exists,
  though, and now actually works even without libreadline, though it is of
  course not exactly libreadline. In any case, you'd probably be better off
  using CLI mode with a <code>ncreader</code>.
</details>

<details>
  <summary>So is Direct Mode deprecated or what?</summary>
  It is not currently deprecated, and definitely receives bugfixes. You are
  probably better served using CLI mode (see above), which came about
  somewhat late in Notcurses development (the 2.3.x series), but is superior
  to Direct Mode in pretty much every way. The only reason to use Direct
  Mode is if you're going to have other programs junking up your display.
</details>

<details>
  <summary>Direct Mode sounds fast! Since it's, like, direct.</summary>
  Direct mode is <i>substantially slower</i> than rendered mode. Rendered
  mode assumes it knows what's on the screen, and uses this information to
  generate optimized sequences of escapes and glyphs. Direct mode writes
  everything it's told to write. It is furthermore far less capable—all
  widgets etc. are available only to rendered mode, and will definitely
  not be extended to Direct Mode.
</details>

<details>
  <summary>Will there ever be Java wrappers?</summary>
  I should hope not. If you want a Java solution, try @klamonte's
  <a href="https://jexer.sourceforge.io/">Jexer</a>. Autumn's a good
  woman, and thorough. We seem to have neatly partitioned the language
  space.
</details>

<details>
  <summary>Given that the glyph channel is initialized as transparent for a
  plane, shouldn't the foreground and background be initialized as transparent,
  also?</summary>
  Probably (they are instead by default initialized to opaque). This would change
  some of the most longstanding behavior of Notcurses, though,
  so it isn't happening.
</details>

<details>
  <summary>I get linker errors when statically linking.</summary>
  Are you linking all necessary libraries? Use
  <code>pkg-config --static --libs notcurses</code>
  (or <code>--libs notcurses-core</code>) to discover them.
</details>

<details>
  <summary>Notcurses exits immediately in MSYS2/Cygwin.</summary>
  Notcurses requires the
  <a href="https://devblogs.microsoft.com/commandline/windows-command-line-introducing-the-windows-pseudo-console-conpty/">Windows ConPTY</a>
  layer. This is available in Cygwin by default since 3.2.0, but is disabled
  by default in MSYS. Launch <code>mintty</code> with <code>-P on</code>
  arguments, or export <code>MSYS=enable_pcon</code> before launching it.
</details>

<details>
  <summary>Can I avoid manually exporting <code>COLORTERM=24bit</code>
  everywhere?</summary>
  Sure. Add <code>SendEnv COLORTERM</code> to <code>.ssh/config</code>, and
  <code>AcceptEnv COLORTERM</code> to <code>sshd_config</code> on the remote
  server. Yes, this will probably require root on the remote server.
  Don't blame me, man; I didn't do it.
</details>

<details>
  <summary>How about <i>arbitrary image manipulation here</i> functionality?</summary>
  I'm not going to beat ImageMagick et al. on image manipulation, but you can
  load an <code>ncvisual</code> from RGBA memory using
  <code>ncvisual_from_rgba()</code>.
</details>

<details>
  <summary>My program locks up during initialization. </summary>
  Notcurses interrogates the terminal. If the terminal doesn't reply to standard
  interrogations, file a Notcurses bug, send upstream a patch, or use a different
  terminal. No known terminal emulators exhibit this behavior.
</details>

<details>
  <summary>How can I draw a large plane, and only make a portion of it visible?</summary>
  The simplest way is probably to create a plane of the same dimensions immediately above
  the plane, and keep a region of it transparent, and the rest opaque. If you want the visible
  area to stay in the same place on the display, but the portion being seen to change, try
  making a plane twice as large in each dimension as the original plane. Make the desired area
  transparent, and the rest opaque. Now move the original plane behind this plane so that the
  desired area lines up with the &ldquo;hole&rdquo;.
</details>

<details>
  <summary>Why no <code>NCSTYLE_REVERSE</code>?</summary>
  It would consume a precious bit. You can use <code>ncchannels_reverse()</code>
  to correctly invert fore- and background colors.
</details>

<details>
  <summary>How do I mix Rendered and Direct mode?</summary>
  You really don't want to. You can stream a subprocess to a plane with the
  <code>ncsubproc</code> widget.
</details>

<details>
  <summary>How can I clear the screen on startup in Rendered mode when not using
  the alternate screen?</summary>
  Call <code>notcurses_refresh()</code> after <code>notcurses_init()</code>
  returns successfully.
</details>

<details>
  <summary>Why do the stats show more Linux framebuffer bitmap bytes written
  than total bytes written to the terminal? And why don't Linux console
  graphics work when I ssh?</summary>
  Linux framebuffer graphics aren't implemented via terminal writes, but rather
  writes directly into a memory map. This memory map isn't available on remote
  machines, and these writes aren't tracked by the standard statistics.
</details>

<details>
 <summary>What is the possessive form of Notcurses?</summary>
 <b>Notcurses'.</b> I cite <a href="https://en.wikipedia.org/wiki/Garner%27s_Modern_English_Usage">
 Garner's Modern English Usage</a> in its third edition: "<b>POSSESSIVES. A. Singular
 Possessives.</b>…Biblical and Classical names that end with a /zəs/ or /eez/
 sound take only the apostrophe." Some ask: is Notcurses then Biblical, or is it
 Classical? Truly, it is both.
</details>

<details>
  <summary>I just want to display a bitmap on my terminal. Your library is
  complex and stupid. You are simple and stupid.</summary>
  If you're willing to call a binary, use <tt>ncplayer</tt> to put an image,
  with desired scaling, anywhere on the screen and call it a day. Otherwise,
  call <tt>notcurses_init()</tt>, <tt>ncvisual_from_file()</tt>,
  <tt>ncvisual_blit()</tt>, <tt>notcurses_render()</tt>, and
  <tt>notcurses_stop()</tt>. It's not too tough. And thanks—your thoughtful
  comments and appreciative tone are why I work on Free Software.
</details>

## Useful links

* [BiDi in Terminal Emulators](https://terminal-wg.pages.freedesktop.org/bidi/)
* [The Xterm FAQ](https://invisible-island.net/xterm/xterm.faq.html)
  * [XTerm Control Sequences](https://invisible-island.net/xterm/ctlseqs/ctlseqs.pdf)
* [The NCURSES FAQ](https://invisible-island.net/ncurses/ncurses.faq.html)
* [ECMA-35 Character Code Structure and Extension Techniques](https://www.ecma-international.org/publications/standards/Ecma-035.htm) (ISO/IEC 2022)
* [ECMA-43 8-bit Coded Character Set Structure and Rules](https://www.ecma-international.org/publications/standards/Ecma-043.htm)
* [ECMA-48 Control Functions for Coded Character Sets](https://www.ecma-international.org/publications/standards/Ecma-048.htm) (ISO/IEC 6429)
* [Unicode 14.0 Full Emoji List](https://unicode.org/emoji/charts/full-emoji-list.html)
* [Unicode Standard Annex #29 Text Segmentation](http://www.unicode.org/reports/tr29)
* [Unicode Standard Annex #15 Normalization Forms](https://unicode.org/reports/tr15/)
* [mintty tips](https://github.com/mintty/mintty/wiki/Tips)
* [The TTY demystified](http://www.linusakesson.net/programming/tty/)
* [Dark Corners of Unicode](https://eev.ee/blog/2015/09/12/dark-corners-of-unicode/)
* [UTF-8 Decoder Capability and Stress Test](https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt)
* [Emoji: how do you get from U+1F355 to 🍕?](https://meowni.ca/posts/emoji-emoji-emoji/)
* [Glyph Hell: An introduction to glyphs, as used and defined in the FreeType engine](http://chanae.walon.org/pub/ttf/ttf_glyphs.htm)
* [Text Rendering Hates You](https://gankra.github.io/blah/text-hates-you/)
* [Use the UTF-8 code page](https://docs.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page)
* My wiki's [Sixel page](https://nick-black.com/dankwiki/index.php?title=Sixel) and Kitty's [extensions](https://sw.kovidgoyal.net/kitty/protocol-extensions.html).
* Linux man pages: [console_codes(4)](http://man7.org/linux/man-pages/man4/console_codes.4.html), [termios(3)](http://man7.org/linux/man-pages/man3/termios.3.html), [ioctl_tty(2)](http://man7.org/linux/man-pages/man2/ioctl_tty.2.html), [ioctl_console(2)](http://man7.org/linux/man-pages/man2/ioctl_console.2.html)
* The Microsoft Windows [Console Reference](https://docs.microsoft.com/en-us/windows/console/console-reference)
* NCURSES man pages: [terminfo(5)](http://man7.org/linux/man-pages/man5/terminfo.5.html), [user_caps(5)](http://man7.org/linux/man-pages/man5/user_caps.5.html)

> “Our fine arts were developed, their types and uses were established, in times
very different from the present, by men whose power of action upon things was
insignificant in comparison with ours. But the amazing growth of our
techniques, the adaptability and precision they have attained, the ideas and
habits they are creating, make it a certainty that _profound changes are
impending in the ancient craft of the Beautiful_.” —Paul Valéry
