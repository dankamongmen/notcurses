% notcurses-input(1)
% nick black <nickblack@linux.com>
% v1.2.3

# NAME

notcurses-input - Read and display input events

# SYNOPSIS

**notcurses-input**

# DESCRIPTION

**notcurses-input** reads from stdin and decodes the input to stdout, including
synthesized events and mouse events. To exit, generate EOF (usually Ctrl+'d').

# OPTIONS

# NOTES

Mouse events are only generated for button presses, and for movement while a
button is held down.

# SEE ALSO

**notcurses(3)**,
**notcurses_input(3)**
