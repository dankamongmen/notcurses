% ncplayer(1)
% nick black <nickblack@linux.com>
% v2.2.1

# NAME

ncplayer - Render images and video to a terminal

# SYNOPSIS

**ncplayer** [**-h**] [**-V**] [**-q**] [**-d** ***delaymult***] [**-l** ***loglevel***] [**-s** ***scalemode***] [**-k**] [**-L**] [**-t** ***seconds***] files

# DESCRIPTION

**ncplayer** uses a multimedia-enabled Notcurses to render images and videos to a
terminal. By default, **stretch**-type scaling is used to fill the rendering
area, and the **sexblitter** blitter is used (where known to work well) for a
3x2→1 mapping from pixels to cells. In a terminal that doesn't support Unicode
13 sextants, the **quadblitter** is used instead.

# OPTIONS

**-d** ***delaymult***: Apply a non-negative rational multiplier to the delayscale.
Only applies to multiframe media such as video and animated images. Not supported with **-k**.

**-t** ***seconds***: Delay **seconds** after each file. If this option is used,
the "press any key to continue" prompt will not be displayed. **seconds** may
be any non-negative number.

**-l** ***loglevel***: Log everything (high log level) or nothing (log level 0) to stderr.

**-s** ***scalemode***: Scaling mode, one of **none**, **hires**, **scale**, **scalehi**, or **stretch**.

**-b** ***blitter***: Blitter, one of **ascii**, **halfblocks**, **quadblitter**,
**sexblitter**, or **braille**.

**-m margins**: Define rendering margins (see below).

**-L**: Loop frames until a key is pressed. Not supported with **-k**.

**-k**: Use direct mode (see **notcurses_direct(3)**). This will have the effect of leaving the output on-screen after program exit, and generating it inline (rather than clearing the screen and placing it at the top). Not supported with **-L** or **-d**.

**-q**: Print neither frame/timing information along the top of the screen, nor the output summary on exit.

**-V**: Print the program name and version, and exit with success.

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

If you're looking for a fast, inline image viewer for the shell, try using
**ncplayer -k -t0 -q**.

Optimal display requires a terminal advertising the **rgb** terminfo(5)
capability, or that the environment variable **COLORTERM** is defined to
**24bit** (and that the terminal honors this variable), along with a
fixed-width font with good coverage of the Unicode Block Drawing Characters.

# BUGS

Direct mode (**-k**) does not yet support multiframe media. It'll read them
just fine, but only show the first frame. This might or might not change in
the future. Direct mode is kinda fundamentally suboptimal for multiframe
media. Until that time, **-k** is exclusive with **-d** and **-L**.

# SEE ALSO

**notcurses(3)**,
**notcurses_direct(3)**,
**notcurses_visual(3)**,
**terminfo(5)**,
**unicode(7)**
