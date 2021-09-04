# Terminals

Notcurses attempts to provide an abstraction layer over the highly varied
world of terminals. First and foremost, Notcurses needs to know the terminal
on which it is running, so that has an accurate understanding of its
capabilities.

It is of course possible that Notcurses is not connected to an actual
terminal (e.g. when running daemonized). In such a case, many escapes
will not be emitted, and no querying is performed.

Notcurses determines terminal capabilities via a combination of (more-or-less)
standardized queries sent to the terminal, the `TERM` environment variable
used by `terminfo(5)`, and the `COLORTERM` environment variable.

## Queries

At startup, the Linux console is identified via `ioctl(2)`s specific it.
Otherwise, if it is determined that the process is connected to a terminal
(see `isatty(3)`), Notcurses writes a series of queries to it. Several are
related to terminal identification:

* Send Tertiary Device Attributes (`CSI = c`)
  * Identifies VTE and foot
* Send Secondary Device Attributes (`CSI > c`)
  * Identifies Alacritty's version number
* `XTVERSION` (`CSI > 0 q`)
  * Identifies XTerm, WezTerm, and Contour
* `XTGETTCAP` for the `TN` key (`DCS + q 544e ST`)
  * Identifies Kitty and MLterm
* Send Primary Device Attributes (`CSI c`)

No terminals requiring special handling identify themselves via Primary Device
Attributes, but we send this because all known terminals respond to it with
*something*, preventing us from hanging, waiting for input (**if a terminal does
*not* reply in a recognizable way to Primary Device Attributes,
`notcurses_init()` will hang**).

Even if the terminal responds unambiguously to one of these queries, Notcurses
must have code to recognize the response, and bind it to some terminal
definition. Assuming the terminal to be thus identified, Notcurses enables or
disables certain capabilities based on built-in knowledge.

Terminal.App exports `TERM_PROGRAM=Apple_Terminal`.

## The `COLORTERM` environment variable

24-bit RGB for glyphs and cell backgrounds is fairly widely implemented. In
the Terminfo database, this is indicated via the `rgb` capability. It is
not uncommon for this capability to not be expressed, despite support being
present. Defining the `COLORTERM` environment variable with the value `24bit`
will instruct Notcurses to issue RGB sequences regardless.

## Terminfo and `TERM`

Even if the terminal is unambiguously determined via query, many capabilities
are acquired from the `terminfo(5)` database, keyed by the `TERM` environment
variable. It is critical that the `TERM` environment variable be correct for
your shell, and that the terminfo database entry keyed by this variable be
up-to-date.

The following have been established on a Debian Unstable workstation.
`ccc` is the Terminfo can-change-colors capability. "Blocks" refers to whether
the terminal provides its own implementation of block-drawing characters, or
relies on the font. Patches to correct/complete this table are very welcome!

| Terminal        | Pixel `TIOCGWINSZ` | `ccc` | Blocks | Recommended environment           | Notes |
| --------------- | ------------------ | ----- | ------ | -----------------------           | ----- |
| [Alacritty](https://github.com/alacritty/alacritty)       | ✅         |  ✅   |❌      |`TERM=alacritty` `COLORTERM=24bit` | [Sixel support WIP](https://github.com/ayosec/alacritty/tree/graphics) |
| [Contour](https://github.com/christianparpart/contour)    | ❌         |  ✅   |?       |`TERM=contour-latest` ?            | Claims Sixel support             |
| [ETerm](https://github.com/mej/Eterm) | | | | `TERM=Eterm` | Doesn't reply to Send Device Attributes |
| [FBterm](https://github.com/zhangyuanwei/fbterm)  | ❌                 |  ?    |?       |`TERM=fbterm`                      | 256 colors, no RGB color. |
| [foot](https://codeberg.org/dnkl/foot)            | ✅                 |  ✅   |✅      |`TERM=foot`                        | Sixel support. |
| [Gnome Terminal](https://gitlab.gnome.org/GNOME/gnome-terminal)  |❌   |  ❌   |✅      |`TERM=gnome` `COLORTERM=24bit`     | `ccc` support *is* available when run with `vte-256color`. |
| [Guake](https://github.com/Guake/guake)           |                    |  ?    |?       |                                   | |
| [ITerm2](https://github.com/gnachman/iTerm2)      | ✅  |  ✅   |✅    |`TERM=xterm-256color`  ||
| [Kitty](https://github.com/kovidgoyal/kitty)      | ✅  |  ✅   |✅    |`TERM=xterm-kitty`                 | See below. |
| [kmscon](https://github.com/dvdhrm/kmscon)        | | ❌    | ❌      |`TERM=xterm-256color`              | No RGB color AFAICT, nor any distinct terminfo entry. No actual `ccc` implementation. Sets `COLORTERM=kmscon`.|
| [Konsole](https://invent.kde.org/utilities/konsole) | ❌       |  ❌   |?       |`TERM=konsole-direct`              | |
| Linux console   | ❌                 |  ✅   |see [below](#the-linux-console) |`TERM=linux` `COLORTERM=24bit`   | 8 (512 glyph fonts) or 16 (256 glyph fonts) colors max, but RGB values are downsampled to a 256-index palette. See below. |
| [mintty](https://github.com/mintty/mintty) | ? | ? | ? | ? | ? |
| [mlterm](https://github.com/arakiken/mlterm)          | ✅                 |  ❌   |?       |`TERM=mlterm-256color`           | Do not set `COLORTERM`. `mlterm-direct` gives strange results. |
| [PuTTY](https://www.chiark.greenend.org.uk/~sgtatham/putty/latest.html)           |                    |  ❌   |❌      |`TERM=putty-256color` `COLORTERM=24bit` | |
| rxvt            | ✅                 |  ?    |?       |                                 | Seems unmaintained; many forks exist. |
| [Sakura](https://github.com/dabisu/sakura)          | ✅                 |  ✅   |?       |`TERM=vte-256color` `COLORTERM=24bit` | VTE-derived, no terminfo entry. |
| [GNU Screen](https://www.gnu.org/software/screen/)      | ✅                 |  ❌   |n/a     |`TERM=screen.OLDTERM`            | Must be compiled with `--enable-256color`. `TERM` should typically be `screen.` suffixed by the appropriate `TERM` value for the true connected terminal, e.g. `screen.vte-256color`. See below. |
| [st ("suckless")](https://st.suckless.org/) | ✅                 |  ✅   |?       |`TERM=st-256color` `COLORTERM=24bit` | Many features are maintained as external patches; users often roll their own instance, composing from these patches. |
| [Tabby](https://github.com/Eugeny/tabby) | ? | ? | ? | ? | |
| [Terminal.app](https://en.wikipedia.org/wiki/Terminal_(macOS)) | ✅ | ❌ | ❌ | `TERM=xterm-256color`| No RGB; no `ccc` despite wanting `xterm-256color`. |
| [Terminator](https://github.com/software-jessies-org/jessies/wiki/Terminatorhttps://github.com/software-jessies-org/jessies/wiki/Terminator)      | ✅                 |  ?    |?       | ?                               | |
| [Terminology](https://github.com/borisfaure/terminology)     | ❌                 |  ❌   |✅       | `TERM=terminology`              | Identified via DA3. 256 colors, no RGB. |
| [Tilda](https://github.com/lanoxx/tilda)  |       |  ?    |?       | ?                               | |
| [tmux](https://github.com/tmux/tmux/wiki) | ✅    |  ❌   |n/a     |`TERM=tmux-256color` `COLORTERM=24bit`| `tmux.conf` must apply `Tc`; see below. `bce` is available with the `tmux-256color-bce` definition. |
| [WezTerm](https://github.com/wez/wezterm) | ✅    |  ✅   |?       |`TERM=wezterm` `COLORTERM=24bit` | See below. |
| [Windows Terminal](https://github.com/microsoft/terminal)|                    |  ?    |?       | ?                               | Nice [escape docs](https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences).|
| [wterm](https://github.com/majestrate/wterm)           |                    |  ?    |?       | ?                               | |
| [XFCE4 Terminal](https://gitlab.xfce.org/apps/xfce4-terminal)  | ❌                 |  ✅   |✅      |`TERM=xfce` `COLORTERM=24bit`    | No `xfce-direct` variant exists. |
| [XTerm](https://invisible-island.net/xterm/xterm.html)           | ✅                 |  ?    |❌      |`TERM=xterm+256color2` `COLORTERM=24bit` | See note about DirectColor. Must configure with `--enable-direct-color`. `TERM=xterm-direct` seems to have the undesirable effect of mapping low RGB values to a palette; I don't yet understand this well. The problem is not seen with the specified configuration. Sixel support when built with `--enable-sixel-graphics` and run in vt340 mode. |
| [Yakuake](https://github.com/KDE/yakuake)         |                    |  ?    |?       | ?                               | |

Note that `xfce4-terminal`, `gnome-terminal`, etc. are essentially skinning
atop the common GNOME [VTE ("Virtual TErminal")](https://gitlab.gnome.org/GNOME/vte) library.

### Kitty

Kitty has some interesting, atypical behaviors. Foremost among these is that
an RGB background color equivalent to the configured default background color
will be rendered as the default background. This means, for instance, that if
the configured default background color is RGB(0, 0, 0), and is translucent,
a background of RGB(0, 0, 0) will be translucent. To work around this, when
`TERM` begins with "kitty", we detect the default background color, and when
we would write this as RGB, we alter one of the colors by 1. See
https://github.com/kovidgoyal/kitty/issues/3185 and
https://github.com/dankamongmen/notcurses/issues/1117.

Kitty is furthermore the only terminal I know to lack the `bce` (Background
Color Erase) capability, but Notcurses never relies on `bce` behavior.

Kitty has introduced an unambiguous [keyboard protocol](https://sw.kovidgoyal.net/kitty/keyboard-protocol/).
Notcurses supports this protocol when it is detected.

### WezTerm

WezTerm [implements](https://wezfurlong.org/wezterm/escape-sequences.html) some
interesting underline options, and both the Sixel and Kitty graphic protocols.

### GNU screen

GNU screen does have 24-bit color support, but only in the 5.X series. Note
that many distributions ship screen 4.X as of 2020. When built with truecolor
support, add `truecolor on` to your `screenrc`, or run it with `--truecolor`.
Attempting to force RGB color in screen 4.X **will not work**.

Add `defutf8 on` to your `screenrc`, or run screen with `-U`, to ensure UTF-8.

### tmux

`tmux` supports 24-bit color through its `Tc` (Truecolor) extension. You'll
need an entry in `tmux.conf` of the form:

`set -ga terminal-overrides ",EXTERNALTERM:Tc"`

Where `EXTERNALTERM` is your `TERM` variable at the time of attachment, e.g.:

`set -ga terminal-overrides ",vte-256color:Tc"`

You'll then need `COLORTERM=24bit` defined within your tmux environment.

### The Linux console

The Linux console supports concurrent virtual terminals, and is manipulated
by userspace via `ioctl()`s. These `ioctl()`s generally fail when applied to
a pseudotty device, as will happen if e.g. invoked upon one's controlling
terminal whilst running in a terminal emulator under X (it is still generally
possible to use them by explicitly specifying a console device, i.e.
`showconsolefont -C /dev/tty0`).

The VGA text console requires the kernel option `CONFIG_VGA_CONSOLE`. A
framebuffer console for VESA 2.0 is provided by `CONFIG_FB_VESA`, while
UEFI-compatible systems can use `CONFIG_FB_EFI`. So long as a framebuffer
driver is present, `CONFIG_FRAMEBUFFER_CONSOLE` will enable a graphics-mode
console using the framebuffer device.

The Linux console can be in either text or graphics mode. The mode can be
determined with the `KDGETMODE` `ioctl()`, and changed with `KDSETMODE`,
using the constants `KD_TEXT` and `KD_GRAPHICS`. Text mode supports a
rectangular matrix of multipixel cells, filled with glyphs from a font,
a foreground color, and a background color. Graphics ("All-Points-Addressable")
mode supports a rectangular matrix of pixels, each with a single color.
Note that both modes require appropriate hardware support (and kernel
configuration options), and might or might not be available on a given
installation. Non-x86 platforms often provide only a framebuffer (graphics)
console.

The kernel text mode loosely corresponds to the 1987 IBM VGA definition. At any
time, the display is configured with a monospace raster font, a palette, and
(when in Unicode mode) a mapping from multibyte sequences to font elements. Up
to 16 colors can be used with a font of 256 glyphs or fewer. Only 8 colors can
be used with fonts having more than 256 glyphs; the maximum font size in any
configuration is 512 glyphs. The keyboard is further configured with a keymap,
mapping keyboard scancodes to elements of the character set. These properties
are per-virtual console, not common to all of them. These limitations are not
typically present on framebuffer consoles.

Exporting `COLORTERM=24bit` and emitting RGB escapes to the Linux console
**does** work, though the RGB values provided are downsampled to a 256-slot
palette. Backgrounds don't seem to have the same degree of flexibility in this
situation as do foregrounds. The output is better, but not as much better as
one might expect. More research is necessary here.

The following more-or-less standard tools exist:
* `showconsolefont`: show the console font
* `setfont`: load console font
* `fbset`: show and modify framebuffer settings
* `fgconsole`: print name of foreground terminal
* `chvt`: change the foreground terminal
* `deallocvt`: destroy a virtual console
* `dumpkeys`: print all keycodes
* `loadkeys`: load scancode/keycode mapping (the keymap)
* `setkeycodes`: load scancode/keycode mappings one at a time
* `showkeys`: interactively print scancodes
* `kbd_mode`: show or set the keyboard mode

Both `mapscrn` and `loadunimap` are obsolete; their functionality is present
in `setfont`.

Note that Notcurses reprograms the console font table when running in the
Linux console (unless `NCOPTION_NO_FONT_CHANGES` is used). This adds support
for half blocks and quadrants.

### Windows Terminal

Ensure UTF-8 is being used for "Administrative language settings"
(see [README.md](README.md)). Codepage 65001 ought be used.

### 24-bit RGB

Many terminals support one or another form of non-indexed color encoding (also
known as DirectColor, RGB color, 24-bit color, or the similar but distinct
TrueColor), using either the semicolon-based presentation introduced by Konsole
or the colon-delimited presentation specified in ECMA-48 and ITU T.416. The
`rgb` termcap capability indicates support for such encodings via the
`set_a_foreground` and `set_b_foreground` capabilities. Not all terminals
implementing `rgb` use the 3x8bpc model; XTerm for instance:

> for values 0 through 7, it uses the “ANSI” control sequences, while
> for other values, it uses the 3-byte direct-color sequence introduced by Konsole.
> the number of colors is 224 while the number of color pairs is 216

Thus emitting `setaf` with an RGB value close to black can result, when
using `xterm-direct`'s `setaf` and `rgb` definitions, in a bright ANSI color.

24-bit RGB is always enabled for Kitty, Alacritty, Contour, WezTerm, iTerm2,
and foot.

### Problematic characters

Some characters seem to cause problems with one terminal or another. These
are best avoided until the problems are better understood:

* '­' U+00AD SOFT HYPHEN (some terminals allocate it a cell, some don't)
* '܏' U+070F SYRIAC ABBREVIATION MARK
* '۝' U+06DD ARABIC END OF AYAH
* '࣢' U+08E2 ARABIC DISPUTED END OF AYAH
* '﷽' U+FDFD ARABIC LIGATURE BISMILLAH AR-RAHMAN AR-RAHEEM
    '
