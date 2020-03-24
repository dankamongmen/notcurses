% notcurses-tetris(1)
% nick black <nickblack@linux.com>
% v1.2.3

# NAME

notcurses-tetris - Render images and video to the console

# SYNOPSIS

**notcurses-tetris** [**-h|--help**] [**-l loglevel**]

# DESCRIPTION

**notcurses-tetris** implements Tetris using notcurses.

# OPTIONS

**-h**: Show help and exit.

**-l loglevel**: Log everything (high log level) or nothing (log level 0) to stderr.

# NOTES

Optimal display requires a terminal advertising the **rgb** terminfo(5)
capability, or that the environment variable **COLORTERM** is defined to
**24bit** (and that the terminal honors this variable), along with a
fixed-width font with good coverage of the Unicode Block Drawing Characters.

# SEE ALSO

**notcurses(3)**
