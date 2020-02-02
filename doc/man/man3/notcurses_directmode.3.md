% notcurses_directmode(3)
% nick black <nickblack@linux.com>
% v1.1.4

# NAME

notcurses_directmode - minimal notcurses instances for styling text

# SYNOPSIS

**#include <notcurses.h>**

**struct ncdirect* notcurses_directmode(const char *termtype, FILE* fp);**

**int ncdirect_bg_rgb8(struct ncdirect* nc, unsigned r, unsigned g, unsigned b);**

**int ncdirect_fg_rgb8(struct ncdirect* nc, unsigned r, unsigned g, unsigned b);**

**int ncdirect_fg(struct ncdirect* nc, unsigned rgb);**

**int ncdirect_bg(struct ncdirect* nc, unsigned rgb);**

**int ncdirect_stop(struct ncdirect* nc);**

# DESCRIPTION

**notcurses_directmode** prepares the **FILE** provided as **fp** (which must
be attached to a terminal) for colorizing and styling. On success, a pointer to
a valid **struct ncdirect** is returned. **NULL** is returned on failure.
Before the process exits, **ncdirect_stop(3)** should be called to reset the
terminal and free up resources.

An appropriate **terminfo(5)** entry must exist for the terminal. This entry is
usually selected using the value of the **TERM** environment variable (see
**getenv(3)**), but a non-**NULL** value for **termtype** will override this. An
invalid terminfo specification can lead to reduced performance, reduced
display capabilities, and/or display errors. notcurses natively targets
24bpp/8bpc RGB color, and it is thus desirable to use a terminal with the
**rgb** capability (e.g. xterm's **xterm-direct**).

# RETURN VALUES

**notcurses_directmode** returns **NULL** on failure. Otherwise, the return
value points to a valid **struct ncdirect**, which can be used until it is
provided to **ncdirect_stop**.

# SEE ALSO

**getenv(3)**, **termios(3)**, **notcurses(3)**, **terminfo(5)**
