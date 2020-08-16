% notcurses_cell(3)
% nick black <nickblack@linux.com>
% v1.6.12

# NAME

notcurses_cell - operations on notcurses cells

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
// See DESCRIPTION below for information on EGC encoding
typedef struct cell {
  uint32_t gcluster;          // 4B → 4B
  uint8_t gcluster_backstop;  // 1B → 5B (8 bits of zero)
  uint8_t reserved;           // 1B → 6B (8 reserved bits, ought be zero)
  uint16_t stylemask;         // 2B → 8B (16 bits of NCSTYLE_* attributes)
  uint64_t channels;
} cell;

#define CELL_TRIVIAL_INITIALIZER \
 { .gcluster = '\0', .stylemask = 0, .channels = 0, }
#define CELL_SIMPLE_INITIALIZER(c) \
 { .gcluster = (c), .stylemask = 0, .channels = 0, }
#define CELL_INITIALIZER(c, s, chan) \
 { .gcluster = (c), .stylemask = (s), .channels = (chan), }

#define CELL_WIDEASIAN_MASK     0x8000000080000000ull
#define CELL_BGDEFAULT_MASK     0x0000000040000000ull
#define CELL_FGDEFAULT_MASK     (CELL_BGDEFAULT_MASK << 32u)
#define CELL_BG_RGB_MASK        0x0000000000ffffffull
#define CELL_FG_RGB_MASK        (CELL_BG_MASK << 32u)
#define CELL_BG_PALETTE         0x0000000008000000ull
#define CELL_FG_PALETTE         (CELL_BG_PALETTE << 32u)
#define CHANNEL_ALPHA_MASK      0x30000000ull
#define CELL_ALPHA_HIGHCONTRAST 0x30000000ull
#define CELL_ALPHA_TRANSPARENT  0x20000000ull
#define CELL_ALPHA_BLEND        0x10000000ull
#define CELL_ALPHA_OPAQUE       0x00000000ull
```

**void cell_init(cell* c);**

**int cell_load(struct ncplane* n, cell* c, const char* gcluster);**

**int cell_prime(struct ncplane* n, cell* c, const char* gcluster,
                 uint32_t stylemask, uint64_t channels);**

**int cell_duplicate(struct ncplane* n, cell* targ, const cell* c);**

**void cell_release(struct ncplane* n, cell* c);**

**void cell_styles_set(cell* c, unsigned stylebits);**

**unsigned cell_styles(const cell* c);**

**void cell_styles_on(cell* c, unsigned stylebits);**

**void cell_styles_off(cell* c, unsigned stylebits);**

**void cell_set_fg_default(cell* c);**

**void cell_set_bg_default(cell* c);**

**int cell_set_fg_alpha(cell* c, unsigned alpha);**

**int cell_set_bg_alpha(cell* c, unsigned alpha);**

**bool cell_double_wide_p(const cell* c);**

**bool cell_simple_p(const cell* c);**

**const char* cell_extended_gcluster(const struct ncplane* n, const cell* c);**

**int cell_load_simple(struct ncplane* n, cell* c, char ch);**

**unsigned cell_bchannel(const cell* cl);**

**unsigned cell_fchannel(const cell* cl);**

**uint64_t cell_set_bchannel(cell* cl, uint32_t channel);**

**uint64_t cell_set_fchannel(cell* cl, uint32_t channel);**

**uint64_t cell_blend_fchannel(cell* cl, unsigned channel, unsigned blends)**

**uint64_t cell_blend_bchannel(cell* cl, unsigned channel, unsigned blends)**

**unsigned cell_fg(const cell* cl);**

**unsigned cell_bg(const cell* cl);**

**unsigned cell_fg_alpha(const cell* cl);**

**unsigned cell_bg_alpha(const cell* cl);**

**unsigned cell_fg_rgb(const cell* cl, unsigned* r, unsigned* g, unsigned* b);**

**unsigned cell_bg_rgb(const cell* cl, unsigned* r, unsigned* g, unsigned* b);**

**int cell_set_fg_rgb(cell* cl, int r, int g, int b);**

**int cell_set_bg_rgb(cell* cl, int r, int g, int b);**

**void cell_set_fg_rgb_clipped(cell* cl, int r, int g, int b);**

**void cell_set_bg_rgb_clipped(cell* cl, int r, int g, int b);**

**int cell_set_fg(cell* c, uint32_t channel);**

**int cell_set_bg(cell* c, uint32_t channel);**

**bool cell_fg_default_p(const cell* cl);**

**bool cell_bg_default_p(const cell* cl);**

# DESCRIPTION

Cells make up the framebuffer associated with each plane, with one cell per
addressable coordinate. You should not usually need to interact directly
with cells.

Each **cell** contains exactly one extended grapheme cluster. If the EGC happens
to be a single ASCII character, i.e. a single character with a value less than
128, it is encoded directly into the **cell**'s **gcluster** field, and no
additional storage is necessary. In this case, **cell_simple_p()** is **true**.
Otherwise, the EGC is stored as a UTF-8 string in some backing egcpool. Egcpools
are associated with **ncplane**s, so **cell**s must be considered associated
with **ncplane**s. Indeed, **ncplane_erase()** destroys the backing storage for
all a plane's cells, invalidating them. This association is formed at the time
of **cell_load()**, **cell_prime()**, or **cell_duplicate()**. All of these
functions first call **cell_release()**, as does **cell_load_simple()**. When
done using a **cell** entirely, call **cell_release()**. **ncplane_destroy()**
will free up the memory used by the **cell**, but the backing egcpool has a
maximum size of 32MiB, and failure to release **cell**s can eventually block new
output.

# RETURN VALUES

**cell_load()** and similar functions return the number of bytes loaded from the
EGC, or -1 on failure. They can fail due to either an invalid UTF-8 input, or the
backing egcpool reaching its maximum size.

**cell_set_fg_rgb()** and similar functions will return -1 if provided invalid
inputs, and 0 otherwise.

# SEE ALSO

**notcurses(3)**,
**notcurses_channels(3)**,
**notcurses_plane(3)**,
**notcurses_output(3)**
