# Hacking!

## notcurses vs notcurses-core

I wanted to achieve three things:

* Administrators decide whether they want multimedia support installed.
* Clients decide whether they want to use multimedia, and write one program.
* No dlopen(3) or weak symbols -- they're unportable, and break static linking.

If the administrator doesn't want multimedia support installed, they can
refrain from installing the notcurses library built with it. Building with
`USE_MULTIMEDIA=none` results in a shim notcurses. This notcurses allows
programs that want multimedia to still link; attempting to actually use
`notcurses_from_file()` will result in an error, and the client application
can test ahead of time with e.g. `notcurses_canopen_images()`.

### Packaging

The ideal packaging IMHO involves two builds, one with `USE_MULTIMEDIA` set
to either `ffmpeg` or `oiio` (`ffmpeg` is preferred to `oiio`), and one with
`USE_MULTIMEDIA=none`. These ought result in equivalent notcurses-core
objects, but two different notcurses objects. Package notcurses-core into
its own package, which recommends or even depends on either of the notcurses
packages. Name the notcurses packages, say, `libnotcurses-ffmpeg` and
`libnotcurses-nomedia`, have them conflict with one another, and have both
depend on notcurses-core. Defining a virtual package `libnotcurses`, provided
by either of the `libnotcurses-*` packages, is desirable if supported.

## Rows

There are four kinds of `y`s: physical, rational, logical, and virtual. Physical
and rational `y`s are independent of any particular plane. A physical `y`
refers to a particular row of the terminal. A rational `y` refers to a particular
row of the rendering area. They are related by:

* physical `y` - margin `top` == rational `y`
* rational `y` + margin `top` == physical `y`

In the absence of a `top` margin, physical `y` == rational `y`.

Logical and virtual `y`s are relative to a plane (possibly the standard plane).
A logical `y` refers to a row of a plane, independent of scrolling. A virtual
`y` refers to a row-sized chunk of the plane's framebuffer, which might be
mapped to any row within the plane. They are related by:

* (logical `y` + plane `logrow`) % plane `leny` == virtual `y`
* (virtual `y` + plane `leny` - plane `logrow`) % plane `leny` == logical `y`

All API points expressing a `y`, whether writing it (e.g. `ncplane_cursor_yx()`)
or reading it (e.g. `ncplane_cursor_move_yx()`), are working with a logical `y`.
The `y` member of an `ncplane` is also a logical `y`.

Whenever we initiate a write past the end of the line, and the virtual `y` is
equal to `ncplane->lenx - 1`, we must scroll. Scrolling:

* plane `logrow` = (plane `logrow` + 1) % plane `leny`

As a result, logical `y` is unchanged, but virtual `y` has advanced.

Virtual `y` is useful for only two things:

* Determining whether to scroll, and
* Indexing into the plane's framebuffer

Thus we usually keep `y` logical.

## Right-to-left text

We want to fully support Unicode and international text. But what does it mean
to use right-to-left text with a fullscreen, random-access application? In
particular, what happens in the case where we've written the right-to-left
string SHRDLU (which ought appear as ULDRHS) to a plane, starting at (0, 0),
and then we place say a U+1F982 SCORPION (🦂) at (0, 2)? Ought this yield
UL🦂HS, or ought it instead yield HS🦂UL? If the original string had been
SH🦂LU, it would have been displayed by most terminals as HS🦂UL, due to
treating it as a right-to-left segment, a left-to-right segment, and finally a
right-to-left segment. Alternatively, it might have been displayed as UL🦂HS,
especially if aligned on the right. It's difficult to know. So, we instead
force text direction by appending U+200E LEFT-TO-RIGHT MARK to any EGCs we
believe to provoke right-to-left. The user is thus solely responsible for
managing right-to-left presentation.

I hate everything about this terrible, fragile, wasteful "solution".

## Rendering/rasterizing/writeout, and resizing

The scope of rendering is a pile. The scope of rasterization is a pile, the
last frame, and the screen. These latter two are shared, and thus concurrent
rasterizations are illegal and an error. Concurrent rendering of different
piles is explicitly supported.

In Notcurses prior to 2.1.0, there was only one pile. Rendering and rasterizing
were a single function, `notcurses_render()`. Since this proceeded end-to-end,
and didn't need worry about concurrency, it could perform an optimal strategy:

* Check for a resize, resizing the last frame and standard plane if appropriate
* Render the (single) pile, taking full advantage of an enlarged terminal
* Rasterize the (single) render, carrying through plenty of state from render
* Write out the (single) rasterization

It is an ineluctable fact that we cannot guarantee proper writeout, since the
terminal can be resized in the middle of a writeout, and the signal is
both unreliable and asynchronous. Receipt of the SIGWINCH signal is async with
regards to the actual geometry change; processing of the signal is async with
regards to its delivery. Even if this was all synchronous, signals are
fundamentally unreliable, and can be missed. Internalize and accept this.

If we write more data than the terminal has geometry (either with regards to
rows or columns), we will produce some garbage. If we write less, we'll simply
fail to fill up the screen (so long as we explicitly move to new rows, which we
do). Both are undesirable, but neither is catastrophic.

Writeout is a blocking process. We do not support non-blocking writeout at this
time. An error at any point while writing out the frame will abort the writeout
and be considered a failure. Writeout takes a buffer, a buffer length, and an
output descriptor; it attempts to write until the buffer has been written in its
entirety. The buffer might only partially update the screen, due to damage
detection (undamaged cells are never placed into the buffer); the buffer is thus
relative to our concept of the current state of the terminal (the "last frame").
The "last frame" is updated in rasterization, as the buffer is generated. It is
thus critical that rasterized frames be written out in order. Writeout is thus
bound to rasterization, except special cases that always rasterize total frames:

* `notcurses_refresh()` (writes last frame to terminal following clear screen)
* `notcurses_rasterize_to_buffer()` (copies last frame to buffer)
* `notcurses_rasterize_to_file()` (appends last frame to file)

Rasterization always results in at least one writeout. Henceforth, we will
consider only rendering and rasterizing, the latter with an implicit writeout.

The output of rendering is fed into rasterization. Especially given multiple
piles, it is possible that another render will take place between rendering
and rasterizing of a given pile (this can happen with even a single pile,
though, now that rendering and rasterizing are decoupled). It is thus
necessary that rendering never refer to the "last written frame", since that
last written frame might change by the time the render is written out.
Similarly, the rasterizer may not assume that the size of the render it is
given is equal to the current conception of the screen size.

The last frame and standard frame are resized in `notcurses_resize()` to match
the recovered terminal geometry. `notcurses_resize()` acquires the geometry via
an `ioctl()`, and resizes these framebuffers, zero-initializing any new area.
Since it's possible that the terminal was resized without our receipt of a
signal, we want to call it in somewhere in the render/resize cycle.

It is undesirable to call `notcurses_resize()` in the multiple render path,
since this would need internal locking to deal with concurrent renders. It *is*
desirable to call `notcurses_resize()` prior to rendering, since otherwise we
might not render portions of the pile only just made visible (in the case of
the terminal being enlarged). It *is not* desirable to call `notcurses_resize()`
prior to rendering, since if the terminal shrinks *following* the render but
*before* the raster, we'd like to know that and thus avoid overwriting.
Remember from above that an underwrite is less damaging than an overwrite. We
thus perform `notcurses_resize()` in the rasterization path. The upshot is that
a rendered frame can be larger or smaller than the screen at the time we
rasterize--but since this could happen anyway, it's no great loss.

*EXCEPT* for one case: imagine that we have a single plane, 1000x1000, that is
all green. Our program starts at 80x24, renders, rasterizes, and enters an
input loop. It performs another render+raster for each input (remember, a
SIGWINCH manifests as `NCKEY_RESIZE`). The terminal is then resized to 100x100.
The following happens:

* initial render renders an 80x24 frame
* initial raster writes out this 80x24 frame, screen is green
* block on input
* terminal is resized to 100x100
* `NCKEY_RESIZE` is read
* second render renders an 80x24 frame
* second raster learns of 100x100 size, writes out 80x24 in upper left
* block on input, screen is partially green and partially background

At the end of our second writeout, we have an incomplete screen, despite the
geometry change happening well before (and indeed triggering) our second cycle.
We do not simply move rendering into the top of rasterization, since resizes
are presumably rare, and we want to facilitate maximum parallelism, which we
can't do if rendering is part of a serial section).

Actually, this suggests (and I then confirmed) that this means the top half
itself is using the screen geometry, and thus already accessing shared data.
So a mutex is happening there no matter what.

By the time we rasterize, we thus have three different geometries in play:

* the most recently-acquired actual screen geometry (as reported by `ioctl()`),
* the geometry of the supplied render (as determined at render time), and
* the geometry of the last-rendered frame.

Rasterization, remember, is a function of the supplied render, the last frame,
and the output geometry--all three of these distinct geometries. So long as
there is no resize step between rasterizing and writing, writing deals with
the same geometries as rasterization, so we ignore it.

Rasterization can be split into two virtual phases: *postpaint* and *rastering*.
*postpaint* corrects for `NCALPHA_HIGHCONTRAST`, performs damage detection,
and copies any necessary EGCs from their source pools to the common pool
(copying these EGCs is why a pile cannot be modified between rendering and
rasterizing--such modifications might invalidate the EGC references). The
*rasterizing* phase takes this final rendered plane, pool, damage map, and the
current rendering state (e.g. cursor position, last style+color), and generates
a buffer. At this point, the last frame is updated, and a new rasterization
could technically begin. It is probably possible to unite the two phases, though
this has not been done, and might never be.

So, rasterization must:

* use the rendered frame's geometry to create a damage map
* iterate over each cell of the rendered frame (postpaint)
  * if the cell was present in the last frame, check for damage
  * if the cell was not present in the last frame, assume damage
* iterate over each cell of the visual area (rasterization)
  * if the cell was present in the damage map, check for damage
    * if there was damage, emit the data (plus a move if applicable)
    * if there was not damage, skip the cell
  * if the cell was not present in the damage map, skip the cell

We skip the cell if it was not present in the damage map because an enlarged
terminal is filled with default cells, which is all we could generate in any
case, having not rendered the cell. This implies that the damage map must be
two-dimensional, as must the render. Only the rasterized buffer is flattened to
a single dimension.

Given our requirement that a pile not be mutated between render and raster, we
know that at render time the pile is suitable for rendering. We *could* thus
check to see if the screen has grown relative to the render, and call for a
fresh render. This would be a great solution for our 1000x1000 case above, but
it doesn't help when the user has only been generating enough output for the
visible area. In this case, new data will not be available should raster call
for a new render; it is instead necessary that the "userspace" resize actions
be taken.

This raises a new issue: given cascading resize callbacks, `notcurses_resize()`
can result in arbitrary changes to the pile. This suggests that the resize
operation cannot occur between render and raster...

### Alternatives to the Painter's Algorithm

The rendering area is RY * RX, where RY and RX are positive integers.

A plane is either active or inactive for a given cell in the rendering area.
The plane is active if it is defined at that cell. It is inactive otherwise.

There is an initial (possibly empty) inactive region before the plane is first
reached. There then follow `A' (A' >= 0)` active regions, separated by
`(I' = A'-1)` inactive regions (`I'` is 0 if `A'` is 0). These active regions `A_0, A_1, ...`
all have the same size, and these inactive regions `I_0, I_1, ...`
likewise all have the same size. I_0 + A_0 == RX. There is then a final
(possibly empty) inactive region following the plane's lowermost, rightmost
intersection with the visual area.

 `I_init + A' * A_0 + I' * I_0 + I_final == RX * RY.`

Given RX and RY, we can describe a plane's activity pattern completely with
three numbers: `I_init`, `A'`, and `A_0`.

Keep two ordered structures, an active set and an inactive set. The active set
is counting down until they become inactive. The inactive set is counting down
until the become active.

Initialization:

For each plane, calculate `I_init` and `A_0`. Planes with `I_init` values of 0 go
into the active set, sorted first by `A_0` and secondarily by plane depth. Planes
with `I_init values >= 0` go into the inactive set, sorted first by `I_init` and
secondarily by plane depth.

For rendering area RY * RX and plane py * px at offset y, x, `I_init` is:

```
 infinite for x >= RX
 infinite for x + px <= 0
 infinite for y >= RY
 infinite for y + py <= 0
 0 for y <= 0, y + py >= 0, x <= 0
 x for y <= 0, y + py >= 0, x > 0
 y * RX + x for y > 0, x >= 0
 y * RX for y > 0
```

max finite initial gap is RY * RX - 1. min initial gap is 0.

Each node is a pointer to a plane, and the scalar coordinate `xy (0 <= xy < PX * PY)`
at which the current state changes (`A_0` and `I_init`).

assuming finite initial gap (i.e. that the plane overlaps the rendering area),
the active length (can exceed practical length) is:

```
 x <= 0:
  x + px >= RX: (spans horizontal range)
   y <= 0:
     RX * py + y, from origin
   y > 0:
     RX * py, from column 0
  x + px < RX:
    x + px,
 x > 0:
  x + px >= RX:
    RX - x
  x + px < RX:
    px
```

max active length is RY * RX (for a plane covering the entirety of the
horizontal viewing area), otherwise RX - 1. min active length is 1.

inactive gap is undefined if plane spans visual region or is invisible.
otherwise, inactive gap is calculated at right edge of plane (column C),
and is equal to PX - (C + 1) + x if x >= 0, or PX - (C + 1) otherwise.

at each step we check to see if the foremost planes of either set need flip
to the other set. this suggests an extra sort per flip. unless we've eclipsed
a plane's `I_init`, or entered a plane's `I_final`, an element moving from one set
to another must have the same previous element as it did before. each node
thus keeps an additional element, a double pointer to the previous element's
next link. upon flip, check this pointer to ensure it's NULL. if it is NULL,
link ourselves. otherwise, chase to the end, and link ourselves.

ANALYSIS

There's a sort at the beginning of O(PlgP) on P planes. We then check
P * PX * PY cells. In the worst case, where all cells actually need be used,
our new algorithm is worse by the cost of a sort.

## Ncvisuals

An `ncvisual` is blitter-independent, and may be used with multiple blitters.
Its `data` field holds RGBA pixels as provided from disk or memory. Its `pixx`,
`pixy`, and `rowstride` fields describe this bitmap. There are `pixy` rows of
`rowstride` bytes, each containing `pixx` RGBA pixels at the front, plus any
necessary padding (external libraries might generate padded output).

`ncvisual_blit` works with at least four geometries:
* `vopts->begy`/`begx`: offsets into unscaled data (pixels)
* `vopts->leny`/`lenx`: lengths of unscaled data to use (pixels)
  * These geometries, when summed, must not exceed `ncv->pixy`/`ncv->pixx`.
  * They are usable as input to scaling.
* `inputy`/`inputx`: Derived: `leny` - `begy` + 1 and `lenx` - `begx` + 1
* `scaledy`/`scaledx`: size of scaled output, derived from target plane and
  scaling type (pixels), usable as input for blitting.
* `outputy`/`outputx`: size of blitted output (pixels)
* `occy`/`occx`: size of blitted output (cells)

`occy` and `occx` may represent a larger area than `outputy` and `outputx`,
since a blit might not occupy the entirety of the cells with which it
interacts. likewise, `outputy` might represent a taller area than `scaledy`,
due to Sixel requirements. `outputx` will currently always equal `scaledx`.
the relationship of `inputy`/`inputx` to `scaledy`/`scaledx` is as follows:

* `NCSCALE_NONE`: equal
* `NCSCALE_SCALE`: `scaledy` = `inputy` * *F*, `scaledx` = `inputx` * *F*, where
  *F* is a float, and at least one of `outputy` and `outputx` maximize the
  space within the target plane relative to mandatory scaling.
* `NCSCALE_STRETCH`: no necessary relation. Both `outputy` and `outputx`
  maximize the space within the target plane relative to mandatory scaling.

"Mandatory scaling" is operative only with regards to Sixel, which must always
be a multiple of six pixels tall.

### Bitmaps

`NCBLIT_PIXEL` yields a bitmap. A bitmap

* occupies the entirety of its plane, by resizing if necessary
* always starts at the origin of its plane
* admits no other output to its plane, nor resizing
* greatly complicates rendering

## Input

Input is greatly complicated by rare but critical in-band signaling from the
terminal itself. This is the method by which, for instance, terminals
advertising Sixel indicate how many color registers they support. We must
ensure such responses never reach the user, and that we act on them quickly.
Such replies are generally distinguished by a (literal) escape. Unfortunately,
the user can (and often does) generate ESC themselves.

The primary instance of this signaling is on startup, when we query the
terminal as part of capability discovery. Until we process the reply, we
don't know what capabilities the terminal offers, particularly with regard
to bitmap graphics.

We have two potential input sources, both of which *might* correspond to
`stdin`. If we were spawned attached to the terminal, we receive both user and
terminal input on the same fd (corresponding to `stdin`). If our input was
redirected from somewhere else, we need open the controlling terminal, and
read from it. This has the happy side-effect of isolating the control plane
from the data plane (though you mustn't rely that this will make control
communication unforgeable; the user can likely write to the controlling
terminal themselves).

If a terminal doesn't understand or implement some query, there will typically
be no response. If a negative response is required, follow up the query (or
queries) with a Device Attributes (DA, `\e[c`) query, to which all known terminals
will respond. So long as a valid response cannot be confused with a response to
DA, this serves as a negative acknowledgement. Relying on this, at startup we
fire off two `XTSMGRAPHICS` queries followed by a DA query, all as one write. We
don't sit around waiting for the response, but instead continue initialization.
Ideally, by the time we're done and need the info, it's ready for us to read.

Some inputs intended for the user are transmitted to us as escapes, however.
Any of the synthesized characters (including e.g. Home, function keys, arrows)
arrive as escapes, which we convert to codepoints in the Private Use Area.
These need be delivered to the user.

There are no asynchronous control messages that we need watch for (the closest
thing is `SIGWINCH` on geometry changes), so we don't generally need to watch
the input. We *do* need to extract any control messages that arrive while the
user is reading input (when `stdin` is connected to the tty, anyway).
Similarly, were we reading, we'd need put aside any input intended for the
user. We thus keep two queues at all times: received control messages, and
received user input. The received user input is non-segmented UTF-8 (i.e.
translated from control sequences). The received control information is stored
as distinct multibyte escape sequences.

## Windows

We support only the [ConPTY](https://devblogs.microsoft.com/commandline/windows-command-line-introducing-the-windows-pseudo-console-conpty/)
aka the Windows Pseudo Console, introduced in Windows 10. We require that the
environment is already attached to a ConPTY (i.e. we don't create an instance
with `CreatePseudoConsole()`. With this, most of the terminal I/O is portable.
We don't have termios at our disposal, using instead `GetConsoleBufferInfo()`.
ConPTY implements cursor location requests via `u7`.

ConHost/ConPTY do not pass input directly through to the end terminal, instead
effectively handing it rendered surfaces. This means that queries are answered
by ConPTY, and thus that it's impossible to do end-terminal identification via
queries. It barely matters, since almost all interaction is with ConPTY
anyway (i.e. it is probably not possible for a ConPTY terminal to support
bitmap graphics at this time).

We only use the UCRT runtime, as this seems to be the only one with sane UTF8
support. Getting UTF8 on Windows is annoyingly complicated. There is no `LANG`
environment variable in the UNIX sense. It is necessary to explicitly call
`setlocale(LC_ALL, ".UTF8")`, even if `nc_langinfo(LC_ENCODING)` returns
"UTF-8". `SetConsoleOutputCP(CP_UTF8)` also seems advised. Code page 65001 is
UTF-8.
