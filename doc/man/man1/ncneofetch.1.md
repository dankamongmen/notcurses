% ncneofetch(1)
% nick black <nickblack@linux.com>
% v3.0.9

# NAME

ncneofetch - Generate low-effort posts for r/unixporn

# SYNOPSIS

**ncneofetch** [**-v**]

# DESCRIPTION

**ncneofetch** renders an image (typically a distribution logo)
and displays some system information. It is a blatant ripoff of
**neofetch(1)** using **notcurses(3)**.

# OPTIONS

**-v**: Increase verbosity.

# NOTES

Optimal display requires a terminal advertising the **rgb** terminfo(5)
capability, or that the environment variable **COLORTERM** is defined to
**24bit** (and that the terminal honors RGB escapes), along with a good
monospaced font supporting the Unicode Block Drawing Characters.

# SEE ALSO

**neofetch(1)**,
**notcurses(3)**
