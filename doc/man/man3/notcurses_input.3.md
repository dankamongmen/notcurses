% notcurses_input(3)
% nick black <nickblack@linux.com>
% v2.4.8

# NAME

notcurses_input - input via notcurses

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
struct timespec;
struct notcurses;

typedef struct ncinput {
  uint32_t id;     // Unicode codepoint
  int y;           // Y cell coordinate of event, -1 for undefined
  int x;           // X cell coordinate of event, -1 for undefined
  bool alt;        // Was Alt held during the event?
  bool shift;      // Was Shift held during the event?
  bool ctrl;       // Was Ctrl held during the event?
  enum {
    EVTYPE_UNKNOWN,
    EVTYPE_PRESS,
    EVTYPE_REPEAT,
    EVTYPE_RELEASE,
  } evtype;
  int ypx, xpx;    // pixel offsets within cell, -1 for undefined
} ncinput;

#define NCMICE_NO_EVENTS     0
#define NCMICE_MOVE_EVENT    0x1
#define NCMICE_BUTTON_EVENT  0x2
#define NCMICE_DRAG_EVENT    0x4
#define NCMICE_ALL_EVENTS    0x7
```

**bool nckey_mouse_p(uint32_t ***r***);**

**bool ncinput_nomod_p(const ncinput* ***ni***);**

**uint32_t notcurses_get(struct notcurses* ***n***, const struct timespec* ***ts***, ncinput* ***ni***);**

**int notcurses_getvec(struct notcurses* ***n***, const struct timespec* ***ts***, ncinput* ***ni***, int vcount);**

**uint32_t notcurses_getc_nblock(struct notcurses* ***n***, ncinput* ***ni***);**

**uint32_t notcurses_getc_blocking(struct notcurses* ***n***, ncinput* ***ni***);**

**int notcurses_mice_enable(struct notcurses* ***n***, unsigned ***eventmask***);**

**int notcurses_mice_disable(struct notcurses* ***n***);**

**int notcurses_inputready_fd(struct notcurses* ***n***);**

**static inline bool ncinput_equal_p(const ncinput* ***n1***, const ncinput* ***n2***);**

**int notcurses_linesigs_disable(struct notcurses* ***n***);**

**int notcurses_linesigs_enable(struct notcurses* ***n***);**

# DESCRIPTION

notcurses supports input from keyboards and mice, and any device that looks
like them. Mouse support requires a broker such as GPM, Wayland, or Xorg, and
must be explicitly enabled via **notcurses_mouse_enable**. The full 32-bit
range of Unicode is supported (see **unicode(7)**), with synthesized events
mapped into the [Supplementary Private Use Area-B](https://unicode.org/charts/PDF/U1.0.10.pdf).
Unicode characters are returned directly as UCS-32, one codepoint at a time.

notcurses takes its keyboard input from **stdin**, which will be placed into
non-blocking mode for the duration of operation. The terminal is put into
non-canonical mode (see **termios(3)**), and thus keys are received without line-buffering.
notcurses maintains its own buffer of input characters, which it will attempt
to fill whenever it reads.

**notcurses_get** allows a **struct timespec** to be specified as a timeout.
If **ts** is **NULL**, **notcurses_get** will block until it reads input, or
is interrupted by a signal. If its values are zeroes, there will be no
blocking. Otherwise, **ts** specifies an absolute deadline (taken against
**CLOCK_MONOTONIC**; see **clock_gettime(2)**). On timeout, 0 is returned.
Event details will be reported in **ni**, unless **ni** is NULL.

**notcurses_inputready_fd** provides a file descriptor suitable for use with
I/O multiplexors such as **poll(2)**. This file descriptor might or might not
be the actual input file descriptor. If it readable, **notcurses_get** can
be called without the possibility of blocking.

**ncinput_equal_p** compares two **ncinput** structs for data equality (i.e.
not considering padding), returning **true** if they represent the same
input (though not necessarily the same input event).

**notcurses_linesigs_disable** disables conversion of inputs **INTR**, **QUIT**,
**SUSP**, and **DSUSP** into **SIGINT**, **SIGQUIT**, and **SIGTSTP**. These
conversions are enabled by default. **notcurses_linesigs_enable** undoes this
action, but signals in the interim are permanently lost.

## Mice

For mouse events, the additional fields ***y***, ***x***, ***ypx***, and
***xpx*** are set. These fields are not meaningful for keypress events.
Mouse events can be distinguished using the **nckey_mouse_p** predicate.
**NCMICE_MOVE_EVENT** requests events whenever the mouse moves when no
buttons are held down. **NCMICE_DRAG_EVENT** requests events when the mouse
is moving with buttons held down. **NCMICE_BUTTON_EVENT** requests events
then the button state changes. **NCMICE_ALL_EVENTS** is provided for
convenience and future-proofing against API (though not ABI) changes.

## Synthesized keypresses

Many keys do not have a Unicode representation, let alone ASCII. Examples
include the modifier keys (Alt, Meta, etc.), the "function" keys, and the arrow
keys on the numeric keypad. The special keys available to the terminal are
defined in the **terminfo(5)** entry, which notcurses loads on startup. Upon
receiving an escape code matching a terminfo input capability, notcurses
synthesizes a special value. An escape sequence must arrive in its entirety to
notcurses; running out of input in the middle of an escape sequence will see it
rejected. Likewise, any error while handling an escape sequence will see the
lex aborted, and the sequence thus far played back as independent literal
keystrokes.

The full list of synthesized keys (there are well over one hundred) can be
found in **<notcurses/notcurses.h>**. For more details, consult **terminfo(5)**.

## **NCKEY_RESIZE**

Unless the **SIGWINCH** handler has been inhibited (see **notcurses_init**),
notcurses will automatically catch screen resizes, and synthesize an
**NCKEY_RESIZE** event. Upon receiving this event, the user may call
**notcurses_refresh** to force an immediate reflow, or just wait until the
next call to **notcurses_render**, when notcurses will pick up the resize
itself. If the **SIGWINCH** handler is inhibited, **NCKEY_RESIZE** is never
generated.

## **NCKEY_EOF**

Upon reaching the end of input, **NCKEY_EOF** will be returned. At this point,
any further calls will immediately return **NCKEY_EOF**. Note that this does
not necessarily result from pressing e.g. Ctrl+D.

# RETURN VALUES

On error, the **get** family of functions return **(uint32_t)-1**. The cause
of the error may be determined using **errno(3)**. Unless the error was a
temporary one (especially e.g. **EINTR**), **notcurses_get** probably cannot
be usefully called forthwith. On a timeout, 0 is returned. Otherwise, the
UCS-32 value of a Unicode codepoint, or a synthesized event, is returned.

If an error is encountered before **notcurses_getvec** has read any input,
it will return -1. If it times out before reading any input, it will return
0. Otherwise, it returns the number of **ncinput** objects written back.

**notcurses_mice_enable** returns 0 on success, and non-zero on failure, as
does **notcurses_mice_disable**. Success does not necessarily mean that a
mouse is available nor that all requested events will be generated.

**ncinput_equal_p** returns **true** if the two **ncinput** structs represent
the same input (though not necessarily the same input event), and
**false** otherwise.

# NOTES

Like any other notcurses function, it is an error to call **notcurses_get**
during or after a call to **notcurses_stop**. If a thread is always sitting
on blocking input, it can be tricky to guarantee that this doesn't happen.

Only one thread may call into the input stack at once, but unlike almost every
other function in notcurses, **notcurses_get** and friends can be called
concurrently with **notcurses_render**.

Do not simply **poll** the file descriptor associated with **stdin** to test
for input readiness. Instead, use the file descriptor returned by
**notcurses_inputready_fd** to ensure compatibility with future versions of
Notcurses (it is possible that future versions will process input in their own
contexts).

When support is detected, the Kitty keyboard disambiguation protocol will be
requested. This eliminates most of the **BUGS** mentioned below.

# BUGS

The Shift key is not indicated in conjunction with typical Unicode text.
If e.g. Shift is used to generate a capital letter 'A', ***id*** will equal 'A',
and ***shift*** will be **false**. Similarly, when Ctrl is pressed along with a
letter, the letter will currently always be reported in its uppercase form.
E.g., if Shift, Ctrl, and 'a' are all pressed, this is indistinguishable from
Ctrl and 'A'.

Ctrl pressed along with 'J' or 'M', whether Shift is pressed or not,
currently registers as **NCKEY_ENTER**. This will likely change in the
future.

When the Kitty keyboard disambiguation protocol is used, most of these
issues are resolved. You can determine whether the protocol is in use
by examining the output of **notcurses-info(1)**. If the **kbd** property
is indicated, you're using the Kitty protocol.

Mouse events in the top and left margins will never be delivered to the
application (as is intended), but mouse events in the bottom and right margins
sometimes can be if the event occurs prior to a window resize.

The ***ypx*** and ***xpx*** fields are never currently valid (i.e. they are
always -1). This ought be fixed in the future using the SGR PixelMode mouse
protocol.

On some operating systems, **CLOCK_REALTIME** is used as the basis for
timeouts instead of **CLOCK_MONOTONIC**. This ought be fixed.

# SEE ALSO

**notcurses-info(1)**,
**clock_gettime(2)**,
**poll(2)**,
**notcurses(3)**,
**notcurses_refresh(3)**,
**notcurses_render(3)**,
**termios(3)**,
**terminfo(5)**,
**ascii(7)**,
**signal(7)**,
**unicode(7)**
