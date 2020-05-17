% notcurses-view(1)
% nick black <nickblack@linux.com>
% v1.4.2.1

# NAME

notcurses-view - Render images and video to the console

# SYNOPSIS

**notcurses-view** [**-h|--help**] [**-d delaymult**] [**-l loglevel**] [**-s scalemode**] [**-k**] files

# DESCRIPTION

**notcurses-view** uses a multimedian-enabled notcurses to render images
and videos to the terminal. Media will be scaled to the terminal's size.

# OPTIONS

**-d delaymult**: Apply a rational multiplier to the framerate.

**-l loglevel**: Log everything (high log level) or nothing (log level 0) to stderr.

**-s scalemode**: Scaling mode, one of **none**, **scale**, or **stretch**.

**-m margins**: Define rendering margins (see below).

**-k**: Inhibit use of the alternate screen. Necessary if you want the output left on your terminal after the program exits.

files: Select which files to render, and what order to render them in.

Default margins are all 0, and thus the full screen will be rendered. Using
**-m**, margins can be supplied. Provide a single number to set all four margins
to the same value, or four comma-delimited values for the top, right, bottom,
and left margins respectively. Negative margins are illegal.

# NOTES

Optimal display requires a terminal advertising the **rgb** terminfo(5)
capability, or that the environment variable **COLORTERM** is defined to
**24bit** (and that the terminal honors this variable), along with a
fixed-width font with good coverage of the Unicode Block Drawing Characters.

# SEE ALSO

**notcurses(3)**,
**notcurses_ncvisual(3)**,
**terminfo(5)**,
**unicode(7)**
