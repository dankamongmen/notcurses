% notcurses_error(3)
% nick black <nickblack@linux.com>
% v1.6.3

# NAME

notcurses_error - granular Notcurses error reporting

# SYNOPSIS

**#include <notcurses/ncerrs.h>**

```c
typedef enum {
  NCERR_SUCCESS = 0,
  NCERR_NOMEM = ENOMEM,
  NCERR_EOF = 0x20464f45, // matches AVERROR_EOF
} nc_err_e;
```

# DESCRIPTION

Various functions in Notcurses return granular information about the cause of
an error. When done, this information is returned through an **nc_err_e**.

# NOTES

The goal is to abstract the union of errors returned by supported operating
systems and the libraries on which Notcurses depends. Thus **nc_err_e** is
a union of members of POSIX and FFmpeg.

# RETURN VALUES

# SEE ALSO

**errno(3)**,
**notcurses(3)**
