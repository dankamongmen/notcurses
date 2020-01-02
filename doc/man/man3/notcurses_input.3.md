% notcurses_input(3)
% nick black <nickblack@linux.com>
% v0.9.9

# NAME

**notcurses_input**—input via notcurses

# SYNOPSIS

#include <notcurses.h>**

```c
typedef struct ncinput {
  char32_t id;     // Unicode codepoint
  int y;           // Y cell coordinate of event, -1 for undefined
  int x;           // X cell coordinate of event, -1 for undefined
  bool alt;        // Was Alt held during the event?
  bool shift;      // Was Shift held during the event?
  bool ctrl;       // Was Ctrl held during the event?
} ncinput;
```

**bool nckey_mouse_p(char32_t r);**

**char32_t notcurses_getc(struct notcurses* n, const struct timespec* ts, sigset_t* sigmask, ncinput* ni);**

**char32_t notcurses_getc_nblock(struct notcurses* n, ncinput* ni);**

**char32_t notcurses_getc_blocking(struct notcurses* n, ncinput* ni);**

**int notcurses_mouse_enable(struct notcurses* n);**

**int notcurses_mouse_disable(struct notcurses* n);**

# DESCRIPTION


# RETURN VALUES

# SEE ALSO

**notcurses(3)**, **poll(2)**, **unicode(7)**
