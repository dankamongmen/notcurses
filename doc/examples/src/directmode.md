# Direct mode

Direct mode allows you to use Notcurses together with standard I/O. While
the cursor can still be moved arbitrarily, direct mode is intended to be used
with newline-delimited, scrolling output. Direct mode has no concept of frame
rendering; output is intended to appear immediately (subject to buffering). It
is still necessary to have a valid `TERM` environment variable identifying a
valid terminfo database entry for the running terminal.

The authoritative reference for direct mode is the `notcurses_direct(3)`
man page.

Enter direct mode with a call to `ncdirect_init()`. It takes three arguments:

`struct ncdirect* ncdirect_init(const char* termtype, FILE* fp, uint64_t flags);`

You can usually simply pass `NULL`, `NULL`, and 0. This will use the terminal
entry specified by the `TERM` environment variable, and write to `stdout`. If
the terminfo entry cannot be loaded, `ncdirect_init()` will fail. Otherwise,
the active style (italics, reverse video, etc.) will be reset to the default,
and all `ncdirect` functions are available to use. When done with the context,
call `ncdirect_stop()` to release its resources, and restore the terminal's
preserved status.

The cursor is not moved by initialization. If your program was invoked as
`ncdirect-demo` from an interactive shell, the cursor is most likely to be
on the first column of the line following your command prompt, exactly where
a program like `ls` would start its output.

```c
{{#include directmode-helloworld.c}}
```
![](directmode-helloworld.png)

The terminal will scroll on output just like it normally does, and if you have
a scrollback buffer, any output you generate will be present there. Remember:
direct mode simply *styles* standard output. With that said, the cursor can be
freely controlled in direct mode, and moved arbitrarily within the viewing
region. Dimensions of the terminal can be acquired with `ncdirect_dim_y()` and
`ncdirect_dim_x()` (if you initialized direct mode with a file not attached to
a terminal, Notcurses will simulate a 80x24 canvas). The cursor's location is
found with `ncdirect_cursor_yx()` (this function fails if run on a
non-terminal, or if the terminal does not support this capability).

**ncdirect_ cursor functions**

The cursor can be moved relative to its current location, or absolutely within
the terminal's boundaries. The location can also be pushed and popped (''FIXME''
why don't we synthesize this in the absence of `sc/rc`? why is this exposed at
all? do some terminals support `sc` but not cursor location discovery?).

The cursor can be hidden and made visible once more with `ncdirect_cursor_disable()`
and `ncdirect_cursor_enable()`. Unlike rendered mode, the cursor is not hidden
by default at initialization.

**example scrolling a screen's worth of text**
**picture before and after run**
