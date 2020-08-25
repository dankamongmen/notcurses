% notcurses-ncreel(1)
% nick black <nickblack@linux.com>
% v1.6.18

# NAME

notcurses-ncreel - Experiment with ncreels

# SYNOPSIS

**notcurses-ncreel** [**-t tabletbordermask**] [**-b bordermask**] [**-ob bottomoffset**] [**-ot topoffset**] [**-ol leftoffset**] [**-or rightoffset**] [**-ln**]

# DESCRIPTION

**notcurses-ncreel** generates an ncreel to experiment with. With the
program open, press 'a' to create a new tablet, or 'd' to delete the focused
tablet (if one exists). 'q' quits at any time.

# OPTIONS

**-l loglevel**: Log everything (log level 8) or nothing (log level 0) to stderr.

# NOTES

Optimal display requires a terminal advertising the **rgb** terminfo(5)
capability, or that the environment variable **COLORTERM** is defined to
**24bit** (and that the terminal honors RGB escapes), along with a good
monospaced font supporting the Unicode Block Drawing Characters.

# SEE ALSO

**notcurses(3)**,
**notcurses_reel(3)**,
**terminfo(5)**
