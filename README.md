# Notcurses: blingful TUIs and character graphics

**What it is**: a library facilitating complex TUIs on modern terminal
emulators, supporting vivid colors, multimedia, threads, and Unicode to the
maximum degree possible. [Things](https://www.youtube.com/watch?v=dcjkezf1ARY) can be done with
Notcurses that simply can't be done with NCURSES. It is furthermore
fast as shit. **What it is not**: a source-compatible X/Open Curses implementation, nor a
replacement for NCURSES on existing systems.

<p align="center">
<a href="https://www.youtube.com/watch?v=dcjkezf1ARY"><img src="https://raw.githubusercontent.com/dankamongmen/notcurses/gh-pages/notcurses-logo.png" alt="setting the standard (hype video)"/></a>
birthed screaming into this world by <a href="https://nick-black.com/dankwiki/index.php/Hack_on">nick black</a> (<a href="mailto:nickblack@linux.com">&lt;nickblack@linux.com&gt;</a>)
</p>

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

<a href="https://repology.org/project/notcurses/versions">
<img src="https://repology.org/badge/vertical-allrepos/notcurses.svg" alt="Packaging status" align="right">
</a>

![Linux](https://img.shields.io/badge/-Linux-grey?logo=linux)
![FreeBSD](https://img.shields.io/badge/-FreeBSD-grey?logo=freebsd)
![OSX](https://img.shields.io/badge/-OSX-grey?logo=osx)
[![Matrix](https://img.shields.io/matrix/notcursesdev:matrix.org?label=matrixchat)](https://app.element.io/#/room/#notcursesdev:matrix.org)
[![Sponsor](https://img.shields.io/badge/-Sponsor-red?logo=github)](https://github.com/sponsors/dankamongmen)

[![Build](https://drone.dsscaw.com:4443/api/badges/dankamongmen/notcurses/status.svg)](https://drone.dsscaw.com:4443/dankamongmen/notcurses)
[![🐧 UbuntuTests](https://github.com/dankamongmen/notcurses/actions/workflows/ubuntu_test.yml/badge.svg)](https://github.com/dankamongmen/notcurses/actions/workflows/ubuntu_test.yml)

[![pypi_version](https://img.shields.io/pypi/v/notcurses?label=pypi)](https://pypi.org/project/notcurses)
[![crates.io](https://img.shields.io/crates/v/libnotcurses-sys.svg)](https://crates.io/crates/libnotcurses-sys)

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
* (OPTIONAL) (build+runtime) GNU [Readline](https://www.gnu.org/software/readline/) 8.0+
* (OPTIONAL) (build+runtime) From QR-Code-generator: [libqrcodegen](https://github.com/nayuki/QR-Code-generator) 1.5.0+
* (OPTIONAL) (build+runtime) From [FFmpeg](https://www.ffmpeg.org/): libswscale 5.0+, libavformat 57.0+, libavutil 56.0+
* (OPTIONAL) (build+runtime) [OpenImageIO](https://github.com/OpenImageIO/oiio) 2.15.0+, requires C++
* (OPTIONAL) (testing) [Doctest](https://github.com/onqtam/doctest) 2.3.5+
* (OPTIONAL) (documentation) [pandoc](https://pandoc.org/index.html) 1.19.2+
* (OPTIONAL) (python bindings): Python 3.7+, [CFFI](https://pypi.org/project/cffi/) 1.13.2+, [pypandoc](https://pypi.org/project/pypandoc/) 1.5+
* (OPTIONAL) (rust bindings): rust 1.47.0+, [bindgen](https://crates.io/crates/bindgen) 0.55.1+, pkg-config 0.3.18+, cty 0.2.1+
* (runtime) Linux 5.3+, FreeBSD 11+, DragonFly BSD 5.9+, or OSX 11.4+

[Here's more information](INSTALL.md) on building and installation.

## Included tools

Eight binaries are installed as part of Notcurses:
* `ncls`: an `ls` that displays multimedia in the terminal
* `ncneofetch`: a [neofetch](https://github.com/dylanaraps/neofetch) ripoff
* `ncplayer`: renders visual media (images/videos)
* `nctetris`: a tetris clone
* `notcurses-demo`: some demonstration code
* `notcurses-info`: detect and print terminal capabilities/diagnostics
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

If you (understandably) want to avoid the large Pandoc stack, but still enjoy
manual page goodness, I publish a tarball with generated man/XHTML along with
each release. Download it, and install the contents as you deem fit.

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

* If your terminal supports 3x8bit RGB color via `setaf` and `setbf` (most modern
  terminals), but exports neither the `RGB` nor `Tc` terminfo capability, you can export
  the `COLORTERM` environment variable as `truecolor` or `24bit`. Note that some
  terminals accept a 24-bit specification, but map it down to fewer colors.
  RGB is enabled whenever [Foot, Contour, WezTerm, or Kitty](TERMINALS.md) is
  identified.

### Fonts

Fonts end up being a whole thing, little of which is pleasant. I'll write this
up someday **FIXME**.

It is worth knowing that several terminals draw the block characters directly,
rather than loading them from a font. This is generally desirable. To inspect
your environment's rendering of drawing characters, run `notcurses-info`.

## FAQs

If things break or seem otherwise lackluster, **please** consult the
[Environment Notes](#environment_notes) section! You **need** to have a correct
`TERM` and `LANG` definition, and probably want `COLORTERM`.

<details>
  <summary>The demo fails in the middle of <code>intro</code>.</summary>
  Check that your <code>TERM</code> value is correct for your terminal.
  <code>intro</code> does a palette fade, which is prone to breaking under
  incorrect <code>TERM</code> values.
  If you're not using <code>xterm</code>, your <code>TERM</code> should not be
  <code>xterm</code>!
</details>

<details>
  <summary>Can I have Notcurses without this huge multimedia stack?</summary>
  Yes! Build with <code>-DUSE_MULTIMEDIA=none</code>.
</details>

<details>
  <summary>Can I build this individual Notcurses program without aforementioned
  multimedia stack?</summary>
  Again yes! Use <code>notcurses_core_init()</code> or
  <code>ncdirect_core_init()</code> in place of <code>notcurses_init()</code>/
  <code>ncdirect_init()</code>, and link with <code>-lnotcurses-core</code>.
  Your application will likely start a few milliseconds faster;
  more importantly, it will link against minimal Notcurses installations.
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
  <summary>Why didn't you just render everything to Sixel?</summary>
  That's not a TUI; it's a slow and inflexible GUI. Many terminal emulators
  don't support Sixel. Sixel doesn't work well with mouse selection.
  Sixel has a limited color palette. With that said, both Sixel and the
  Kitty bitmap protocol are well-supported.
</details>

<details>
  <summary>I'm not seeing <code>NCKEY_RESIZE</code> until I press
  some other key.</summary>
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
  Pass <code>NCDIRECT_OPTION_CBREAK</code> to <code>ncdirect_init()</code>, and
  ensure you do not pass <code>NCDIRECT_OPTION_NO_READLINE</code>. If you'd like,
  set <code>rl_readline_name</code> and <code>rl_attempted_completion_function</code>
  prior to calling <code>ncdirect_init()</code>. With that said, consider using
  a Notcurses <code>ncreader</code>.
</details>

<details>
  <summary>Will there ever be Java wrappers?</summary>
  I should hope not. If you want a Java solution, try Autumn Lamonte's
  <a href="https://jexer.sourceforge.io/">Jexer</a>.
</details>

<details>
  <summary>Given that the glyph channel is initialized as transparent for a
  plane, shouldn't the foreground and background be initialized as transparent,
  also?</summary>
  Probably (they are instead initialized to default opaque). This would change
  some of the most longstanding behavior of Notcurses, though,
  so it isn't happening.
</details>

<details>
  <summary>Why does my right-to-left text appear left-to-right?</summary>
  Notcurses doesn't honor the BiDi state machine, and in fact forces
  left-to-right with BiDi codes. Likewise, ultra-wide glyphs will have
  interesting effects. ﷽!
</details>

<details>
  <summary>I get linker errors when statically linking.</summary>
  Are you linking all necessary libraries? Use
  <code>pkg-config --static --libs notcurses</code>
  (or <code>--libs notcurses-core</code>) to discover them.
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
  <summary>How about *arbitrary image manipulation here* functionality?</summary>
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
* [mintty tips](https://github.com/mintty/mintty/wiki/Tips)
* [The TTY demystified](http://www.linusakesson.net/programming/tty/)
* [Dark Corners of Unicode](https://eev.ee/blog/2015/09/12/dark-corners-of-unicode/)
* [UTF-8 Decoder Capability and Stress Test](https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt)
* [Emoji: how do you get from U+1F355 to 🍕?](https://meowni.ca/posts/emoji-emoji-emoji/)
* [Glyph Hell: An introduction to glyphs, as used and defined in the FreeType engine](http://chanae.walon.org/pub/ttf/ttf_glyphs.htm)
* [Text Rendering Hates You](https://gankra.github.io/blah/text-hates-you/)
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
