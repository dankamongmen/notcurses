% notcurses_visual(3)
% nick black <nickblack@linux.com>
% v2.0.0

# NAME
notcurses_visual - notcurses multimedia

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
typedef enum {
  NCSCALE_NONE,
  NCSCALE_SCALE,
  NCSCALE_STRETCH,
} ncscale_e;

typedef enum {
  NCBLIT_DEFAULT, // let the ncvisual pick
  NCBLIT_1x1,     // full block                █
  NCBLIT_2x1,     // upper half + 1x1          ▀█
  NCBLIT_1x1x4,   // shaded full blocks        ▓▒░█
  NCBLIT_2x2,     // quadrants + 2x1           ▗▐ ▖▀▟▌▙█
  NCBLIT_4x1,     // four vertical levels      █▆▄▂
  NCBLIT_BRAILLE, // 4 rows, 2 cols (braille)  ⡀⡄⡆⡇⢀⣀⣄⣆⣇⢠⣠⣤⣦⣧⢰⣰⣴⣶⣷⢸⣸⣼⣾⣿
  NCBLIT_8x1,     // eight vertical levels     █▇▆▅▄▃▂▁
  NCBLIT_SIXEL,   // 6 rows, 1 col (RGB), spotty support among terminals
} ncblitter_e;

#define NCVISUAL_OPTION_NODEGRADE 0x0001
#define NCVISUAL_OPTION_BLEND     0x0002

struct ncvisual_options {
  struct ncplane* n;
  ncscale_e scaling;
  int y, x;
  int begy, begx; // origin of rendered section
  int leny, lenx; // size of rendered section
  ncblitter_e blitter; // glyph set to use (maps input to output cells)
  uint64_t flags; // bitmask over NCVISUAL_OPTION_*
};

typedef int (*streamcb)(struct notcurses*, struct ncvisual*, void*);
```

**bool notcurses_canopen_images(const struct notcurses* nc);**

**bool notcurses_canopen_videos(const struct notcurses* nc);**

**bool notcurses_cansixel(const struct notcurses* nc);**

**struct ncvisual* ncvisual_from_file(const char* file);**

**struct ncvisual* ncvisual_from_rgba(const void* rgba, int rows, int rowstride, int cols);**

**struct ncvisual* ncvisual_from_bgra(const void* bgra, int rows, int rowstride, int cols);**

**struct ncvisual* ncvisual_from_plane(struct ncplane* n, ncblitter_e blit, int begy, int begx, int leny, int lenx);**

**int ncvisual_geom(const struct notcurses* nc, const struct ncvisual* n, const struct ncvisual_options* vopts, int* y, int* x, int* toy, int* tox);**

**void ncvisual_destroy(struct ncvisual* ncv);**

**int ncvisual_decode(struct ncvisual* nc);**

**struct ncplane* ncvisual_render(struct notcurses* nc, struct ncvisual* ncv, const struct ncvisual_options* vopts);**

**int ncvisual_simple_streamer(struct ncplane* n, struct ncvisual* ncv, const struct timespec* disptime, void* curry);**

**int ncvisual_stream(struct notcurses* nc, struct ncvisual* ncv, float timescale, streamcb streamer, const struct ncvisual_options* vopts, void* curry);**

**int ncvisual_rotate(struct ncvisual* n, double rads);**

**int ncvisual_resize(struct ncvisual* n, int rows, int cols);**

**int ncvisual_polyfill_yx(struct ncvisual* n, int y, int x, uint32_t rgba);**

**int ncvisual_at_yx(const struct ncvisual* n, int y, int x, uint32_t* pixel);**

**int ncvisual_set_yx(const struct ncvisual* n, int y, int x, uint32_t pixel);**

**char* ncvisual_subtitle(const struct ncvisual* ncv);**

**int notcurses_lex_scalemode(const char* op, ncscale_e* scalemode);**

**const char* notcurses_str_scalemode(ncscale_e scalemode);**

**int notcurses_lex_blitter(const char* op, ncblitter_e* blitter);**

**const char* notcurses_str_blitter(ncblitter_e blitter);**

**ncblitter_e ncvisual_default_blitter(bool utf8, ncscale_e scaling);**

# DESCRIPTION

An **ncvisual** is a virtual pixel framebuffer. They can be created from
RGBA/BGRA data in memory (**ncvisual_from_rgba**/**ncvisual_from_bgra**),
or from the content of a suitable **ncplane** (**ncvisual_from_ncplane**).
If Notcurses was built against a multimedia engine (FFMpeg or OpenImageIO),
image and video files can be loaded into visuals using
**ncvisual_from_file**. **ncvisual_from_file** discovers the container
and codecs, but does not verify that the entire file is well-formed.
**ncvisual_decode** ought be invoked to recover subsequent frames, once
per frame.

Once the visual is loaded, it can be transformed using **ncvisual_rotate**
and **ncvisual_resize**. These are persistent operations, unlike any scaling
that takes place at render time. If a subtitle is associated with the frame,
it can be acquired with **ncvisual_subtitle**.

**ncvisual_from_rgba** and **ncvisual_from_bgra** both require a number of
**rows**, a number of image columns **cols**, and a virtual row length of
**rowstride** / 4 columns. The input data must provide 32 bits per pixel, and
thus must be at least **rowstride** * **rows** bytes, of which a **cols** *
**rows** * 4-byte subset is used. It is not possible to **mmap(2)** an image
file and use it directly--decompressed, decoded data is necessary. The
resulting plane will be ceil(**rows**/2) rows, and **cols** columns.
**ncvisual_from_plane** requires specification of a rectangle via **begy**,
**begx**, **leny**, and **lenx**. The only valid characters within this
region are those used by the **NCBLIT_2x2** blitter, though this may change
in the future.

**ncvisual_rotate** executes a rotation of **rads** radians, in the clockwise
(positive) or counterclockwise (negative) direction.

**ncvisual_subtitle** will return a UTF-8-encoded subtitle corresponding to
the current frame if such a subtitle was decoded. Note that a subtitle might
be returned for multiple frames, or might not.

**ncvisual_render** blits the visual to an **ncplane**, based on the contents
of its **struct ncvisual_options**. If **n** is not **NULL**, it specifies the
plane on which to render, and **y**/**x** specify a location within that plane.
Otherwise, a new plane will be created, and placed at **y**/**x** relative to
the rendering area. **begy**/**begx** specify the upper left corner of a
subsection of the **ncvisual** to render, while **leny**/**lenx** specify the
geometry of same. **flags** is a bitfield over:

* **NCVISUAL_OPTION_NODEGRADE** If the specified blitter is not available, fail rather than degrading.
* **NCVISUAL_OPTION_BLEND**: Render with **CELL_ALPHA_BLEND**.

# BLITTERS

The different **ncblitter_e** values select from among available glyph sets:

* **NCBLIT_DEFAULT**: Let the **ncvisual** choose its own blitter.
* **NCBLIT_1x1**: Full block (█) or empty glyph.
* **NCBLIT_2x1**: Adds the lower half block (▄) to **NCBLIT_1x1**.
* **NCBLIT_1x1x4**: Adds three shaded full blocks (▓▒░) to **NCBLIT_1x1**.
* **NCBLIT_2x2**: Adds left and right half blocks (▌▐) and quadrants (▖▗▟▙) to **NCBLIT_2x1**.
* **NCBLIT_4x1**: Adds ¼ and ¾ blocks (▂▆) to **NCBLIT_2x1**.
* **NCBLIT_BRAILLE**: 4 rows and 2 columns of braille (⡀⡄⡆⡇⢀⣀⣄⣆⣇⢠⣠⣤⣦⣧⢰⣰⣴⣶⣷⢸⣸⣼⣾⣿).
* **NCBLIT_8x1**: Adds ⅛, ⅜, ⅝, and ⅞ blocks (▇▅▃▁) to **NCBLIT_4x1**.
* **NCBLIT_SIXEL**: Sixel, a 6-by-1 RGB pixel arrangement.

# RETURN VALUES

**notcurses_canopen_images** and **notcurses_canopen_videos** returns true if
images and videos, respecitvely, can be decoded, or false if Notcurses was
built with insufficient multimedia support.

**ncvisual_from_file** returns an **ncvisual** object on success, or **NULL**
on failure. Success indicates that the specified **file** was opened, and
enough data was read to make a firm codec identification. It does not imply
that the entire file is properly-formed.

**ncvisual_decode** returns 0 on success, or 1 on end of file, or -1 on
failure. It is only necessary for multimedia-based visuals. It advances one
frame for each call.

**ncvisual_from_plane** returns **NULL** if the **ncvisual** cannot be created
and bound. This is usually due to illegal content in the source **ncplane**.

**ncvisual_render** returns **NULL** on error, and otherwise the plane to
which the visual was rendered. If **opts->n** is provided, this will be
**opts->n**. Otherwise, a plane will be created, perfectly sized for the
visual and the specified blitter.

**ncvisual_geom** returns non-zero if the **blitter** is invalid.

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
