% notcurses_ncvisual(3)
% nick black <nickblack@linux.com>
% v1.3.2

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

**struct ncvisual* ncvisual_open_plane(struct notcurses* nc, const char* file,
                                         nc_err_e* err, int y, int x,
                                         ncscale_e style);**

**void ncvisual_destroy(struct ncvisual* ncv);**

**nc_err_e ncvisual_decode(struct ncvisual* nc);**

**int ncvisual_render(const struct ncvisual* ncv, int begy, int begx,
                        int leny, int lenx);**

**int ncvisual_simple_streamer(struct notcurses* nc, struct ncvisual* ncv, void* curry);**

**int ncvisual_stream(struct notcurses* nc, struct ncvisual* ncv, nc_err_e* err,
                      float timescale, streamcb streamer, void* curry);**

**struct ncplane* ncvisual_plane(struct ncvisual* ncv);**

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

# RETURN VALUES

**notcurses_canopen** returns true if this functionality is enabled, or false
if Notcurses was not built with FFmpeg support. **ncplane_visual_open** and
**ncvisual_open_plane** return an **ncvisual** object on success, or **NULL**
on failure. Success from these functions indicates that the specified **file**
was opened, and enough data was read to make a firm codec identification. It
does not mean that the entire file is properly-formed. On failure, **err**
will be updated. **ncvisual_decode** returns **NCERR_SUCCESS** on success, or
**NCERR_EOF** on end of file, or some other **nc_err_e** on failure. It
likewise updates **err** in the event of an error. **ncvisual_render** returns
the number of cells emitted, or -1 on error.

# NOTES

Multimedia decoding requires that Notcurses be built with either FFmpeg or
OpenImageIO support. What formats can be decoded is totally dependent on the
linked library.

# SEE ALSO

**notcurses(3)**, **notcurses_ncplane(3)**
