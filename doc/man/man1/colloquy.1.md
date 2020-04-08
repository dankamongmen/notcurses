% colloquy(1)
% nick black <nickblack@linux.com>
% v1.2.5

# NAME

colloquy - Command line dialogs and widgets

# SYNOPSIS

**colloquy** [**-h|--help**]

# DESCRIPTION

**colloquy** uses notcurses to construct attractive widgets from the command
line and shell scripts. It can be used with any terminal environment, and
supports both keyboards and mice. Available widget types include:

* Message boxes requiring a button press to continue
* Selection of a single item from a list
* Selection of zero or more items from a list

Data to be displayed is provided on the command line, and the user's input
is reported on standard output. If the user aborts, the program returns
non-zero.

**colloquy** is inspired by (and compatible with a subset of) **dialog(1)**,
a similar tool built atop NCURSES.

# OPTIONS

**-h**|**--help**: Display a usage message

# NOTES

Optimal display requires a terminal advertising the **rgb** terminfo(5)
capability, or that the environment variable **COLORTERM** is defined to
**24bit** (and that the terminal honors this variable), along with a
fixed-width font with good coverage of the Unicode Block Drawing Characters.

# SEE ALSO

**dialog(1)**,
**notcurses(3)**
