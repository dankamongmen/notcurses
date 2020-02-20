% notcurses-view(1)
% nick black <nickblack@linux.com>
% v1.2.1

# NAME

notcurses-view - Render images and video to the console

# SYNOPSIS

**notcurses-view** [**-h|--help**] [**-d delaymult**] [**-l loglevel**] [**-s scalemode**] files

# DESCRIPTION

**notcurses-view** uses the FFmpeg libraries and notcurses to render images
and videos to the terminal. Media will be scaled to the terminal's size.

# OPTIONS

**-d delaymult**: Apply a rational multiplier to the standard delay of 1s.

**-l loglevel**: Log everything (high log level) or nothing (log level 0) to stderr.

**-s scalemode**: Scaling mode, one of **none**, **scale**, or **stretch**.

files: Select which files to render, and what order to render them in.

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
