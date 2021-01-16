# Terminals and `TERM`

With the wrong environment settings, programs can't properly control
your terminal. It is critical that the `TERM` environment variable be
correct for your shell, and that the terminfo database entry keyed
by this variable be up-to-date. Furthermore, for 24-bit TrueColor, it
is necessary to either use a `-direct` variant of your terminfo
entry, or to declare `COLORTERM=24bit`. The latter instruct Notcurses
to use 24-bit escapes regardless of advertised support. If you define
this variable, and your terminal doesn't actually support these sequences,
you're going to have a bad time.

The following have been established on a Debian Unstable workstation.
`ccc` is the Terminfo can-change-colors capability. `bce` is background
color erase. "Blocks" refers to whether the terminal provides its own
implementation of block-drawing characters, or relies on the font. Patches
to correct/complete this table are very welcome!

| Terminal        | `ccc` | `bce` | Blocks | Recommended environment         | Notes |
| --------------- | ----- | ----- | ------ | ------------------------------- | ----- |
| Alacritty       | ✔     | ✔     |🗴       |`TERM=alacritty` `COLORTERM=24bit` | |
| FBterm          | ?     | ?     |?       |`TERM=fbterm`                    | 256 colors, no RGB color. |
| Gnome Terminal  | 🗴     | ✔     |?       |`TERM=gnome` `COLORTERM=24bit`   | `ccc` support *is* available when run with `vte-256color`. |
| Guake           |       |       |        |                                 | |
| ITerm2          | ?     | ?     |?       |                                 | |
| Kitty           | ✔     | 🗴     |✔       |`TERM=xterm-kitty`               | |
| kmscon          | ?     | ?     |?       |`TERM=xterm-256color`            | No RGB color AFAICT, nor any distinct terminfo entry. |
| Konsole         | 🗴     | ✔     |?       |`TERM=konsole-direct`            | |
| Linux console   | ✔     | ✔     |N/A     |`TERM=linux` `COLORTERM=24bit`   | 8 (512 glyph fonts) or 16 (256 glyph fonts) colors max, but RGB values are downsampled to a 256-index palette. See below. |
| mlterm          | 🗴     | ✔     |?       |`TERM=mlterm-256color`           | Do not set `COLORTERM`. `mlterm-direct` gives strange results. |
| PuTTY           | ?     | ?     |?       |                                 | |
| rxvt            | ?     | ?     |?       |                                 | |
| Sakura          | ✔     | ✔     |?       |`TERM=vte-256color` `COLORTERM=24bit` | No terminfo entry? |
| GNU Screen      | 🗴     | ✔     |?       |`TERM=screen.OLDTERM`            | Must be compiled with `--enable-256color`. `TERM` should typically be `screen.` suffixed by the appropriate `TERM` value for the true connected terminal, e.g. `screen.vte-256color`. See below. |
| st              | ✔     | ✔     |?       |`TERM=st-256color` `COLORTERM=24bit` | |
| Terminator      | ?     | ?     |?       | ?                               | |
| Terminology     | ?     | ?     |?       | ?                               | |
| Tilda           | ?     | ?     |?       | ?                               | |
| tmux            | ?     | ?     |?       | ?                               | |
| wezterm         | ✔     | ✔     |?       |`TERM=wezterm` `COLORTERM=24bit` | |
| Windows Terminal| ?     | ?     |?       | ?                               | |
| wterm           | ?     | ?     |?       | ?                               | |
| XFCE4 Terminal  | ✔     | ✔     |?       |`TERM=xfce` `COLORTERM=24bit`    | No `xfce-direct` variant exists. |
| XTerm           | ?     | ✔     |🗴       |`TERM=xterm+256color2` `COLORTERM=24bit` | See note about DirectColor. Must configure with `--enable-direct-color`. `TERM=xterm-direct` seems to have the undesirable effect of mapping low RGB values to a palette; I don't yet understand this well. The problem is not seen with the specified configuration. Sixel support when built with `--enable-sixel-graphics` and run in vt340 mode. |
| Yakuake         | ?     | ?     |?       | ?                               | |

Note that `xfce4-terminal`, `gnome-terminal`, etc. are essentially skinning
atop the common VTE ("Virtual TErminal") library.

## Kitty

Kitty has some interesting, atypical behaviors. Foremost among these is that
an RGB background color equivalent to the configured default background color
will be rendered as the default background. This means, for instance, that if
the configured default background color is RGB(0, 0, 0), and is translucent,
a background of RGB(0, 0, 0) will be translucent. To work around this, when
`TERM` begins with "kitty", we detect the default background color, and when
we would write this as RGB, we alter one of the colors by 1. See
https://github.com/kovidgoyal/kitty/issues/3185 and
https://github.com/dankamongmen/notcurses/issues/1117. This behavior can
be demonstrated with the `kittyzapper` binary (`src/poc/kittyzapper.c`).

Kitty is furthermore the only terminal I know to lack the `bce` capability, but
Notcurses never relies on `bce` behavior.

## GNU screen

GNU screen does have 24-bit color support, but only in the 5.X series. Note
that many distributions ship screen 4.X as of 2020. When built with truecolor
support, add `truecolor on` to your `screenrc`, or run it with `--truecolor`.
Attempting to force RGB color in screen 4.X **will not work**.

Add `defutf8 on` to your `screenrc`, or run screen with `-U`, to ensure UTF-8.

## The Linux console

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

## DirectColor

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

## Problematic characters

Some characters seem to cause problems with one terminal or another. These
are best avoided until the problems are better understood:

* '­' U+00AD SOFT HYPHEN (some terminals allocate it a cell, some don't)
* '܏' U+070F SYRIAC ABBREVIATION MARK
* '۝' U+06DD ARABIC END OF AYAH
* '࣢' U+08E2 ARABIC DISPUTED END OF AYAH
* '﷽' U+FDFD ARABIC LIGATURE BISMILLAH AR-RAHMAN AR-RAHEEM
    '
