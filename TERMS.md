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

| Terminal | Recommended environment | Notes |
| -------- | ------ | ----- |
| Linux console | `TERM=linux` | 8 (512 glyph fonts) or 16 (256 glyph fonts) colors max. |
| FBterm | | |
| kmscon | `TERM=xterm-256color` | No RGB color AFAICT, nor any distinct terminfo entry. |
| XTerm | `TERM=xterm-256color` `COLORTERM=24bit` | Must configure with `--enable-direct-color`. `TERM=xterm-direct` seems to have the undesirable effect of mapping low RGB values to a palette; I don't yet understand this well. The problem is not seen with the specified configuration. |
| XFCE4 Terminal | `TERM=xfce` `COLORTERM=24bit` | No `xfce-direct` variant exists |
| Gnome Terminal | `TERM=gnome` `COLORTERM=24bit` | |
| Konsole | `TERM=konsole-direct` | |
| Alacritty | `TERM=alacritty` `COLORTERM=24bit` | |
| Kitty | `TERM=kitty-direct` | |
| Sakura | `TERM=vte-256color` `COLORTERM=24bit` | |
| st | `TERM=st-256color` `COLORTERM=24bit` | |
| GNU Screen | `TERM=screen-256colors` | Must be compiled with `--enable-256color`. |
| tmux | | |

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
