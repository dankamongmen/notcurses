% notcurses_util(3)
% nick black <nickblack@linux.com>
% v2.3.15

# NAME

notcurses_util - portable utility functions

# SYNOPSIS

**#include <notcurses/notcurses.h>**

**char* notcurses_accountname(void);**

**char* notcurses_hostname(void);**

# DESCRIPTION

Notcurses provides some utility functions, usually to abstract away
platform-dependent differences.

**notcurses_accountname** returns a heap-allocated copy of the account
name under which the program is running. **notcurses_hostname** returns
a heap-allocated copy of the local host name.

# NOTES

# RETURN VALUES

All functions return ***NULL*** on error.

# SEE ALSO

**hostname(1)**,
**notcurses(3)**
