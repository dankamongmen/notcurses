from cffi import FFI
ffibuild = FFI()

ffibuild.set_source(
    "_notcurses",
    """
    #include <notcurses/direct.h>
    #include <notcurses/notcurses.h>
    """,
    libraries=["notcurses"],
)

ffibuild.cdef("""
typedef struct ncinput {
  char32_t id;     // Unicode codepoint
  int y;           // Y cell coordinate of event, -1 for undefined
  int x;           // X cell coordinate of event, -1 for undefined
  bool alt;        // Was Alt held during the event?
  bool shift;      // Was Shift held during the event?
  bool ctrl;       // Was Ctrl held during the event?
  uint64_t seqnum; // Monotonically increasing input event counter
} ncinput;
// sigset_t differs from system to system, annoying
// char32_t notcurses_getc(struct notcurses* n, const struct timespec* ts, sigset_t* sigmask, ncinput* ni);
int notcurses_inputready_fd(struct notcurses* n);
typedef struct cell {
  uint32_t gcluster;          // 4B → 4B
  uint8_t gcluster_backstop;  // 1B → 5B (8 bits of zero)
  uint8_t width;              // 1B → 6B (8 bits, width)
  uint16_t stylemask;         // 2B → 8B (16 bits of NCSTYLE_* attributes)
  uint64_t channels;          // + 8b == 16b
} cell;
typedef enum {
  NCLOGLEVEL_SILENT,  // default. print nothing once fullscreen service begins
  NCLOGLEVEL_PANIC,   // print diagnostics immediately related to crashing
  NCLOGLEVEL_FATAL,   // we're hanging around, but we've had a horrible fault
  NCLOGLEVEL_ERROR,   // we can't keep doin' this, but we can do other things
  NCLOGLEVEL_WARNING, // you probably don't want what's happening to happen
  NCLOGLEVEL_INFO,    // "standard information"
  NCLOGLEVEL_VERBOSE, // "detailed information"
  NCLOGLEVEL_DEBUG,   // this is honestly a bit much
  NCLOGLEVEL_TRACE,   // there's probably a better way to do what you want
} ncloglevel_e;
typedef struct notcurses_options {
  const char* termtype;
  FILE* renderfp;
  ncloglevel_e loglevel;
  int margin_t, margin_r, margin_b, margin_l;
  uint64_t flags;
} notcurses_options;
struct notcurses* notcurses_init(const notcurses_options*, FILE*);
void notcurses_version_components(int* major, int* minor, int* patch, int* tweak);
int notcurses_lex_margins(const char* op, notcurses_options* opts);
int notcurses_stop(struct notcurses*);
int notcurses_render(struct notcurses* nc);
int ncpile_render(struct ncplane* n);
int ncpile_rasterize(struct ncplane* n);
int notcurses_render_to_buffer(struct notcurses* nc, char** buf, size_t* buflen);
int notcurses_render_to_file(struct notcurses* nc, FILE* fp);
struct ncplane* notcurses_stdplane(struct notcurses*);
const struct ncplane* notcurses_stdplane_const(const struct notcurses* nc);
void ncplane_set_channels(struct ncplane* n, uint64_t channels);
uint64_t ncplane_set_fchannel(struct ncplane* n, uint32_t channel);
uint64_t ncplane_set_bchannel(struct ncplane* n, uint32_t channel);
int ncplane_set_base_cell(struct ncplane* ncp, const cell* c);
int ncplane_set_base(struct ncplane* ncp, const char* egc, uint32_t styles, uint64_t channels);
int ncplane_base(struct ncplane* ncp, cell* c);
struct ncplane* notcurses_top(struct notcurses* n);
struct ncplane* notcurses_bottom(struct notcurses* n);
void notcurses_drop_planes(struct notcurses* nc);
int notcurses_refresh(struct notcurses* n, int* restrict y, int* restrict x);
struct ncplane* ncplane_reparent(struct ncplane* n, struct ncplane* newparent);
typedef enum {
  NCALIGN_LEFT,
  NCALIGN_CENTER,
  NCALIGN_RIGHT,
} ncalign_e;
typedef struct ncplane_options {
  int y;            // vertical placement relative to parent plane
  int x;            // horizontal placement relative to parent plane
  int rows;         // number of rows, must be positive
  int cols;         // number of columns, must be positive
  void* userptr;    // user curry, may be NULL
  const char* name; // name (used only for debugging), may be NULL
  int (*resizecb)(struct ncplane*); // callback when parent is resized
  uint64_t flags;   // closure over NCPLANE_OPTION_*
} ncplane_options;
struct ncplane* ncplane_create(struct ncplane* n, const ncplane_options* nopts);
unsigned notcurses_supported_styles(const struct notcurses* nc);
unsigned notcurses_palette_size(const struct notcurses* nc);
bool notcurses_cantruecolor(const struct notcurses* nc);
bool notcurses_canfade(const struct notcurses* nc);
bool notcurses_canchangecolor(const struct notcurses* nc);
bool notcurses_canopen_images(const struct notcurses* nc);
bool notcurses_canopen_videos(const struct notcurses* nc);
bool notcurses_canutf8(const struct notcurses* nc);
bool notcurses_cansixel(const struct notcurses* nc);
int notcurses_mouse_enable(struct notcurses* n);
int notcurses_mouse_disable(struct notcurses* n);
int ncplane_destroy(struct ncplane* ncp);
int ncplane_mergedown(const struct ncplane* src, struct ncplane* dst, int begsrcy, int begsrcx, int leny, int lenx, int dsty, int dstx);
int ncplane_mergedown_simple(const struct ncplane* restrict src, struct ncplane* restrict dst);
void ncplane_erase(struct ncplane* n);
int ncplane_cursor_move_yx(struct ncplane* n, int y, int x);
void ncplane_cursor_yx(struct ncplane* n, int* y, int* x);
int ncplane_move_yx(struct ncplane* n, int y, int x);
void ncplane_yx(struct ncplane* n, int* y, int* x);
int ncplane_y(const struct ncplane* n);
int ncplane_x(const struct ncplane* n);
void ncplane_dim_yx(const struct ncplane* n, int* rows, int* cols);
int ncplane_putc_yx(struct ncplane* n, int y, int x, const cell* c);
void ncplane_move_top(struct ncplane* n);
void ncplane_move_bottom(struct ncplane* n);
int ncplane_move_below(struct ncplane* restrict n, struct ncplane* restrict below);
int ncplane_move_above(struct ncplane* restrict n, struct ncplane* restrict above);
struct ncplane* ncplane_below(struct ncplane* n);
struct ncplane* ncplane_above(struct ncplane* n);
char* notcurses_at_yx(struct notcurses* nc, int yoff, int xoff, uint16_t* stylemask, uint64_t* channels);
char* ncplane_at_cursor(struct ncplane* n, uint16_t* stylemask, uint64_t* channels);
char* ncplane_at_yx(const struct ncplane* n, int y, int x, uint16_t* stylemask, uint64_t* channels);
typedef enum {
  NCBLIT_1x1,
  NCBLIT_2x1,
  NCBLIT_2x2,
  NCBLIT_3x2,
  NCBLIT_4x1,
  NCBLIT_BRAILLE,
  NCBLIT_8x1,
  NCBLIT_SIXEL,
} ncblitter_e;
const char* notcurses_str_blitter(ncblitter_e blitter);
int notcurses_lex_blitter(const char* op, ncblitter_e* blitter);
uint32_t* ncplane_rgba(const struct ncplane* nc, ncblitter_e blit, int begy, int begx, int leny, int lenx);
char* ncplane_contents(const struct ncplane* nc, int begy, int begx, int leny, int lenx);
void* ncplane_set_userptr(struct ncplane* n, void* opaque);
void* ncplane_userptr(struct ncplane* n);
int ncplane_resize(struct ncplane* n, int keepy, int keepx, int keepleny, int keeplenx, int yoff, int xoff, int ylen, int xlen);
uint64_t ncplane_channels(const struct ncplane* n);
int ncplane_set_fg_rgb8(struct ncplane* n, int r, int g, int b);
int ncplane_set_bg_rgb8(struct ncplane* n, int r, int g, int b);
void ncplane_set_fg_rgb8_clipped(struct ncplane* n, int r, int g, int b);
void ncplane_set_bg_rgb8_clipped(struct ncplane* n, int r, int g, int b);
int ncplane_set_fg_rgb(struct ncplane* n, unsigned channel);
int ncplane_set_bg_rgb(struct ncplane* n, unsigned channel);
void ncplane_set_fg_default(struct ncplane* n);
void ncplane_set_bg_default(struct ncplane* n);
int ncplane_set_fg_alpha(struct ncplane* n, unsigned alpha);
int ncplane_set_bg_alpha(struct ncplane* n, unsigned alpha);
int ncplane_set_fg_palindex(struct ncplane* n, int idx);
int ncplane_set_bg_palindex(struct ncplane* n, int idx);
unsigned ncplane_styles(const struct ncplane* n);
void ncplane_set_styles(struct ncplane* n, unsigned stylebits);
void ncplane_on_styles(struct ncplane* n, unsigned stylebits);
void ncplane_off_styles(struct ncplane* n, unsigned stylebits);
typedef struct ncstats {
  uint64_t renders;
  uint64_t failed_renders;
  uint64_t render_bytes;
  int64_t render_max_bytes;
  int64_t render_min_bytes;
  uint64_t render_ns;
  int64_t render_max_ns;
  int64_t render_min_ns;
  uint64_t cellelisions;
  uint64_t cellemissions;
  uint64_t fbbytes;
  uint64_t fgelisions;
  uint64_t fgemissions;
  uint64_t bgelisions;
  uint64_t bgemissions;
  uint64_t defaultelisions;
  uint64_t defaultemissions;
  uint64_t refreshes;
} ncstats;
ncstats* notcurses_stats_alloc(struct notcurses* nc);
void notcurses_stats(struct notcurses* nc, ncstats* stats);
void notcurses_stats_reset(struct notcurses* nc, ncstats* stats);
int ncplane_hline_interp(struct ncplane* n, const cell* c, int len, uint64_t c1, uint64_t c2);
int ncplane_vline_interp(struct ncplane* n, const cell* c, int len, uint64_t c1, uint64_t c2);
int ncplane_box(struct ncplane* n, const cell* ul, const cell* ur, const cell* ll, const cell* lr, const cell* hline, const cell* vline, int ystop, int xstop, unsigned ctlword);
typedef int (*fadecb)(struct notcurses* nc, struct ncplane* ncp, const struct timespec* ts, void* curry);
int ncplane_fadeout(struct ncplane* n, const struct timespec* ts, fadecb fader, void* curry);
int ncplane_fadein(struct ncplane* n, const struct timespec* ts, fadecb fader, void* curry);
int ncplane_pulse(struct ncplane* n, const struct timespec* ts, fadecb fader, void* curry);
int ncplane_putegc_yx(struct ncplane* n, int y, int x, const char* gclust, int* sbytes);
int ncplane_putstr_aligned(struct ncplane* n, int y, ncalign_e align, const char* s);
int ncplane_putstr_stained(struct ncplane* n, const char* s);
int ncplane_putwstr_stained(struct ncplane* n, const wchar_t* gclustarr);
struct ncplane* ncplane_dup(const struct ncplane* n, void* opaque);
int cell_load(struct ncplane* n, cell* c, const char* gcluster);
int cell_duplicate(struct ncplane* n, cell* targ, const cell* c);
void cell_release(struct ncplane* n, cell* c);
const char* cell_extended_gcluster(const struct ncplane* n, const cell* c);
typedef struct palette256 {
  // We store the RGB values as a regular ol' channel
  uint32_t chans[256];
} palette256;
palette256* palette256_new(struct notcurses* nc);
int palette256_use(struct notcurses* nc, const palette256* p);
void palette256_free(palette256* p);
typedef enum {
  NCSCALE_NONE,
  NCSCALE_SCALE,
  NCSCALE_STRETCH,
} ncscale_e;
int notcurses_lex_scalemode(const char* op, ncscale_e* scalemode);
const char* notcurses_str_scalemode(ncscale_e scalemode);
struct ncvisual* ncvisual_from_file(const char* file);
struct ncvisual* ncvisual_from_rgba(const void* rgba, int rows, int rowstride, int cols);
struct ncvisual* ncvisual_from_bgra(const void* rgba, int rows, int rowstride, int cols);
struct ncvisual* ncvisual_from_plane(const struct ncplane* n, ncblitter_e blit, int begy, int begx, int leny, int lenx);
int ncvisual_geom(const struct notcurses* nc, const struct ncvisual* n, const struct ncvisual_options* vopts, int* y, int* x, int* toy, int* tox);
void ncvisual_destroy(struct ncvisual* ncv);
int ncvisual_decode(struct ncvisual* nc);
int ncvisual_decode_loop(struct ncvisual* nc);
int ncvisual_rotate(struct ncvisual* n, double rads);
int ncvisual_resize(struct ncvisual* n, int rows, int cols);
int ncvisual_polyfill_yx(struct ncvisual* n, int y, int x, uint32_t rgba);
struct ncplane* ncvisual_render(struct notcurses* nc, struct ncvisual* ncv, const struct ncvisual_options* vopts);
char* ncvisual_subtitle(const struct ncvisual* ncv);
int ncvisual_at_yx(const struct ncvisual* n, int y, int x, uint32_t* pixel);
int ncvisual_set_yx(const struct ncvisual* n, int y, int x, uint32_t pixel);
typedef int (*streamcb)(struct ncvisual*, struct ncvisual_options*, const struct timespec*, void*);
int ncvisual_stream(struct notcurses* nc, struct ncvisual* ncv, float timescale, streamcb streamer, const struct ncvisual_options* vopts, void* curry);
struct ncvisual_options {
  struct ncplane* n;
  ncscale_e scaling;
  int y, x;
  int begy, begx;
  int leny, lenx;
  ncblitter_e blitter;
  uint64_t flags;
};
int ncblit_bgrx(const void* data, int linesize, const struct ncvisual_options *vopts);
int ncblit_rgba(const void* data, int linesize, const struct ncvisual_options *vopts);
struct ncselector_item {
  char* option;
  char* desc;
  size_t opcolumns;   // filled in by library
  size_t desccolumns; // filled in by library
};
typedef struct ncselector_options {
  char* title; // title may be NULL, inhibiting riser, saving two rows.
  char* secondary; // secondary may be NULL
  char* footer; // footer may be NULL
  struct ncselector_item* items; // initial items and descriptions
  // default item (selected at start), must be < itemcount unless itemcount
  // is 0, in which case 'defidx' must also be 0
  unsigned defidx;
  // maximum number of options to display at once, 0 to use all available space
  unsigned maxdisplay;
  // exhaustive styling options
  uint64_t opchannels;   // option channels
  uint64_t descchannels; // description channels
  uint64_t titlechannels;// title channels
  uint64_t footchannels; // secondary and footer channels
  uint64_t boxchannels;  // border channels
  uint64_t flags;        // bitmap over NCSELECTOR_OPTIONS_*
} ncselector_options;
struct ncselector* ncselector_create(struct ncplane* n, const ncselector_options* opts);
int ncselector_additem(struct ncselector* n, const struct ncselector_item* item);
int ncselector_delitem(struct ncselector* n, const char* item);
const char* ncselector_selected(const struct ncselector* n);
struct ncplane* ncselector_plane(struct ncselector* n);
const char* ncselector_previtem(struct ncselector* n);
const char* ncselector_nextitem(struct ncselector* n);
bool ncselector_offer_input(struct ncselector* n, const struct ncinput* nc);
void ncselector_destroy(struct ncselector* n, char** item);
struct ncmselector_item {
  char* option;
  char* desc;
  bool selected;
};
typedef struct ncmultiselector_options {
  char* title; // title may be NULL, inhibiting riser, saving two rows.
  char* secondary; // secondary may be NULL
  char* footer; // footer may be NULL
  struct ncmselector_item* items; // initial items, descriptions, and statuses
  // maximum number of options to display at once, 0 to use all available space
  unsigned maxdisplay;
  // exhaustive styling options
  uint64_t opchannels;   // option channels
  uint64_t descchannels; // description channels
  uint64_t titlechannels;// title channels
  uint64_t footchannels; // secondary and footer channels
  uint64_t boxchannels;  // border channels
  uint64_t flags;        // bitmap over NCSELECTOR_OPTIONS_*
} ncmultiselector_options;
struct ncmultiselector* ncmultiselector_create(struct ncplane* n, const ncmultiselector_options* opts);
int ncmultiselector_selected(struct ncmultiselector* n, bool* selected, unsigned count);
struct ncplane* ncmultiselector_plane(struct ncmultiselector* n);
bool ncmultiselector_offer_input(struct ncmultiselector* n, const struct ncinput* nc);
void ncmultiselector_destroy(struct ncmultiselector* n);
struct ncmenu_item {
  char* desc;           // utf-8 menu item, NULL for horizontal separator
  ncinput shortcut;     // shortcut, all should be distinct
};
struct ncmenu_section {
  char* name;             // utf-8 c string
  int itemcount;
  struct ncmenu_item* items;
  ncinput shortcut;       // shortcut, will be underlined if present in name
};
typedef struct ncmenu_options {
  struct ncmenu_section* sections; // array of 'sectioncount' menu_sections
  int sectioncount;         // must be positive
  uint64_t headerchannels;  // styling for header
  uint64_t sectionchannels; // styling for sections
  unsigned flags;           // bitfield over NCMENU_OPTION_*
} ncmenu_options;
struct ncmenu* ncmenu_create(struct ncplane* n, const ncmenu_options* opts);
int ncmenu_unroll(struct ncmenu* n, int sectionidx);
int ncmenu_rollup(struct ncmenu* n);
int ncmenu_nextsection(struct ncmenu* n);
int ncmenu_prevsection(struct ncmenu* n);
int ncmenu_nextitem(struct ncmenu* n);
int ncmenu_previtem(struct ncmenu* n);
int ncmenu_item_set_status(struct ncmenu* n, const char* section, const char* item, bool enabled);
const char* ncmenu_selected(const struct ncmenu* n, struct ncinput* ni);
bool ncmenu_offer_input(struct ncmenu* n, const struct ncinput* nc);
int ncmenu_destroy(struct ncmenu* n);
const char* ncmetric(uintmax_t val, unsigned decimal, char* buf, int omitdec, unsigned mult, int uprefix);
typedef struct ncreel_options {
  unsigned bordermask;
  uint64_t borderchan;
  unsigned tabletmask;
  uint64_t tabletchan;
  uint64_t focusedchan;
  unsigned flags;      // bitfield over NCREEL_OPTION_*
} ncreel_options;
struct ncreel* ncreel_create(struct ncplane* nc, const ncreel_options* popts);
struct ncplane* ncreel_plane(struct ncreel* pr);
typedef int (*tabletcb)(struct nctablet* t, bool cliptop);
struct nctablet* ncreel_add(struct ncreel* pr, struct nctablet* after, struct nctablet* before, tabletcb cb, void* opaque);
int ncreel_tabletcount(const struct ncreel* pr);
int ncreel_del(struct ncreel* pr, struct nctablet* t);
int ncreel_redraw(struct ncreel* pr);
struct nctablet* ncreel_focused(struct ncreel* pr);
struct nctablet* ncreel_next(struct ncreel* pr);
struct nctablet* ncreel_prev(struct ncreel* pr);
void ncreel_destroy(struct ncreel* pr);
void* nctablet_userptr(struct nctablet* t);
struct ncplane* nctablet_plane(struct nctablet* t);
int ncplane_polyfill_yx(struct ncplane* n, int y, int x, const cell* c);
int ncplane_gradient(struct ncplane* n, const char* egc, uint32_t styles, uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr, int ystop, int xstop);
int ncplane_highgradient(struct ncplane* n, uint32_t ul, uint32_t ur, uint32_t ll, uint32_t lr, int ystop, int xstop);
int ncplane_highgradient_sized(struct ncplane* n, uint32_t ul, uint32_t ur, uint32_t ll, uint32_t lr, int ylen, int xlen);
int ncplane_putchar_stained(struct ncplane* n, char c);
int ncplane_putegc_stained(struct ncplane* n, const char* gclust, int* sbytes);
int ncplane_putwegc_stained(struct ncplane* n, const wchar_t* gclust, int* sbytes);
int ncplane_format(struct ncplane* n, int ystop, int xstop, uint32_t styles);
int ncplane_stain(struct ncplane* n, int ystop, int xstop, uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr);
int ncplane_rotate_cw(struct ncplane* n);
int ncplane_rotate_ccw(struct ncplane* n);
void ncplane_translate(const struct ncplane* src, const struct ncplane* dst, int* y, int* x);
bool ncplane_translate_abs(const struct ncplane* n, int* y, int* x);
typedef struct ncplot_options {
  uint64_t maxchannels;
  uint64_t minchannels;
  uint16_t legendstyle;
  ncblitter_e gridtype;
  uint64_t rangex;
  unsigned flags;
  const char* title;
  uint64_t flags;        // bitmap over NCPLOT_OPTIONS_*
} ncplot_options;
struct ncuplot* ncuplot_create(struct ncplane* n, const ncplot_options* opts, uint64_t miny, uint64_t maxy);
struct ncdplot* ncdplot_create(struct ncplane* n, const ncplot_options* opts, double miny, double maxy);
struct ncplane* ncuplot_plane(struct ncuplot* n);
struct ncplane* ncdplot_plane(struct ncdplot* n);
int ncuplot_add_sample(struct ncuplot* n, uint64_t x, uint64_t y);
int ncdplot_add_sample(struct ncdplot* n, uint64_t x, double y);
int ncuplot_set_sample(struct ncuplot* n, uint64_t x, uint64_t y);
int ncdplot_set_sample(struct ncdplot* n, uint64_t x, double y);
int ncuplot_sample(const struct ncuplot* n, uint64_t x, uint64_t* y);
int ncdplot_sample(const struct ncdplot* n, uint64_t x, double* y);
void ncuplot_destroy(struct ncuplot* n);
void ncdplot_destroy(struct ncdplot* n);
bool ncplane_set_scrolling(struct ncplane* n, bool scrollp);
typedef struct ncfdplane_options {
  void* curry; // parameter provided to callbacks
  bool follow; // keep reading after hitting end? (think tail -f)
  uint64_t flags;        // bitmap over NCPLOT_OPTIONS_*
} ncfdplane_options;
typedef int(ncfdplane_callback)(struct ncfdplane* n, const void* buf, size_t s, void* curry);
typedef int(ncfdplane_done_cb)(struct ncfdplane* n, int fderrno, void* curry);
struct ncfdplane* ncfdplane_create(struct ncplane* n, const ncfdplane_options* opts, int fd, ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn);
struct ncplane* ncfdplane_plane(struct ncfdplane* n);
int ncfdplane_destroy(struct ncfdplane* n);
typedef struct ncsubproc_options {
  void* curry;
  uint64_t restart_period;  // restart this many seconds after an exit (watch)
  uint64_t flags;        // bitmap over NCPLOT_OPTIONS_*
} ncsubproc_options;
struct ncsubproc* ncsubproc_createv(struct ncplane* n, const ncsubproc_options* opts, const char* bin, char* const arg[], ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn);
struct ncsubproc* ncsubproc_createvp(struct ncplane* n, const ncsubproc_options* opts, const char* bin, char* const arg[], ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn);
struct ncsubproc* ncsubproc_createvpe(struct ncplane* n, const ncsubproc_options* opts, const char* bin, char* const arg[], char* const env[], ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn);
struct ncplane* ncsubproc_plane(struct ncsubproc* n);
int ncsubproc_destroy(struct ncsubproc* n);
typedef struct ncreader_options {
  uint64_t tchannels; // channels used for input
  uint32_t tattrword; // attributes used for input
  unsigned flags;     // bitfield over NCREADER_OPTION_*
} ncreader_options;
struct ncreader* ncreader_create(struct ncplane* n, const ncreader_options* opts);
int ncreader_clear(struct ncreader* n);
struct ncplane* ncreader_plane(struct ncreader* n);
bool ncreader_offer_input(struct ncreader* n, const struct ncinput* ni);
char* ncreader_contents(const struct ncreader* n);
void ncreader_destroy(struct ncreader* n, char** contents);
int ncplane_puttext(struct ncplane* n, int y, ncalign_e align, const char* text, size_t* bytes);
int ncplane_putnstr_yx(struct ncplane* n, int y, int x, size_t s, const char* gclusters);
int ncplane_putnstr_aligned(struct ncplane* n, int y, ncalign_e align, size_t s, const char* gclustarr);
int ncplane_qrcode(struct ncplane* n, ncblitter_e blitter, int* ymax, int* xmax, const void* data, size_t len);
struct ncdirect* ncdirect_init(const char* termtype, FILE* fp, uint64_t flags);
int ncdirect_set_bg_rgb8(struct ncdirect* n, unsigned r, unsigned g, unsigned b);
int ncdirect_set_fg_rgb8(struct ncdirect* n, unsigned r, unsigned g, unsigned b);
unsigned ncdirect_palette_size(const struct ncdirect* nc);
int ncdirect_putstr(struct ncdirect* nc, uint64_t channels, const char* utf8);
int ncdirect_printf_aligned(struct ncdirect* n, int y, ncalign_e align, const char* fmt, ...);
int ncdirect_set_fg_rgb(struct ncdirect* n, unsigned rgb);
int ncdirect_set_bg_rgb(struct ncdirect* n, unsigned rgb);
int ncdirect_set_styles(struct ncdirect* n, unsigned stylebits);
int ncdirect_on_styles(struct ncdirect* n, unsigned stylebits);
int ncdirect_off_styles(struct ncdirect* n, unsigned stylebits);
int ncdirect_clear(struct ncdirect* n);
int ncdirect_stop(struct ncdirect* n);
int ncdirect_dim_x(const struct ncdirect* nc);
int ncdirect_dim_y(const struct ncdirect* nc);
int ncdirect_cursor_move_yx(struct ncdirect* n, int y, int x);
int ncdirect_cursor_enable(struct ncdirect* nc);
int ncdirect_cursor_disable(struct ncdirect* nc);
int ncdirect_cursor_up(struct ncdirect* nc, int num);
int ncdirect_cursor_left(struct ncdirect* nc, int num);
int ncdirect_cursor_right(struct ncdirect* nc, int num);
int ncdirect_cursor_down(struct ncdirect* nc, int num);
int ncdirect_flush(const struct ncdirect* nc);
int ncdirect_hline_interp(struct ncdirect* n, const char* egc, int len, uint64_t h1, uint64_t h2);
int ncdirect_vline_interp(struct ncdirect* n, const char* egc, int len, uint64_t h1, uint64_t h2);
int ncdirect_box(struct ncdirect* n, uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr, const wchar_t* wchars, int ylen, int xlen, unsigned ctlword);
int ncdirect_rounded_box(struct ncdirect* n, uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr, int ylen, int xlen, unsigned ctlword);
int ncdirect_double_box(struct ncdirect* n, uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr, int ylen, int xlen, unsigned ctlword);
bool ncdirect_canopen_images(const struct ncdirect* n);
bool ncdirect_canutf8(const struct ncdirect* n);
int ncdirect_render_image(struct ncdirect* n, const char* filename, ncalign_e align, ncblitter_e blitter, ncscale_e scale);
struct ncplane* ncplane_parent(struct ncplane* n);
const struct ncplane* ncplane_parent_const(const struct ncplane* n);
int notcurses_cursor_enable(struct notcurses* nc, int y, int x);
int notcurses_cursor_disable(struct notcurses* nc);
int ncreader_move_left(struct ncreader* n);
int ncreader_move_right(struct ncreader* n);
int ncreader_move_up(struct ncreader* n);
int ncreader_move_down(struct ncreader* n);
int ncreader_write_egc(struct ncreader* n, const char* egc);
int ncstrwidth(const char* text);
""")

if __name__ == "__main__":
    ffibuild.compile(verbose=True)
