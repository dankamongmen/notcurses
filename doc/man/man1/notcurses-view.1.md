% notcurses-view(1)
% nick black <nickblack@linux.com>
% v2.0.2

# NAME

notcurses-view - Render images and video to a terminal

# SYNOPSIS

**notcurses-view** [**-h**] [**-q**] [**-d delaymult**] [**-l loglevel**] [**-s scalemode**] [**-k**] [**-L**] files

# DESCRIPTION

**notcurses-view** uses a multimedia-enabled notcurses to render images
and videos to a terminal. By default, **stretch**-type scaling is used to
fill the rendering area, and the **quadblitter** blitter is used for a
2x2→1 mapping from pixels to cells.

# OPTIONS

**-d delaymult**: Apply a rational multiplier to the framerate.

**-l loglevel**: Log everything (high log level) or nothing (log level 0) to stderr.

**-s scalemode**: Scaling mode, one of **none**, **scale**, or **stretch**.

**-b blitter**: Blitter, one of **ascii**, **halfblocks**, **quadblitter**,
**sexblitter**, or **braille**.

**-m margins**: Define rendering margins (see below).

**-L**: Loop frames until a key is pressed.

**-k**: Inhibit use of the alternate screen. Necessary if you want the output left on your terminal after the program exits.

**-q**: Don't print frame/timing information along the top of the screen.

**-h**: Print help information, and exit with success.

files: Select which files to render, and what order to render them in.

Default margins are all 0 and default scaling is **stretch**. The full
rendering area will thus be used. Using **-m**, margins can be supplied.
Provide a single number to set all four margins to the same value, or four
comma-delimited values for the top, right, bottom, and left margins
respectively. Negative margins are illegal.

Scaling mode **stretch** resizes the object to match the target rendering
area exactly. **scale** resizes the object so that the longer edge of the
rendering area is matched exactly, and the other edge is changed to
maintain aspect ratio. **none** uses the original image size.

Blitters can be selected by pressing '0' through '8'. **NCBLIT_DEFAULT**
corresponds to '0'. The various blitters are described in
**notcurses_visual**.

A video can be paused with space. Press space (or any other valid control)
to resume.

# NOTES

Optimal display requires a terminal advertising the **rgb** terminfo(5)
capability, or that the environment variable **COLORTERM** is defined to
**24bit** (and that the terminal honors this variable), along with a
fixed-width font with good coverage of the Unicode Block Drawing Characters.

# SEE ALSO

**notcurses(3)**,
**notcurses_visual(3)**,
**terminfo(5)**,
**unicode(7)**
