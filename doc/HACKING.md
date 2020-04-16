# Hacking!

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
