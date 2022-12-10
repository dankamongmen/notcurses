% tfman(1)
% nick black <nickblack@linux.com>
% v3.0.9

# NAME

tfman - Swank manual page browser 

# SYNOPSIS

**tfman** [**-h**] [**-V**] [**-q**] files

# DESCRIPTION

**tfman** displays manual pages ala **man(1)** using the Notcurses
(**notcurses(3)**) terminal UI library.

# OPTIONS

**-V**: Print the program name and version, and exit with success.

**-h**: Print help information, and exit with success.

**-q**: Don't wait for any input, and don't use the alternate screen.

files: Files to render.

If successfully loaded and parsed, the top of the page will be visible.
In the lower right corner is a listing of page sections. By default, the
page browser is in use; press Tab to move between the page browser and
the structure browser. Press 's' to toggle the structure browser's
visibility.

**tfman** can identify gzipped manual pages, and inflate them on the fly.

# NOTES

The following keypresses are recognized:

* **Ctrl-L**: Redraw the screen.
* **q**: Quit.
* **k**/**up**: Move up by one line.
* **b**/**pgup**: Move up by one page.
* **h**/**left**: Move up by one section.
* **j**/**down**: Move down by one line.
* **f**/**pgdown**: Move down by one page.
* **l**/**right**: Move down by one section.
* **g**/**home**: Go to first line in file.
* **s**: Toggle the structure browser's visibility.

The mouse wheel can also be used to move up and down within the active browser.

# BUGS

**tfman** does not currently (and is unlikely to ever) support the full
**groff** macro language.

# SEE ALSO

**man(1)**,
**notcurses(3)**,
**groff_man(7)**,
**man-pages(7)**,
**unicode(7)**
