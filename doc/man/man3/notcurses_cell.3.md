% notcurses_cell(3)
% nick black <nickblack@linux.com>
% v2.3.4

# NAME

notcurses_cell - operations on nccell objects

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
// See DESCRIPTION below for information on EGC encoding
typedef struct nccell {
  uint32_t gcluster;          // 4B → 4B
  uint8_t gcluster_backstop;  // 1B → 5B (8 bits of zero)
  uint8_t width;              // 1B → 6B (8 bits of column width)
  uint16_t stylemask;         // 2B → 8B (16 bits of NCSTYLE_* attributes)
  uint64_t channels;
} nccell;

#define CELL_TRIVIAL_INITIALIZER \
 { .gcluster = '\0', .stylemask = 0, .channels = 0, }
#define CELL_SIMPLE_INITIALIZER(c) \
 { .gcluster = (c), .stylemask = 0, .channels = 0, }
#define CELL_INITIALIZER(c, s, chan) \
 { .gcluster = (c), .stylemask = (s), .channels = (chan), }

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

**void nccell_init(nccell* ***c***);**

**int nccell_load(struct ncplane* ***n***, nccell* ***c***, const char* ***gcluster***);**

**int nccell_prime(struct ncplane* ***n***, nccell* ***c***, const char* ***gcluster***,
                 uint32_t ***stylemask***, uint64_t ***channels***);**

**int nccell_duplicate(struct ncplane* ***n***, nccell* ***targ***, const nccell* ***c***);**

**void nccell_release(struct ncplane* ***n***, nccell* ***c***);**

**int nccell_width(const struct ncplane* ***n***, const nccell* ***c***);**

**void nccell_styles_set(nccell* ***c***, unsigned ***stylebits***);**

**unsigned nccell_styles(const nccell* ***c***);**

**bool nccellcmp(const struct ncplane* ***n1***, const nccell* ***c1***, const struct ncplane* ***n2***, const nccell* ***c2***);**

**void nccell_on_styles(nccell* ***c***, unsigned ***stylebits***);**

**void nccell_off_styles(nccell* ***c***, unsigned ***stylebits***);**

**void nccell_set_fg_default(nccell* ***c***);**

**void nccell_set_bg_default(nccell* ***c***);**

**int nccell_set_fg_alpha(nccell* ***c***, unsigned ***alpha***);**

**int nccell_set_bg_alpha(nccell* ***c***, unsigned ***alpha***);**

**bool nccell_double_wide_p(const nccell* ***c***);**

**const char* nccell_extended_gcluster(const struct ncplane* ***n***, const nccell* ***c***);**

**char* nccell_strdup(const struct ncplane* ***n***, const nccell* ***c***);**

**int nccell_load_char(struct ncplane* ***n***, nccell* ***c***, char ***ch***);**

**int nccell_load_egc32(struct ncplane* ***n***, nccell* ***c***, uint32_t ***egc***);**

**char* nccell_extract(const struct ncplane* ***n***, const nccell* ***c***, uint16_t* ***stylemask***, uint64_t* ***channels***);**

**uint32_t nccell_bchannel(const nccell* ***c***);**

**uint32_t nccell_fchannel(const nccell* ***c***);**

**uint64_t nccell_set_bchannel(nccell* ***c***, uint32_t ***channel***);**

**uint64_t nccell_set_fchannel(nccell* ***c***, uint32_t ***channel***);**

**uint32_t nccell_fg_rgb(const nccell* ***c***);**

**uint32_t nccell_bg_rgb(const nccell* ***c***);**

**unsigned nccell_fg_alpha(const nccell* ***c***);**

**unsigned nccell_bg_alpha(const nccell* ***c***);**

**unsigned nccell_fg_rgb8(const nccell* ***c***, unsigned* ***r***, unsigned* ***g***, unsigned* ***b***);**

**unsigned nccell_bg_rgb8(const ncell* ***c***, unsigned* ***r***, unsigned* ***g***, unsigned* ***b***);**

**int nccell_set_fg_rgb8(nccell* ***c***, int ***r***, int ***g***, int ***b***);**

**int nccell_set_bg_rgb8(nccell* ***c***, int ***r***, int ***g***, int ***b***);**

**void nccell_set_fg_rgb8_clipped(nccell* ***c***, int ***r***, int ***g***, int ***b***);**

**void nccell_set_bg_rgb8_clipped(nccell* ***c***, int ***r***, int ***g***, int ***b***);**

**int nccell_set_fg_rgb(nccell* ***c***, uint32_t ***channel***);**

**int nccell_set_bg_rgb(nccell* ***c***, uint32_t ***channel***);**

**bool nccell_fg_default_p(const nccell* ***c***);**

**bool nccell_bg_default_p(const nccell* ***c***);**

**int ncstrwidth(const char* ***text***)**;

# DESCRIPTION

Cells make up the framebuffer associated with each plane, with one cell per
addressable coordinate. You should not usually need to interact directly
with **nccell**s.

Each **nccell** contains exactly one extended grapheme cluster. If the EGC
is encoded in UTF-8 with four or fewer bytes (all Unicode codepoints as of
Unicode 13 can be encoded in no more than four UTF-8 bytes), it is encoded
directly into the **nccell**'s **gcluster** field, and no additional storage
is necessary. Otherwise, the EGC is stored as a nul-terminated UTF-8 string in
some backing egcpool. Egcpools are associated with **ncplane**s, so **nccell**s
must be considered associated with **ncplane**s. Indeed, **ncplane_erase**
destroys the backing storage for all a plane's cells, invalidating them. This
association is formed at the time of **nccell_load**, **nccell_prime**, or
**nccell_duplicate**. All of these functions first call **nccell_release**, as
do **nccell_load_egc32** and **nccell_load_char**. When done using a **nccell**
entirely, call **nccell_release**. **ncplane_destroy** will free up the memory
used by the **nccell**, but the backing egcpool has a maximum size of 16MiB,
and failure to release **nccell**s can eventually block new output.

**nccell_extended_gcluster** provides a nul-terminated handle to the EGC. This
ought be considered invalidated by changes to the **nccell** or **egcpool**.
The handle is **not** heap-allocated; do **not** attempt to **free(3)** it.
A heap-allocated copy can be acquired with **nccell_strdup**.

# RETURN VALUES

**nccell_load** and similar functions return the number of bytes loaded from the
EGC, or -1 on failure. They can fail due to either an invalid UTF-8 input, or the
backing egcpool reaching its maximum size.

**nccell_set_fg_rgb8** and similar functions will return -1 if provided invalid
inputs, and 0 otherwise.

**nccellcmp** returns a negative integer, 0, or a positive integer if ***c1*** is
less than, equal to, or more than ***c2***, respectively.

**nccell_extended_gcluster** returns **NULL** if called on a sprixel (see
**notcurses_visual(3)**.

**nccell_width** returns the number of columns occupied by ***c***, according
to **wcwidth(3)***. **ncstrwidth** is an equivalent for strings.

# NOTES

**cell** was renamed to **nccell** in Notcurses 2.2.0, so as not to bleed such
a common term into the (single, global) C namespace. **cell** will be retained
as an alias until Notcurses 3.0. Likewise, many functions prefixed with **cell**
have been renamed to start with **nccell**.

# SEE ALSO

**notcurses(3)**,
**notcurses_channels(3)**,
**notcurses_plane(3)**,
**notcurses_output(3)**,
**notcurses_visual(3)**,
**wcwidth(3)**
