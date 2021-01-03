% notcurses_cell(3)
% nick black <nickblack@linux.com>
% v2.1.3

# NAME

notcurses_cell - operations on nccell objects

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
// See DESCRIPTION below for information on EGC encoding
typedef struct nccell {
  uint32_t gcluster;          // 4B → 4B
  uint8_t gcluster_backstop;  // 1B → 5B (8 bits of zero)
  uint8_t reserved;           // 1B → 6B (8 reserved bits, ought be zero)
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
#define CELL_ALPHA_TRANSPARENT  0x2.1.3000ull
#define CELL_ALPHA_BLEND        0x10000000ull
#define CELL_ALPHA_OPAQUE       0x00000000ull
```

**void cell_init(nccell* ***c***);**

**int cell_load(struct ncplane* ***n***, nccell* ***c***, const char* ***gcluster***);**

**int cell_prime(struct ncplane* ***n***, nccell* ***c***, const char* ***gcluster***,
                 uint32_t ***stylemask***, uint64_t ***channels***);**

**int cell_duplicate(struct ncplane* ***n***, nccell* ***targ***, const nccell* ***c***);**

**void cell_release(struct ncplane* ***n***, nccell* ***c***);**

**void cell_styles_set(nccell* ***c***, unsigned ***stylebits***);**

**unsigned cell_styles(const nccell* ***c***);**

**void cell_on_styles(nccell* ***c***, unsigned ***stylebits***);**

**void cell_off_styles(nccell* ***c***, unsigned ***stylebits***);**

**void cell_set_fg_default(nccell* ***c***);**

**void cell_set_bg_default(nccell* ***c***);**

**int cell_set_fg_alpha(nccell* ***c***, unsigned ***alpha***);**

**int cell_set_bg_alpha(nccell* ***c***, unsigned ***alpha***);**

**bool cell_double_wide_p(const nccell* ***c***);**

**const char* cell_extended_gcluster(const struct ncplane* ***n***, const nccell* ***c***);**

**char* cell_strdup(const struct ncplane* ***n***, const nccell* ***c***);**

**int cell_load_char(struct ncplane* ***n***, nccell* ***c***, char ***ch***);**

**int cell_load_egc32(struct ncplane* ***n***, nccell* ***c***, uint32_t ***egc***);**

**char* cell_extract(const struct ncplane* ***n***, const nccell* ***c***, uint16_t* ***stylemask***, uint64_t* ***channels***);**

**uint32_t cell_bchannel(const nccell* ***c***);**

**uint32_t cell_fchannel(const nccell* ***c***);**

**uint64_t cell_set_bchannel(nccell* ***c***, uint32_t ***channel***);**

**uint64_t cell_set_fchannel(nccell* ***c***, uint32_t ***channel***);**

**uint32_t cell_fg_rgb(const nccell* ***c***);**

**uint32_t cell_bg_rgb(const nccell* ***c***);**

**unsigned cell_fg_alpha(const nccell* ***c***);**

**unsigned cell_bg_alpha(const nccell* ***c***);**

**unsigned cell_fg_rgb8(const nccell* ***c***, unsigned* ***r***, unsigned* ***g***, unsigned* ***b***);**

**unsigned cell_bg_rgb8(const ncell* ***c***, unsigned* ***r***, unsigned* ***g***, unsigned* ***b***);**

**int cell_set_fg_rgb8(nccell* ***c***, int ***r***, int ***g***, int ***b***);**

**int cell_set_bg_rgb8(nccell* ***c***, int ***r***, int ***g***, int ***b***);**

**void cell_set_fg_rgb8_clipped(nccell* ***c***, int ***r***, int ***g***, int ***b***);**

**void cell_set_bg_rgb8_clipped(nccell* ***c***, int ***r***, int ***g***, int ***b***);**

**int cell_set_fg_rgb(nccell* ***c***, uint32_t ***channel***);**

**int cell_set_bg_rgb(nccell* ***c***, uint32_t ***channel***);**

**bool cell_fg_default_p(const nccell* ***c***);**

**bool cell_bg_default_p(const nccell* ***c***);**

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
must be considered associated with **ncplane**s. Indeed, **ncplane_erase()**
destroys the backing storage for all a plane's cells, invalidating them. This
association is formed at the time of **cell_load()**, **cell_prime()**, or
**cell_duplicate()**. All of these functions first call **cell_release()**, as
does **cell_load_simple()**. When done using a **nccell** entirely, call
**cell_release()**. **ncplane_destroy()** will free up the memory used by the
**nccell**, but the backing egcpool has a maximum size of 16MiB, and failure to
release **nccell**s can eventually block new output.

**cell_extended_gcluster** provides a nul-terminated handle to the EGC. This
ought be considered invalidated by changes to the **nccell** or **egcpool**.
The handle is **not** heap-allocated; do **not** attempt to **free(3)** it.
A heap-allocated copy can be acquired with **cell_strdup**.

# RETURN VALUES

**cell_load()** and similar functions return the number of bytes loaded from the
EGC, or -1 on failure. They can fail due to either an invalid UTF-8 input, or the
backing egcpool reaching its maximum size.

**cell_set_fg_rgb8()** and similar functions will return -1 if provided invalid
inputs, and 0 otherwise.

# NOTES

**cell** was renamed to **nccell** in Notcurses 2.1.3, so as not to bleed such
a common term into the (single, global) C namespace. **cell** will be retained
as an alias until Notcurses 3.0.

# SEE ALSO

**notcurses(3)**,
**notcurses_channels(3)**,
**notcurses_plane(3)**,
**notcurses_output(3)**
