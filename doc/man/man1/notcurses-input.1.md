% notcurses-input(1)
% nick black <nickblack@linux.com>
% v2.4.2

# NAME

notcurses-input - Read and display input events

# SYNOPSIS

**notcurses-input** [**-v**]

# DESCRIPTION

**notcurses-input** reads from stdin and decodes the input to stdout, including
synthesized events and mouse events. To exit, generate EOF (usually Ctrl+'d').

Each event will be printed on a single line. Leading that line is a series
of modifier indicators:

* 'A'/'a': Alt was or was not pressed.
* 'C'/'c': Ctrl was or was not pressed.
* 'S'/'s': Shift was or was not pressed.
* 'L'/'R'/'P'/'u': Key was a release, repeat, press, or of unknown type.

# OPTIONS

**-v**: Increase verbosity.

# NOTES

Mouse events are only generated for button presses and releases, and for
movement while a button is held down.

# SEE ALSO

**tack(1)**,
**notcurses(3)**,
**notcurses_input(3)**
