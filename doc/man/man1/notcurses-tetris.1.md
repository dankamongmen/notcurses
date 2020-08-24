% notcurses-tetris(1)
% nick black <nickblack@linux.com>
% v1.6.17

# NAME

notcurses-tetris - Render images and video to a terminal

# SYNOPSIS

**notcurses-tetris** [**-h|--help**] [**-l loglevel**]

# DESCRIPTION

**notcurses-tetris** implements Tetris using notcurses.

The goal is to complete horizontal lines, without allowing tetriminos to
reach the top of the screen. The falling tetrimino can be rotated counter-
clockwise with the 'z' key, and clockwise with the 'x' key. The tetrimino
can be moved left and right with 'h' and 'l', respectively. It can be moved
down with 'j'. The arrow keys can also be used. Quit with 'q'.

# OPTIONS

**-h**: Show help and exit.

**-l loglevel**: Log everything (high log level) or nothing (log level 0) to stderr.

# NOTES

Optimal display requires a terminal advertising the **rgb** terminfo(5)
capability, or that the environment variable **COLORTERM** is defined to
**24bit** (and that the terminal honors RGB escapes), along with a good
fixed-width font with good coverage of the Unicode Block Drawing Characters.

# SEE ALSO

**notcurses(3)**
