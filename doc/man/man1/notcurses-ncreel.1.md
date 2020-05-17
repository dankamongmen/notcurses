% notcurses-ncreel(1)
% nick black <nickblack@linux.com>
% v1.4.2.1

# NAME

notcurses-ncreel - Experiment with ncreels

# SYNOPSIS

**notcurses-ncreel** [**-t tabletbordermask**] [**-b bordermask**] [**-ob bottomoffset**] [**-ot topoffset**] [**-ol leftoffset**] [**-or rightoffset**]

# DESCRIPTION

**notcurses-ncreel** generates an ncreel to experiment with. With the
program open, press 'a' to create a new tablet, or 'd' to delete the focused
tablet (if one exists). 'q' quits at any time.

# OPTIONS

# NOTES

Optimal display requires a terminal advertising the **rgb** terminfo(5)
capability, or that the environment variable **COLORTERM** is defined to
**24bit** (and that the terminal honors RGB escapes), along with a good
monospaced font supporting the Unicode Block Drawing Characters.

# SEE ALSO

**notcurses(3)**,
**notcurses_reel(3)**,
**terminfo(5)**
