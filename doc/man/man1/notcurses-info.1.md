% notcurses-info(1)
% nick black <nickblack@linux.com>
% v2.3.4

# NAME

notcurses-info - Display information about the terminal environment

# SYNOPSIS

**notcurses-info**

# DESCRIPTION

**notcurses-info** prints all the information it knows about the current
terminal environment, including material loaded from **terminfo(5)** (based
on the **TERM** environment variable), replies from the terminal in
response to our queries, and built-in heuristics.

The Unicode half block, quadrant, sextant, and Braille glyphs are all included
in the output. If their appearance is irregular, it might behoove you to choose
another font.

# OPTIONS

# NOTES

The behavior of **notcurses-info** (and indeed all of Notcurses) depends on
the **TERM** and **LANG** environment variables, the installed POSIX locales,
and the installed **terminfo(5)** databases.

# SEE ALSO

**notcurses(3)**,
**terminfo(5)**
