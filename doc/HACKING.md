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
*postpaint* corrects for `CELL_ALPHA_HIGHCONTRAST`, performs damage detection,
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
