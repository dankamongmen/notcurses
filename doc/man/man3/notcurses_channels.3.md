% notcurses_channels(3)
% nick black <nickblack@linux.com>
% v3.0.9

# NAME

notcurses_channels - operations on notcurses channels

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
#define NCCHANNEL_INITIALIZER(r, g, b) \
  (((uint32_t)r << 16u) + ((uint32_t)g << 8u) + (b) + NC_BGDEFAULT_MASK)

#define NCCHANNELS_INITIALIZER(fr, fg, fb, br, bg, bb) \
  ((NCCHANNEL_INITIALIZER(fr, fg, fb) << 32ull) + \
   (NCCHANNEL_INITIALIZER(br, bg, bb)))
```

**uint32_t ncchannel_r(uint32_t ***channel***);**

**uint32_t ncchannel_g(uint32_t ***channel***);**

**uint32_t ncchannel_b(uint32_t ***channel***);**

**uint32_t ncchannel_rgb(uint32_t ***channel***);**

**uint32_t ncchannel_rgb8(uint32_t ***channel***, uint32_t* restrict ***r***, uint32_t* restrict ***g***, uint32_t* restrict ***b***);**

**int ncchannel_set_rgb8(uint32_t* ***channel***, unsigned ***r***, unsigned ***g***, unsigned ***b***);**

**int ncchannel_set(uint32_t* ***channel***, uint32_t ***rgb***);**

**uint32_t ncchannel_alpha(uint32_t ***channel***);**

**int ncchannel_set_alpha(uint32_t* ***channel***, unsigned ***alpha***);**

**bool ncchannel_default_p(uint32_t ***channel***);**

**uint32_t ncchannel_set_default(uint32_t* ***channel***);**

**uint32_t ncchannels_fg_rgb(uint64_t ***channels***);**

**uint32_t ncchannels_bg_rgb(uint64_t ***channels***);**

**int ncchannels_set_fg_rgb(uint64_t* ***channels***, uint32_t ***rgb***);**

**int ncchannels_set_bg_rgb(uint64_t* ***channels***, uint32_t ***rgb***);**

**unsigned ncchannels_fg_alpha(uint64_t ***channels***);**

**unsigned ncchannels_bg_alpha(uint64_t ***channels***);**

**int ncchannels_set_fg_alpha(uint64_t* ***channels***, int ***alpha***);**

**int ncchannels_set_bg_alpha(uint64_t* ***channels***, int ***alpha***);**

**uint32_t ncchannels_fg_rgb8(uint64_t ***channels***, unsigned* ***r***, unsigned* ***g***, unsigned* ***b***);**

**uint32_t ncchannels_bg_rgb8(uint64_t ***channels***, unsigned* ***r***, unsigned* ***g***, unsigned* ***b***);**

**int ncchannels_set_fg_rgb8(uint64_t* ***channels***, unsigned ***r***, unsigned ***g***, unsigned ***b***);**

**int ncchannels_set_bg_rgb8(uint64_t* ***channels***, unsigned ***r***, unsigned ***g***, unsigned ***b***);**

**bool ncchannels_fg_default_p(uint64_t ***channels***);**

**bool ncchannels_bg_default_p(uint64_t ***channels***);**

**uint64_t ncchannels_set_fg_default(uint64_t* ***channels***);**

**uint64_t ncchannels_set_bg_default(uint64_t* ***channels***);**

**uint64_t ncchannels_reverse(uint64_t ***channels***);**

**unsigned ncchannel_palindex(uint32_t ***channel***);**

**bool ncchannel_palindex_p(uint32_t ***channel***);**

**int ncchannel_set_palindex(uint32_t* ***channel***, unsigned ***idx***);**

**unsigned ncchannels_fg_palindex(uint64_t ***channels***);**

**unsigned ncchannels_bg_palindex(uint64_t ***channels***);**

**int ncchannels_set_fg_palindex(uint64_t* ***channels***, unsigned ***idx***);**

**int ncchannels_set_bg_palindex(uint64_t* ***channels***, unsigned ***idx***);**

**uint64_t ncchannels_set_channels(uint64_t* ***dst***, uint64_t ***channels***);**

**uint64_t ncchannels_channels(uint64_t ***channels***);**

**uint64_t ncchannels_combine(uint32_t ***fchan***, uint32_t ***bchan***);**

# DESCRIPTION

Channels ought not be manually manipulated. They contain several bits used
"behind the scenes", and e.g. direct assignment is likely to lead to strange
and infrequent failures. To assign one channel pair to another, use
**ncchannels_set_channels**. To assign a channel to a channel pair's
foreground, use **ncchannels_set_fchannel**. To assign a channel to a channel
pair's background, use **ncchannels_set_bchannel**.

**ncchannel_palindex** extracts the palette index from a channel. The channel
must be palette-indexed, or the return value is meaningless. Verify palette
indexing with **ncchannel_palindex_p**. A channel can be set to palette
indexed mode (and have the index set) with **ncchannel_set_palindex**. The
index must be less than **NCPALETTESIZE**.

**ncchannels_combine** creates a new channel pair using ***fchan*** as the
foreground channel and ***bchan*** as the background channel.

# RETURN VALUES

Functions returning **int** return -1 on failure, or 0 on success. Failure is
always due to invalid inputs. Functions returning **bool** are predicates, and
return the requested value. Functions returning **unsigned** forms return the
input, modified as requested.

**ncchannels_reverse** inverts the color components of the two channels,
while holding all other elements constant. It's the Notcurses approximation
to reverse video.

# SEE ALSO

**notcurses(3)**,
**notcurses_cell(3)**,
**notcurses_plane(3)**,
**notcurses_output(3)**
