% ncman(1)
% nick black <nickblack@linux.com>
% v3.0.0

# NAME

ncman - Swank manual page browser 

# SYNOPSIS

**ncman** [**-h**] [**-V**] files

# DESCRIPTION

**ncman** displays manual pages ala **man(1)** using the Notcurses
(**notcurses(3)**) terminal UI library.

# OPTIONS

**-V**: Print the program name and version, and exit with success.

**-h**: Print help information, and exit with success.

files: Files to render.

The following keypresses are recognized:

* **Ctrl-L**: Redraw the screen.
* **q**: Quit.

# NOTES

# BUGS

# SEE ALSO

**man(1)**,
**notcurses(3)**,
**groff_man(7)**,
**man-pages(7)**,
**unicode(7)**
