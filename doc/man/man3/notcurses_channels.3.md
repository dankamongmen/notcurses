% notcurses_channels(3)
% nick black <nickblack@linux.com>
% v1.3.2

# NAME

notcurses_channels - operations on notcurses channels

# SYNOPSIS

**#include <notcurses.h>**

**static inline unsigned
channel_r(unsigned channel);**

**static inline unsigned
channel_g(unsigned channel);**

**static inline unsigned
channel_b(unsigned channel);**

**static inline unsigned
channel_rgb(unsigned channel, unsigned* restrict r, unsigned* restrict g,
                unsigned* restrict b);**

**static inline int
channel_set_rgb(unsigned* channel, int r, int g, int b);**

**static inline int
channel_set(unsigned* channel, unsigned rgb);**

**static inline unsigned
channel_alpha(unsigned channel);**

**static inline int
channel_set_alpha(unsigned* channel, int alpha);**

**static inline bool
channel_default_p(unsigned channel);**

**static inline unsigned
channel_set_default(unsigned* channel);**

**static inline unsigned
channels_bchannel(uint64_t channels);**

**static inline unsigned
channels_fchannel(uint64_t channels);**

**static inline uint64_t
channels_set_bchannel(uint64_t* channels, uint32_t channel);**

**static inline uint64_t
channels_set_fchannel(uint64_t* channels, uint32_t channel);**

**static inline unsigned
channels_fg(uint64_t channels);**

**static inline unsigned
channels_bg(uint64_t channels);**

**static inline unsigned
channels_fg_alpha(uint64_t channels);**

**static inline unsigned
channels_bg_alpha(uint64_t channels);**

**static inline unsigned
channels_fg_rgb(uint64_t channels, unsigned* r, unsigned* g, unsigned* b);**

**static inline unsigned
channels_bg_rgb(uint64_t channels, unsigned* r, unsigned* g, unsigned* b);**

**static inline int
channels_set_fg_rgb(uint64_t* channels, int r, int g, int b);**

**static inline int
channels_set_bg_rgb(uint64_t* channels, int r, int g, int b);**

**static inline int
channels_set_fg(uint64_t* channels, unsigned rgb);**

**static inline int
channels_set_bg(uint64_t* channels, unsigned rgb);**

**static inline int
channels_set_fg_alpha(uint64_t* channels, int alpha);**

**static inline int
channels_set_bg_alpha(uint64_t* channels, int alpha);**

**static inline bool
channels_fg_default_p(uint64_t channels);**

**static inline bool
channels_bg_default_p(uint64_t channels);**

**static inline uint64_t
channels_set_fg_default(uint64_t* channels);**

**static inline uint64_t
channels_set_bg_default(uint64_t* channels);**

**static inline unsigned
channels_blend(unsigned c1, unsigned c2, unsigned blends);**

# DESCRIPTION


# RETURN VALUES

Functions returning `int` return -1 on failure, or 0 on success. Failure is
always due to invalid inputs. Functions returning `bool` are predicates, and
return the requested value. Functions returning `unsigned` forms return the
input, modified as requested.

# SEE ALSO

**notcurses(3)**, **notcurses_cell(3)**, **notcurses_ncplane(3)**,
**notcurses_output(3)**
