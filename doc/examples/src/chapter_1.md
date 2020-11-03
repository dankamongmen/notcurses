# Chapter 1

Notcurses is a library for terminal applications—programs which do not require
a GUI environment, and write output as cells rather than pixels. Within a GUI
environment, such programs run in a *terminal emulator*; they can also run
on a *virtual console*, a *pseudoterminal* (such as those used by `ssh`), or
even a true hardware terminal. It is inspired by the X/Open Curses
specification, though it is (by necessity) not source-compatible with that API.

Terminal applications emit a stream of *code points*. Code points index into
the terminal's *character set*; visual characters are run through a font engine
to produce *glyphs*, whereas *escape sequences* facilitate in-band control of
the output and operation of the terminal, including changing presentation style
and moving the cursor. A major function of Notcurses is to portably and
efficiently emit the sequences necessary to effect higher-level client goals.

Notcurses offers two major modes of operation:
* **Direct mode** can be used together with standard I/O. It is primarily meant
  to enhance simple scrolling applications via coloring and styling, though it
  can also move the cursor, read raw keyboard input, and render media. Use
  direct mode if you intend to primarily output through standard library
  functions such as `printf()`. Direct mode uses `struct ncdirect`s.
* **Fullscreen mode** facilitates more complex Text User Interface applications.
  These applications maintain virtual state in the form of an ordered stack of
  `struct ncplane`s; when they're ready, the visible area is redrawn. It can
  provide much greater performance than direct mode, but does not tolerate the
  use of standard I/O. Fullscreen mode uses `struct notcurses`.

Whichever mode is used, it is essential to destroy all Notcurses contexts upon
program exit, or the terminal can be left in an undesirable state. By default,
Notcurses registers handlers for most fatal signals, to destroy its own
contexts. On a typical exit, the user must destroy these contexts.

Notcurses does not strictly require a terminal device for input or output; it
can be freely redirected to arbitrary character devices or files. When not
connected to a terminal device, many escapes will not be generated.
