% notcurses-tester(1)
% nick black <nickblack@linux.com>
% v2.4.999

# NAME

notcurses-tester - Notcurses unit testing

# SYNOPSIS

**notcurses-tester** [**-p datadir**] [**-l**]

# DESCRIPTION

**notcurses-tester** drives the several hundred unit tests included with
Notcurses. It requires several data files installed alongside Notcurses;
if these are in an irregular location, supply **-p**.

# OPTIONS

**-p** ***path***: Look in the specified ***path*** for data files.

**-l**: Enable all possible diagnostics/logging.

# NOTES

Valid **TERM** and **LANG** environment variables are necessary for
**notcurses-tester**'s correct operation.

# SEE ALSO

**notcurses(3)**
