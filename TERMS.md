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
