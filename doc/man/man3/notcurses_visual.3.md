% notcurses_visual(3)
% nick black <nickblack@linux.com>
% v2.2.8

# NAME
notcurses_visual - notcurses multimedia

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
typedef enum {
  NCSCALE_NONE,
  NCSCALE_SCALE,
  NCSCALE_STRETCH,
  NCSCALE_NONE_HIRES,
  NCSCALE_SCALE_HIRES,
} ncscale_e;

typedef enum {
  NCBLIT_DEFAULT, // let the ncvisual pick
  NCBLIT_1x1,     // spaces only
  NCBLIT_2x1,     // halves + 1x1
  NCBLIT_2x2,     // quadrants + 2x1
  NCBLIT_3x2,     // sextants + 1x1
  NCBLIT_BRAILLE, // 4 rows, 2 cols (braille)
  NCBLIT_PIXEL,   // pixel graphics
  NCBLIT_4x1,     // four vertical levels, (plots)
  NCBLIT_8x1,     // eight vertical levels, (plots)
} ncblitter_e;

#define NCVISUAL_OPTION_NODEGRADE  0x0001
#define NCVISUAL_OPTION_BLEND      0x0002
#define NCVISUAL_OPTION_HORALIGNED 0x0004
#define NCVISUAL_OPTION_VERALIGNED 0x0008
#define NCVISUAL_OPTION_ADDALPHA   0x0010

struct ncvisual_options {
  struct ncplane* n;
  ncscale_e scaling;
  int y, x;
  int begy, begx; // origin of rendered section
  int leny, lenx; // size of rendered section
  ncblitter_e blitter; // glyph set to use
  uint64_t flags; // bitmask over NCVISUAL_OPTION_*
  uint32_t transcolor; // use this color for ADDALPHA
};

typedef int (*streamcb)(struct notcurses*, struct ncvisual*, void*);
```

**struct ncvisual* ncvisual_from_file(const char* ***file***);**

**struct ncvisual* ncvisual_from_rgba(const void* ***rgba***, int ***rows***, int ***rowstride***, int ***cols***);**

**struct ncvisual* ncvisual_from_bgra(const void* ***bgra***, int ***rows***, int ***rowstride***, int ***cols***);**

**struct ncvisual* ncvisual_from_plane(struct ncplane* ***n***, ncblitter_e ***blit***, int ***begy***, int ***begx***, int ***leny***, int ***lenx***);**

**int ncvisual_blitter_geom(const struct notcurses* ***nc***, const struct ncvisual* ***n***, const struct ncvisual_options* ***vopts***, int* ***y***, int* ***x***, int* ***scaley***, int* ***scalex***, ncblitter_e* ***blitter***);**

**void ncvisual_destroy(struct ncvisual* ***ncv***);**

**int ncvisual_decode(struct ncvisual* ***ncv***);**

**int ncvisual_decode_loop(struct ncvisual* ***ncv***);**

**struct ncplane* ncvisual_render(struct notcurses* ***nc***, struct ncvisual* ***ncv***, const struct ncvisual_options* ***vopts***);**

**int ncvisual_simple_streamer(struct ncplane* ***n***, struct ncvisual* ***ncv***, const struct timespec* ***disptime***, void* ***curry***);**

**int ncvisual_stream(struct notcurses* ***nc***, struct ncvisual* ***ncv***, float ***timescale***, streamcb ***streamer***, const struct ncvisual_options* ***vopts***, void* ***curry***);**

**int ncvisual_rotate(struct ncvisual* ***n***, double ***rads***);**

**int ncvisual_resize(struct ncvisual* ***n***, int ***rows***, int ***cols***);**

**int ncvisual_inflate(struct ncvisual* ***n***, int ***scale***);**

**int ncvisual_polyfill_yx(struct ncvisual* ***n***, int ***y***, int ***x***, uint32_t ***rgba***);**

**int ncvisual_at_yx(const struct ncvisual* ***n***, int ***y***, int ***x***, uint32_t* ***pixel***);**

**int ncvisual_set_yx(const struct ncvisual* ***n***, int ***y***, int ***x***, uint32_t ***pixel***);**

**char* ncvisual_subtitle(const struct ncvisual* ***ncv***);**

**int notcurses_lex_scalemode(const char* ***op***, ncscale_e* ***scaling***);**

**const char* notcurses_str_scalemode(ncscale_e ***scaling***);**

**int notcurses_lex_blitter(const char* ***op***, ncblitter_e* ***blitter***);**

**const char* notcurses_str_blitter(ncblitter_e ***blitter***);**

**ncblitter_e ncvisual_media_defblitter(const struct notcurses ***nc***, ncscale_e ***scaling***);**

**int ncplane_qrcode(struct ncplane* ***n***, int* ***ymax***, int* ***xmax***, const void* ***data***, size_t ***len***)**

# DESCRIPTION

An **ncvisual** is a virtual pixel framebuffer. They can be created from
RGBA/BGRA data in memory (**ncvisual_from_rgba**/**ncvisual_from_bgra**),
or from the content of a suitable **ncplane** (**ncvisual_from_ncplane**).
If Notcurses was built against a multimedia engine (FFMpeg or OpenImageIO),
image and video files can be loaded into visuals using
**ncvisual_from_file**. **ncvisual_from_file** discovers the container
and codecs, but does not verify that the entire file is well-formed.
**ncvisual_decode** ought be invoked to recover subsequent frames, once
per frame. **ncvisual_decode_loop** will return to the first frame,
as if **ncvisual_decode** had never been called.

Once the visual is loaded, it can be transformed using **ncvisual_rotate**,
**ncvisual_resize**, and **ncvisual_inflate**. These are persistent operations,
unlike any scaling that takes place at render time. If a subtitle is associated
with the frame, it can be acquired with **ncvisual_subtitle**.
**ncvisual_resize** uses the media layer's best scheme to enlarge or shrink the
original data, typically involving some interpolation. **ncvisual_inflate**
maps each pixel to ***scale***x***scale*** pixels square, retaining the
original color; it is an error if ***scale*** is less than one.

**ncvisual_from_rgba** and **ncvisual_from_bgra** both require a number of
**rows**, a number of image columns **cols**, and a virtual row length of
**rowstride** / 4 columns. The input data must provide 32 bits per pixel, and
thus must be at least **rowstride** * **rows** bytes, of which a
**cols** * **rows** * 4-byte subset is used. It is not possible to **mmap(2)** an image
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
* **NCVISUAL_OPTION_HORALIGNED**: Interpret ***x*** as an **ncalign_e**.
* **NCVISUAL_OPTION_VERALIGNED**: Interpret ***y*** as an **ncalign_e**.

**ncvisual_blitter_geom** allows the caller to determine any or all of the
visual's pixel geometry, the blitter to be used, and that blitter's scaling
in both dimensions. Any but the first argument may be **NULL**.

**ncplane_qrcode** draws an ISO/IEC 18004:2015 QR Code for the **len** bytes of
**data** using **NCBLIT_2x1** (this is the only blitter that will work with QR
Code scanners, due to its 1:1 aspect ratio).

# OPTIONS

**begy** and **begx** specify the upper left corner of the image to start
drawing. **leny** and **lenx** specify the area of the subimage drawn.
**leny** and/or **lenx** may be specified as a negative number to draw
through the bottom right corner of the image.

The **n** field specifies the plane to use. If this is **NULL**, a new plane
will be created, having the precise geometry necessary to blit the specified
section of the image. This might be larger (or smaller) than the visual area.

**y** and **x** have different meanings depending on whether or not **n** was
**NULL**. If not (drawing onto a preexisting plane), they specify where in
the plane to start drawing. If **n** was **NULL** (new plane), they specify
the origin of the new plane relative to the standard plane. If the **flags**
field contains **NCVISUAL_OPTION_HORALIGNED**, the **x** parameter is
interpreted as an **ncalign_e** rather than an absolute position. If the
**flags** field contains **NCVISUAL_OPTION_VERALIGNED**, the **y** parameter
is interpreted as an **ncalign_e** rather than an absolute position.

# BLITTERS

The different **ncblitter_e** values select from among available glyph sets:

* **NCBLIT_DEFAULT**: Let the **ncvisual** choose its own blitter.
* **NCBLIT_1x1**: Spaces only. Works in ASCII, unlike most other blitters.
* **NCBLIT_2x1**: Adds the half blocks (▄▀) to **NCBLIT_1x1**.
* **NCBLIT_2x2**: Adds left and right half blocks (▌▐) and quadrants (▖▗▟▙) to **NCBLIT_2x1**.
* **NCBLIT_3x2**: Adds sextants to **NCBLIT_1x1**.
* **NCBLIT_BRAILLE**: 4 rows and 2 columns of braille (⡀⡄⡆⡇⢀⣀⣄⣆⣇⢠⣠⣤⣦⣧⢰⣰⣴⣶⣷⢸⣸⣼⣾⣿).
* **NCBLIT_PIXEL**: Adds pixel graphics (these also work in ASCII).

Two more blitters exist for plots, but are unsuitable for generic media:

* **NCBLIT_4x1**: Adds ¼ and ¾ blocks (▂▆) to **NCBLIT_2x1**.
* **NCBLIT_8x1**: Adds ⅛, ⅜, ⅝, and ⅞ blocks (▇▅▃▁) to **NCBLIT_4x1**.

**NCBLIT_4x1** and **NCBLIT_8x1** are intended for use with plots, and are
not really applicable for general visuals. **NCBLIT_BRAILLE** doesn't tend
to work out very well for images, but (depending on the font) can be very
good for plots.

A string can be transformed to a blitter with **notcurses_lex_blitter**,
recognizing **ascii**, **half**, **quad**, **sex**, **fourstep**, **braille**,
**eightstep**, and **pixel**. Conversion in the opposite direction is performed
with **notcurses_str_blitter**.

In the absence of scaling, for a given set of pixels, more rows and columns in
the blitter will result in a smaller output image. An image rendered with
**NCBLIT_1x1** will be twice as tall as the same image rendered with
**NCBLIT_2x1**, which will be twice as wide as the same image rendered with
**NCBLIT_2x2**. The same image rendered with **NCBLIT_3x2** will be one-third
as tall and one-half as wide as the original **NCBLIT_1x1** render (again, this
depends on **NCSCALE_NONE**). If the output size is held constant (using for
instance **NCSCALE_SCALE_HIRES** and a large image), more rows and columns will
result in more effective resolution.

A string can be transformed to a scaling mode with **notcurses_lex_scalemode**,
recognizing **stretch**, **scalehi**, **hires**, **scale**, and **none**.
Conversion in the opposite direction is performed with
**notcurses_str_scalemode**.

Assuming a cell is twice as tall as it is wide, **NCBLIT_1x1** (and indeed
any NxN blitter) will stretch an image by a factor of 2 in the vertical
dimension. **NCBLIT_2x1** will not distort the image whatsoever, as it maps a
vector two pixels high and one pixel wide to a single cell. **NCBLIT_3x2** will
stretch an image by a factor of 1.5.

The cell's dimension in pixels is ideally evenly divisible by the blitter
geometry. If **NCBLIT_3x2** is used together with a cell 8 pixels wide and
14 pixels tall, two of the vertical segments will be 5 pixels tall, while one
will be 4 pixels tall. Such unequal distributions are more likely with larger
blitter geometries. Likewise, there are only ever two colors available to us in
a given cell. **NCBLIT_1x1** and **NCBLIT_2x2** can be perfectly represented
with two colors per cell. Blitters of higher geometry are increasingly likely
to require some degree of interpolation. Transparency is always honored with
complete fidelity.

Finally, rendering operates slightly differently when two planes have both been
blitted, and one lies atop the other. See **notcurses_render(3)** for more
information.

# PIXEL BLITTING

Some terminals support pixel-based output via one of a number of protocols.
Checking for bitmap support requires interrogating the terminal and reading a
response. This takes time, and will never complete if the terminal doesn't
respond. Notcurses will not do so without an explicit request from the client
code. Before **NCBLIT_PIXEL** can be used, it is thus necessary to call
**notcurses_check_pixel_support**. If this function has not successfully
returned, attempts to use **NCBLIT_PIXEL** will fall back to cell-based
blitting (or fail, if **NCVISUAL_OPTION_NODEGRADE** is used).

**NCBLIT_PIXEL** has some stringent requirements on the type of planes it can
be used with; it is usually best to let **ncvisual_render** create the backing
plane by providing a **NULL** value for **n**. If you must bring your own
plane, it must be perfectly sized for the bitmap (i.e. large enough, and not
more than a full cell larger in either dimension--the bitmap, always placed at
the origin, must at least partially cover every cell of the plane). Using
**NCSCALE_STRETCH** means that the second condition will always be met. Once a
sprixel is blitted to a plane, cell methods (including cell blitting) may not
be used with it. Resizing the plane eliminates the sprixel, as does destroying
the plane. A sprixelated plane may be moved in all three dimensions,
duplicated, and reparented. The base cell of a sprixelated plane is
meaningless; if the sprixel is not an even multiple of the cell geometry, any
excess cell material is ignored during rendering.

Only one bitmap can be blitted onto a plane at a time (but multiple planes
with bitmaps may be visible); blitting a second to the same plane will delete
the original.

# RETURN VALUES

**ncvisual_from_file** returns an **ncvisual** object on success, or **NULL**
on failure. Success indicates that the specified **file** was opened, and
enough data was read to make a firm codec identification. It does not imply
that the entire file is properly-formed.

**ncvisual_decode** returns 0 on success, or 1 on end of file, or -1 on
failure. It is only necessary for multimedia-based visuals. It advances one
frame for each call. **ncvisual_decode_loop** has the same return values: when
called following decoding of the last frame, it will return 1, but a subsequent
**ncvisual_render** will return the first frame.

**ncvisual_from_plane** returns **NULL** if the **ncvisual** cannot be created
and bound. This is usually due to illegal content in the source **ncplane**.

**ncvisual_render** returns **NULL** on error, and otherwise the plane to
which the visual was rendered. If **opts->n** is provided, this will be
**opts->n**. Otherwise, a plane will be created, perfectly sized for the
visual and the specified blitter.

**ncvisual_blitter_geom** returns non-zero if the specified blitter is invalid.

**ncvisual_media_defblitter** returns the blitter selected by **NCBLIT_DEFAULT**
in the specified configuration. If UTF8 is not enabled, this will always be
**NCBLIT_1x1**. If ***scale*** is **NCSCALE_NONE** or **NCSCALE_SCALE**, the
aspect-preserving **NCBLIT_2x1** will be returned. If sextants are available
(see **notcurses_cansextant**), this will be **NCBLIT_3x2**, or otherwise
**NCBLIT_2x2**.

# NOTES

Multimedia decoding requires that Notcurses be built with either FFmpeg or
OpenImageIO support. What formats can be decoded is totally dependent on the
linked library. OpenImageIO does not support subtitles.

Sixel documentation can be found at [Dankwiki](https://nick-black.com/dankwiki/index.php?title=Sixel).
Kitty's graphics protocol is specified in [its documentation](https://sw.kovidgoyal.net/kitty/graphics-protocol.html).

Bad font support can ruin **NCBLIT_2x2**, **NCBLIT_3x2**, **NCBLIT_4x1**,
**NCBLIT_BRAILLE**, and **NCBLIT_8x1**. Braille glyphs ought ideally draw only
the raised dots, rather than drawing all eight dots with two different styles.
It's often best for the emulator to draw these glyphs itself.

Several emulators claim to implement Sixel, but do so in a more or less broken
fashion. I consider **XTerm** and **foot** to be reference Sixel
implementations on X.org and Wayland, respectively.

Sixels are fundamentally expressed in terms of six-line bands. If the rendered
bitmap is not a multiple of six rows, the necessary rows will be faked via
transparent rows. All sprixels have a height in rows, and if this height is
not a multiple of the cell height in rows, the last rows will only partially
obstruct a row of cells. This can lead to undesirable redraws and flicker if
the cells underneath the sprixel change. A sprixel which is both a multiple of
the cell height and a multiple of six is the most predictable possible sprixel.

# BUGS

Functions which describe rendered state such as **ncplane_at_yx** and
**notcurses_at_yx** will return an **nccell** with a sprixel ID, but this
sprixel cannot be accessed.

**ncvisual_rotate** currently supports only **M_PI**/2 and -**M_PI**/2
radians for **rads**, but this will change soon.

**ncvisual_render** should be able to create new planes in piles other than
the standard pile. This ought become a reality soon.

Sprixels interact poorly with multiple planes, and such usage is discouraged.
This situation might improve in the future.

Multiple threads may not currently call **ncvisual_render** concurrently
using the same **ncvisual**, even if targeting distinct **ncplane**s. This
will likely change in the future.

# SEE ALSO

**notcurses(3)**,
**notcurses_capabilities(3)**,
**notcurses_plane(3)**,
**notcurses_render(3)**,
**utf-8(7)**
