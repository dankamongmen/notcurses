# Direct mode

Direct mode allows you to use Notcurses together with standard I/O. While
the cursor can still be moved arbitrarily, direct mode is intended to be used
with newline-delimited, scrolling output. Direct mode has no concept of frame
rendering; output is intended to appear immediately (subject to buffering). It
is still necessary to have a valid `TERM` environment variable identifying a
valid terminfo database entry for the running terminal.

Enter direct mode with a call to `ncdirect_init()`.
