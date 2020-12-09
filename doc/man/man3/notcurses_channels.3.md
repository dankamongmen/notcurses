% notcurses_channels(3)
% nick black <nickblack@linux.com>
% v2.0.11

# NAME

notcurses_channels - operations on notcurses channels

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
#define CHANNELS_RGB_INITIALIZER(fr, fg, fb, br, bg, bb) \
  (((((uint64_t)(fr) << 16u) + ((uint64_t)(fg) << 8u) + (uint64_t)(fb)) << 32ull) + \
   (((br) << 16u) + ((bg) << 8u) + (bb)) + CELL_BGDEFAULT_MASK + CELL_FGDEFAULT_MASK)

#define CHANNEL_RGB_INITIALIZER(r, g, b) \
  (((uint32_t)r << 16u) + ((uint32_t)g << 8u) + (b) + CELL_BGDEFAULT_MASK)
```

**uint32_t channel_r(uint32_t ***channel***);**

**uint32_t channel_g(uint32_t ***channel***);**

**uint32_t channel_b(uint32_t ***channel***);**

**uint32_t channel_rgb8(uint32_t channel, uint32_t* restrict ***r***, uint32_t* restrict ***g***, uint32_t* restrict ***b***);**

**int channel_set_rgb8(uint32_t* channel, int ***r***, int ***g***, int ***b***);**

**int channel_set(uint32_t* channel, uint32_t rg***b***);**

**uint32_t channel_alpha(uint32_t ***channel***);**

**int channel_set_alpha(uint32_t* ***channel***, unsigned ***alpha***);**

**bool channel_default_p(uint32_t ***channel***);**

**uint32_t channel_set_default(uint32_t* ***channel***);**

**unsigned channels_bchannel(uint64_t ***channels***);**

**unsigned channels_fchannel(uint64_t ***channels***);**

**uint64_t channels_set_bchannel(uint64_t* ***channels***, uint32_t ***channel***);**

**uint64_t channels_set_fchannel(uint64_t* ***channels***, uint32_t ***channel***);**

**unsigned channels_fg_rgb(uint64_t ***channels***);**

**unsigned channels_bg_rgb(uint64_t ***channels***);**

**unsigned channels_fg_alpha(uint64_t ***channels***);**

**unsigned channels_bg_alpha(uint64_t ***channels***);**

**unsigned channels_fg_rgb8(uint64_t ***channels***, unsigned* ***r***, unsigned* ***g***, unsigned* ***b***);**

**unsigned channels_bg_rgb8(uint64_t ***channels***, unsigned* ***r***, unsigned* ***g***, unsigned* ***b***);**

**int channels_set_fg_rgb8(uint64_t* ***channels***, int ***r***, int ***g***, int ***b***);**

**int channels_set_bg_rgb8(uint64_t* ***channels***, int ***r***, int ***g***, int ***b***);**

**int channels_set_fg_rgb(uint64_t* ***channels***, unsigned ***rgb***);**

**int channels_set_bg_rgb(uint64_t* ***channels***, unsigned ***rgb***);**

**int channels_set_fg_alpha(uint64_t* ***channels***, int ***alpha***);**

**int channels_set_bg_alpha(uint64_t* ***channels***, int ***alpha***);**

**bool channels_fg_default_p(uint64_t ***channels***);**

**bool channels_bg_default_p(uint64_t ***channels***);**

**uint64_t channels_set_fg_default(uint64_t* ***channels***);**

**uint64_t channels_set_bg_default(uint64_t* ***channels***);**

# DESCRIPTION


# RETURN VALUES

Functions returning `int` return -1 on failure, or 0 on success. Failure is
always due to invalid inputs. Functions returning `bool` are predicates, and
return the requested value. Functions returning `unsigned` forms return the
input, modified as requested.

# SEE ALSO

**notcurses(3)**,
**notcurses_cell(3)**,
**notcurses_plane(3)**,
**notcurses_output(3)**
