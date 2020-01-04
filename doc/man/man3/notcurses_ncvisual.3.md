% notcurses_ncvisual(3)
% nick black <nickblack@linux.com>
% v1.0.0

# NAME

notcurses_ncvisual-notcurses multimedia

# SYNOPSIS

**#include <notcurses.h>**

**struct ncvisual* ncplane_visual_open(struct ncplane* nc, const char* file,
                                         int* averr);**

```c
typedef enum {
  NCSCALE_NONE,
  NCSCALE_SCALE,
  NCSCALE_STRETCH,
} ncscale_e;
```

**struct ncvisual* ncvisual_open_plane(struct notcurses* nc, const char* file,
                                         int* averr, int y, int x,
                                         ncscale_e style);**

**void ncvisual_destroy(struct ncvisual* ncv);**

**struct AVFrame* ncvisual_decode(struct ncvisual* nc, int* averr);**

**int ncvisual_render(const struct ncvisual* ncv, int begy, int begx,
                        int leny, int lenx);**

**typedef int (*streamcb)(struct notcurses*, struct ncvisual*, void*);**

**int ncvisual_simple_streamer(struct notcurses* nc, struct ncvisual* ncv, void* curry);**

**int ncvisual_stream(struct notcurses* nc, struct ncvisual* ncv,
                        int* averr, streamcb streamer, void* curry);**

**struct ncplane* ncvisual_plane(struct ncvisual* ncv);**

# DESCRIPTION


# RETURN VALUES

# SEE ALSO

**notcurses(3)**, **notcurses_ncplane(3)**
