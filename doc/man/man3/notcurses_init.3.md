% notcurses_init(3)
% nick black <nickblack@linux.com>
% v3.0.9

# NAME

notcurses_init - initialize a Notcurses instance

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
#define NCOPTION_INHIBIT_SETLOCALE   0x0001ull
#define NCOPTION_NO_CLEAR_BITMAPS    0x0002ull
#define NCOPTION_NO_WINCH_SIGHANDLER 0x0004ull
#define NCOPTION_NO_QUIT_SIGHANDLERS 0x0008ull
#define NCOPTION_PRESERVE_CURSOR     0x0010ull
#define NCOPTION_SUPPRESS_BANNERS    0x0020ull
#define NCOPTION_NO_ALTERNATE_SCREEN 0x0040ull
#define NCOPTION_NO_FONT_CHANGES     0x0080ull
#define NCOPTION_DRAIN_INPUT         0x0100ull
#define NCOPTION_SCROLLING           0x0200ull

#define NCOPTION_CLI_MODE (NCOPTION_NO_ALTERNATE_SCREEN \
                           |NCOPTION_NO_CLEAR_BITMAPS \
                           |NCOPTION_PRESERVE_CURSOR \
                           |NCOPTION_SCROLLING)

typedef enum {
  NCLOGLEVEL_SILENT,  // print nothing once fullscreen service begins
  NCLOGLEVEL_PANIC,   // default. print diagnostics before we crash/exit
  NCLOGLEVEL_FATAL,   // we're hanging around, but we've had a horrible fault
  NCLOGLEVEL_ERROR,   // we can't keep doing this, but we can do other things
  NCLOGLEVEL_WARNING, // you probably don't want what's happening to happen
  NCLOGLEVEL_INFO,    // "standard information"
  NCLOGLEVEL_VERBOSE, // "detailed information"
  NCLOGLEVEL_DEBUG,   // this is honestly a bit much
  NCLOGLEVEL_TRACE,   // there's probably a better way to do what you want
} ncloglevel_e;

typedef struct notcurses_options {
  const char* termtype;
  ncloglevel_e loglevel;
  unsigned margin_t, margin_r, margin_b, margin_l;
  uint64_t flags; // from NCOPTION_* bits
} notcurses_options;
```

**struct notcurses* notcurses_init(const notcurses_options* ***opts***, FILE* ***fp***);**

**void notcurses_version_components(int* ***major***, int* ***minor***, int* ***patch***, int* ***tweak***);**

**int notcurses_cursor_enable(struct notcurses* ***nc***, int ***y***, int ***x***);**

**int notcurses_cursor_disable(struct notcurses* ***nc***);**

**int notcurses_cursor_yx(const struct notcurses* ***nc***, int* ***y***, int* ***x***);**

**int notcurses_lex_margins(const char* ***op***, notcurses_options* ***opts***);**

**int notcurses_default_foreground(const struct notcurses* ***nc***, uint32_t* ***fg***);**

**int notcurses_default_background(const struct notcurses* ***nc***, uint32_t* ***bg***);**

# DESCRIPTION

**notcurses_init** prepares the terminal for cursor-addressable (multiline)
mode, writing to some **FILE**. If the **FILE** provided as ***fp*** is
**NULL**, **stdout** will be used. Whatever **FILE** is used must be writable,
is ideally fully buffered, and must be byte-oriented (see **fwide(3)**).
If the **FILE** is not connected to a terminal (for example when redirected to
a file), **/dev/tty** will be opened (if possible) for communication with the
controlling terminal. Most output (including all styling and coloring) is
written to the **FILE**; only queries need be sent to a true terminal. If no
terminal is available (for example when running as a daemon), defaults
regarding such things as screen size and the palette are assumed.

The **struct notcurses_option** passed as ***opts*** controls behavior.
Passing a **NULL** ***opts*** is equivalent to passing an all-zero (default)
***opts***. A process can have only one Notcurses context active at a time;
calling **notcurses_init** again before calling **notcurses_stop** will
return **NULL** (this is reliable even if called concurrently from two
threads).

On success, a pointer to a valid **struct notcurses** is returned. **NULL** is
returned on failure. Before the process exits, **notcurses_stop(3)** should be
called to reset the terminal and free up resources.

An appropriate **terminfo(5)** entry must exist for the terminal. This entry is
usually selected using the value of the **TERM** environment variable (see
**getenv(3)**), but a non-**NULL** value for ***termtype*** will override this
(terminfo is not used on Microsoft Windows, where it is neither meaningful nor
necessary to define **TERM**). An invalid terminfo specification
can lead to reduced performance, reduced display capabilities, and/or display
errors. Notcurses natively targets 24bpp/8bpc RGB color, and it is thus
desirable to use a terminal with the **rgb** capability (e.g. xterm's
**xterm-direct**). Colors will otherwise be quantized down to whatever the
terminal supports.

If the terminal advertises support for an "alternate screen" via the **smcup**
terminfo capability, Notcurses will employ it by default. This can be prevented
by setting **NCOPTION_NO_ALTERNATE_SCREEN** in ***flags***. Users tend to have
strong opinions regarding the alternate screen, so it's often useful to expose
this via a command-line option. When the alternate screen is not used, the
contents of the terminal at startup remain visible until obliterated, on a
cell-by-cell basis (see **notcurses_plane(3)** for details on clearing the
screen at startup without using the alternate screen). If the alternate screen
is not available, the display will still be cleared without
**NCOPTION_NO_ALTERNATE_SCREEN**.

Notcurses hides the cursor by default. It can be dynamically enabled, moved, or
disabled during execution via **notcurses_cursor_enable** and
**notcurses_cursor_disable**. It will be hidden while updating the screen.
The current location of the terminal cursor can be acquired with
**notcurses_cursor_yx**, whether visible or not.

**notcurses_init** typically emits some diagnostics at startup, including version
information and some details of the configured terminal. This can be inhibited
with **NCOPTION_SUPPRESS_BANNERS**. This will also inhibit the performance
summary normally printed by **notcurses_stop(3)**.

Notcurses can render to a subregion of the terminal by specifying desired
margins on all four sides. By default, all margins are zero, and thus rendering
will be performed on the entirety of the viewing area. This is orthogonal to
use of the alternate screen; using the alternate screen plus margins will see
the full screen cleared, followed by rendering to a subregion. Inhibiting the
alternate screen plus margins will render to a subregion, with the screen
outside this region not cleared. Margins are best-effort.
**notcurses_lex_margins** provides lexing a margin argument expression in one
of two forms:

* a single number, which will be applied to all sides, or
* four comma-delimited numbers, applied to top, right, bottom, and left.

To allow future options without requiring redefinition of the structure, the
***flags*** field is only a partially-defined bitfield. Undefined bits should be
zero. The following flags are defined:

* **NCOPTION_INHIBIT_SETLOCALE**: Unless this flag is set, **notcurses_init**
    will call **setlocale(LC_ALL, NULL)**. If the result is either "**C**" or
    "**POSIX**", it will print a diagnostic to **stderr**, and then call
    **setlocale(LC_ALL, "").** This will attempt to set the locale based off
    the **LANG** environment variable. Your program should call **setlocale(3)**
    itself, usually as one of the first lines.

* **NCOPTION_NO_CLEAR_BITMAPS**: On entry, make no special attempt to clear any
    preexisting bitmaps. Note that they might still get cleared even if this is
    set, and they might not get cleared even if this is not set.

* **NCOPTION_NO_WINCH_SIGHANDLER**: A signal handler will usually be installed
    for **SIGWINCH** and **SIGCONT**, resulting in **NCKEY_RESIZE** events
    being generated on input. With this flag, the handler will not be
    installed.

* **NCOPTION_NO_QUIT_SIGHANDLERS**: A signal handler will usually be installed
    for **SIGABRT**, **SIGBUS**, **SIGFPE**, **SIGILL**, **SIGINT**,
    **SIGQUIT**, **SIGSEGV**, and **SIGTERM**, cleaning up the terminal on such
    exceptions. With this flag, the handler will not be installed.

* **NCOPTION_PRESERVE_CURSOR**: The virtual cursor is typically placed at the
    screen's origin at startup. With this flag, it is instead placed wherever
    the cursor was at program launch.

* **NCOPTION_SUPPRESS_BANNERS**: Disables the diagnostics and version
    information printed on startup, and the performance summary on exit.

* **NCOPTION_NO_ALTERNATE_SCREEN**: Do not use the alternate screen
    (see **terminfo(5)**), even if it is available.

* **NCOPTION_NO_FONT_CHANGES**: Do not touch the font. Notcurses might
    otherwise attempt to extend the font, especially in the Linux console.

* **NCOPTION_DRAIN_INPUT**: Standard input may be freely discarded. If you do not
    intend to process input, pass this flag. Otherwise, input can buffer up, and
    eventually prevent Notcurses from processing messages from the terminal. It
    will furthermore avoid wasting time processing useless input.

* **NCOPTION_SCROLLING**: Enable scrolling on the standard plane. This is
    equivalent to calling **ncplane_set_scrolling(stdn, true)** on some
    standard plane ***stdn***.

**NCOPTION_CLI_MODE** is provided as an alias for the bitwise OR of
**NCOPTION_SCROLLING**, **NCOPTION_NO_ALTERNATE_SCREEN**,
**NCOPTION_PRESERVE_CURSOR**, and **NCOPTION_NO_CLEAR_BITMAPS**. If
writing a CLI, it is recommended to use **NCOPTION_CLI_MODE** rather
than explicitly listing these options.

**notcurses_default_foreground** returns the default foreground color, if it
could be detected. **notcurses_default_background** returns the default
background color, if it could be detected.

## Fatal signals

It is important to reset the terminal before exiting, whether terminating due
to intended operation or a received signal. This is usually accomplished by
explicitly calling **notcurses_stop(3)** during shutdown. For convenience, Notcurses
by default installs signal handlers for various signals which would typically
result in process termination (see **signal(7)**). These signal handlers call
**notcurses_stop(3)** for each **struct notcurses** in the process, and then propagate
the signal to any previously-configured handler. These handlers are disabled
upon entry to **notcurses_stop(3)**.

To prevent signal handler registration, provide **NCOPTION_NO_QUIT_SIGHANDLERS**.
No means is provided to selectively register fatal signal handlers. If this is
done, the caller ought be sure to effect similar functionality themselves.

## Resize events

**SIGWINCH** (SIGnal WINdow CHange) is delivered to the process when the terminal
is resized. The default action is to ignore it (**SIG_IGN**). Notcurses installs
a handler for this signal. The handler causes Notcurses to update its idea of
the terminal's size using **TIOCGWINSZ** (see **ioctl_tty(2)**), and generates an
**NCKEY_RESIZE** input event (see **notcurses_input(3)**. This signal handler can be
inhibited by setting **NCOPTION_NO_WINCH_SIGHANDLER** in **flags**. If this is
done, the caller should probably watch for the signal, and invoke
**notcurses_refresh(3)** or **notcurses_render(3)** upon its receipt.

A resize event does not invalidate any references returned earlier by
Notcurses. The content of any new screen area is undefined until the next call
to **notcurses_render(3)**. This is true even if an existing **struct ncplane**
(see **notcurses_plane(3)**) overlaps the new area, since the signal could
arrive while the ncplanes are being modified. Signal handlers are quite
restricted as to what actions they can perform, so minimal work is performed in
the handler proper.

Thus, in the absence of **NCOPTION_NO_WINCH_SIGHANDLER**, **SIGWINCH** results in:

* interruption of some thread to process the signal
* a **TIOCGWINSZ** **ioctl** to retrieve the new screen size
* queuing of a **NCKEY_RESIZE** input event (if there is space in the queue)

Upon the next call to **notcurses_render(3)** or **notcurses_refresh(3)**, the
standard plane (see **notcurses_stdplane(3)**) will be resized to the new
screen size. The next **notcurses_render(3)** call will function as expected
across the new screen geometry.

## The hardware cursor

Most terminals provide a cursor, a visual indicator of where output will next
be placed. There is usually (but not always) some degree of control over what
glyph forms this cursor, and whether it e.g. blinks.

By default, Notcurses disables this cursor in rendered mode. It can be turned
back on with **notcurses_enable_cursor**, which has immediate effect (there is
no need to call **notcurses_render(3)**). If already visible, this function
updates the location. Each time the physical screen is updated, Notcurses will
disable the cursor, write the update, move the cursor back to this location,
and finally make the cursor visible. **notcurses_cursor_yx** retrieves the
location of the cursor, whether visible or not. **notcurses_disable_cursor**
hides the cursor.

You generally shouldn't need to touch the terminal cursor. It's only really
relevant with echoed user input, and you don't want echoed user input in
rendered mode (instead, read the input, and write it to a plane yourself).
A subprocess can be streamed to a plane with an **ncsubproc**, etc.

If the **NCOPTION_PRESERVE_CURSOR** flag is provided, the cursor's location
will be determined at startup, and the standard plane's virtual cursor will
be placed to match it (instead of in the upper-left corner). Combined with
**NCOPTION_NO_ALTERNATE_SCREEN** and a scrolling standard plane, this allows
rendered mode to be used as a normal scrolling shell application.

# RETURN VALUES

**NULL** is returned on failure. Otherwise, the return value points at a valid
**struct notcurses**, which can be used until it is provided to
**notcurses_stop(3)**.

**notcurses_cursor_disable** returns -1 if the cursor is already invisible.

**notcurses_default_foreground** returns -1 if the default foreground color
could not be detected.

**notcurses_default_background** returns -1 if the default background color
could not be detected.

# ENVIRONMENT VARIABLES

The **NOTCURSES_LOGLEVEL** environment variable, if defined, ought be an
integer between -1 and 7. These values correspond to **NCLOGLEVEL_SILENT**
through **NCLOGLEVEL_TRACE**, and override the **loglevel** field of
**notcurses_options**.

The **TERM** environment variable will be used by **setupterm(3ncurses)** to
select an appropriate terminfo database.

# NOTES

Several command-line options and keybindings are recommended for Notcurses
rendered-mode programs:

* **-l[0-8]** ought be mapped to the various **NCLOGLEVEL** values.
  Alternatively, map **-v** to **NCLOGLEVEL_WARNING**, and map
  **-vv** to **NCLOGLEVEL_INFO**.
* **-k** ought be mapped to **NCOPTION_NO_ALTERNATE_SCREEN**.
* Ctrl+L ought be mapped to **notcurses_refresh(3)**.

# BUGS

The introductory diagnostics are not currently emitted when the alternate
screen is used. They ought be printed to the regular screen before entering
the alternate screen. They are displayed normally when
**NCOPTION_NO_ALTERNATE_SCREEN** is used.

# SEE ALSO

**fwide(3)**,
**getenv(3)**,
**setlocale(3)**,
**termios(3)**,
**notcurses(3)**,
**notcurses_input(3)**,
**notcurses_plane(3)**,
**notcurses_refresh(3)**,
**notcurses_render(3)**,
**notcurses_stop(3)**,
**setupterm(3ncurses)**,
**terminfo(5)**,
**signal(7)**
