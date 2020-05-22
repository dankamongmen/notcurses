% notcurses_ncvisual(3)
% nick black <nickblack@linux.com>
% v1.4.3

# NAME
notcurses_ncvisual - notcurses multimedia

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
typedef enum {
  NCSCALE_NONE,
  NCSCALE_SCALE,
  NCSCALE_STRETCH,
} ncscale_e;

typedef int (*streamcb)(struct notcurses*, struct ncvisual*, void*);
```

**bool notcurses_canopen(const struct notcurses* nc);**

**struct ncvisual* ncplane_visual_open(struct ncplane* nc, const char* file,
                                         nc_err_e* err);**

**struct ncvisual* ncvisual_from_file(struct notcurses* nc, const char* file,
                                         nc_err_e* err, int y, int x,
                                         ncscale_e style);**

**struct ncvisual* ncvisual_from_rgba(struct notcurses* nc, const void* rgba, int rows, int rowstride, int cols);**

**struct ncvisual* ncvisual_from_bgra(struct notcurses* nc, const void* bgra, int rows, int rowstride, int cols);**

**struct ncvisual* ncvisual_from_plane(struct ncplane* n);**

**void ncvisual_destroy(struct ncvisual* ncv);**

**nc_err_e ncvisual_decode(struct ncvisual* nc);**

**int ncvisual_render(const struct ncvisual* ncv, int begy, int begx,
                        int leny, int lenx);**

**int ncvisual_simple_streamer(struct notcurses* nc, struct ncvisual* ncv, void* curry);**

**int ncvisual_stream(struct notcurses* nc, struct ncvisual* ncv, nc_err_e* err,
                      float timescale, streamcb streamer, void* curry);**

**struct ncplane* ncvisual_plane(struct ncvisual* ncv);**

**int ncvisual_rotate(struct ncvisual* n, double rads);**

**char* ncvisual_subtitle(const struct ncvisual* ncv);**

# DESCRIPTION

The frame will be scaled to the size of the ncplane per the ncscale_e style.
**ncvisual_render** actually blits the decoded frame to its associated plane.
A subregion of the frame can be specified using **begx**, **begy**, **lenx**,
and **leny**. To render the rectangle having its origin at **begy**, **begx**
and the lower-right corner, -1 can be supplied as **leny** and **lenx**.
{0, 0, -1, -1} will thus render the entire visual. Negative values for **begy**
or **begx** are an error. It is an error to specify any region beyond the
boundaries of the frame. Supplying zero for either **leny** or **lenx** will
result in a zero-area rendering.

It is possible to create an **ncvisual** from memory, rather than from a
disk, but only using raw RGBA/BGRA 8bpc content. For RGBA, use
**ncvisual_from_rgba**. For BGRA, use **ncvisual_from_bgra**. Both require
a number of **rows**, a number of image columns **cols**, and a virtual row
length of **rowstride** / 4 columns. The input data must provide 32 bits per
pixel, and thus must be at least **rowstride** * **rows** bytes, of
which a **cols** * **rows** * 4-byte subset is used. It is not possible to
**mmap(2)** an image file and use it directly--decompressed, decoded data
is necessary. The resulting plane will be ceil(**rows**/2) rows, and **cols**
columns. It will not be necessary to call **ncvisual_decode**, but it is
still necessary to call **ncvisual_render**.

The contents of an **ncplane** can be "promoted" into an **ncvisual** with
**ncvisual_from_ncplane**. The existing plane will be bound and decoded to a
new **ncvisual**. Only spaces, half blocks, and full blocks may be present
in the plane.

**ncvisual_rotate** executes a rotation of **rads** radians, in the clockwise
(positive) or counterclockwise (negative) direction. If the **ncvisual** owns
(created) its underlying **ncplane**, that plane will be resized as necessary
to display the entirety of the rotated visual.

**ncvisual_subtitle** will return a UTF-8-encoded subtitle corresponding to
the current frame if such a subtitle was decoded. Note that a subtitle might
be returned for multiple frames, or might not.

# RETURN VALUES

**notcurses_canopen** returns true if this functionality is enabled, or false
if Notcurses was not built with multimedia support. **ncplane_visual_open** and
**ncvisual_from_file** return an **ncvisual** object on success, or **NULL**
on failure. Success from these functions indicates that the specified **file**
was opened, and enough data was read to make a firm codec identification. It
does not mean that the entire file is properly-formed. On failure, **err**
will be updated. **ncvisual_decode** returns **NCERR_SUCCESS** on success, or
**NCERR_EOF** on end of file, or some other **nc_err_e** on failure. It
likewise updates **err** in the event of an error. **ncvisual_render** returns
the number of cells emitted, or -1 on error.

**ncvisual_from_plane** returns **NULL** if the **ncvisual** cannot be created
and bound. This is usually due to illegal content in the source **ncplane**.

# NOTES

Multimedia decoding requires that Notcurses be built with either FFmpeg or
OpenImageIO support. What formats can be decoded is totally dependent on the
linked library. OpenImageIO does not support subtitles.

# BUGS

**ncvisual_rotate** currently supports only **M_PI**/2 and -**M_PI**/2
radians for **rads**, but this will change soon.

# SEE ALSO

**notcurses(3)**,
**notcurses_plane(3)**,
**utf-8(7)**
