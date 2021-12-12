% tfman(1)
% nick black <nickblack@linux.com>
% v3.0.1

# NAME

tfman - Swank manual page browser 

# SYNOPSIS

**tfman** [**-h**] [**-V**] files

# DESCRIPTION

**tfman** displays manual pages ala **man(1)** using the Notcurses
(**notcurses(3)**) terminal UI library.

# OPTIONS

**-V**: Print the program name and version, and exit with success.

**-h**: Print help information, and exit with success.

files: Files to render.

The following keypresses are recognized:

* **Ctrl-L**: Redraw the screen.
* **q**: Quit.
* **k**/**up**: Move up by one line.
* **b**/**pgup**: Move up by one page.
* **j**/**down**: Move down by one line.
* **f**/**pgdown**: Move down by one page.

# NOTES

# BUGS

**tfman** does not currently (and is unlikely to ever) support the full
**groff** macro language.

# SEE ALSO

**man(1)**,
**notcurses(3)**,
**groff_man(7)**,
**man-pages(7)**,
**unicode(7)**
