#ifndef NOTCURSES_INTERNAL
#define NOTCURSES_INTERNAL

#ifdef __cplusplus
extern "C" {
#endif

#include "version.h"
#include "builddef.h"

#include <term.h>
#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <wctype.h>
#include <pthread.h>
#include <termios.h>
#include <stdbool.h>
#include <unictype.h>
#include <langinfo.h>
#include <netinet/in.h>
#include "notcurses/notcurses.h"
#include "compat/compat.h"
#include "termdesc.h"
#include "egcpool.h"

#define API __attribute__((visibility("default")))
#define ALLOC __attribute__((malloc)) __attribute__((warn_unused_result))

struct esctrie;
struct sixelmap;
struct ncvisual_details;

// Does this glyph completely obscure the background? If so, there's no need
// to emit a background when rasterizing, a small optimization.
#define CELL_NOBACKGROUND_MASK  0x8700000000000000ull

// Was this glyph drawn as part of an ncvisual? If so, we need to honor
// blitter stacking rather than the standard trichannel solver.
#define CELL_BLITTERSTACK_MASK  CELL_NOBACKGROUND_MASK

// we can't define multipart ncvisual here, because OIIO requires C++ syntax,
// and we can't go throwing C++ syntax into this header. so it goes.

typedef enum {
  SPRIXEL_QUIESCENT,   // sprixel has been drawn
  SPRIXEL_INVALIDATED, // sprixel needs to be redrawn
  SPRIXEL_HIDE,        // sprixel queued for destruction
  SPRIXEL_MOVED,       // sprixel needs be moved
} sprixel_e;

// elements of the T-A matrix describe transparency and annihilation at a
// per-cell basis, making up something of a state machine. when a sprixel
// plane is first created, the TAM is (meaninglessly) initialized to all
// zeroes (SPRIXCELL_OPAQUE). during the construction of the sprixel from
// an RGBA frame, OPAQUE entries are possibly marked MIXED or TRANSPARENT.
// subsequent sprixels blitted to the same plane will reuse the TAM, and
// retain any SPRIXCELL_ANNIHILATED entries, cutting them out of the
// sprixel.
//
// sixel can transition to ANNIHILATED via a no-op; kitty can transition
// to ANNIHILATED only by wiping the cell (removing it from the sprixel via
// all-0 alphas), deleting the bitmap, and displaying it once more. sixel
// bitmaps are removed by obliterating them with new output, while kitty
// bitmaps are removed by a fixed-length terminal escape. an important
// implication is that sixels cannot be progressively reduced by emitting
// progressively more transparent sixels atop one another--to remove a
// cell from a Sixel sprixel, it is necessary to print a glyph. the same
// goes for Kitty sprixels, but there we delete and rerender bitmaps
// in toto without glyph involvement.
//
// a glyph above an OPAQUE sprixel requires annihilating the underlying cell,
// and emitting the glyph only after annihilation is complete. a glyph below
// an OPAQUE sprixel should never be emitted (update the lastframe to
// contain it, but do not mark the cell damaged). should the sprixel be
// removed, the cell will be marked damaged, and the glyph will be updated.
//
// a glyph above a MIXED sprixcell requires the same process as one above an
// OPAQUE sprixcell. a glyph below a MIXED sprixcell can be emitted, but a
// Sixel-based sprixel must then be printed afresh. a Kitty-based sprixel
// needn't be touched in this case.
//
// a glyph above a TRANSPARENT sprixcell requires annihilating the underlying
// cell, but this is a special annihilation which never requires a wipe nor
// redisplay, just the O(1) state transition. a glyph below a TRANSPARENT
// sprixcell can be emitted with no change to the sprixcell. TRANSPARENT
// sprixcells move to ANNIHILATED_TRANS upon annihilation.
//
// a glyph above an ANNIHILATED sprixcell can be emitted with no change to
// the sprixcell. it does not make sense to emit a glyph below an ANNIHILATED
// sprixcell; if there is no longer a glyph above the sprixcell, the sprixcell
// must transition back to its original state (see below).
//
// rendering a new RGBA frame into the same sprixel plane can result in changes
// between OPAQUE, MIXED, and TRANSPARENT. an OPAQUE sprixcell which becomes
// TRANSPARENT or MIXED upon rendering a new RGBA frame must damage its cell,
// since the glyph underneath might have changed without being emitted. the
// new glyph must be emitted prior to redisplay of the sprixel.
//
// an ANNIHILATED sprixcell with no glyph above it must be restored to its
// original form (from the most recent RGBA frame). this requires the original
// pixel data. for Sixel, we must keep the palette indices in an auxiliary
// vector, hung off the TAM, updated each time we convert an RGBA frame into a
// partially- or wholly-ANNIHILATED sprixel. for Kitty, we must keep the
// original alpha values. the new state can be solved from this data. if the
// new state is either OPAQUE or MIXED, the sprixel must be redisplayed. if the
// new state is TRANSPARENT, this cell requires no such redisplay, and the
// payload needn't be modified. to special-case this O(1) conversion, we keep a
// distinct state, ANNIHILATED_TRANS. only a TRANSPARENT sprixcell can enter
// into this state.
//
// when a sprixel is removed from the rendering pile, in Sixel all cells it
// covered must be marked damaged, so that they are rendered, obliterating
// the bitmap. in Kitty the bitmap can simply be deleted, except for those
// cells which were SPRIXCELL_OPAQUE (they must be damaged).
//
// when a sprixel is moved, its TAM must be updated. OPAQUE, MIXED, and
// TRANSPARENT cells retain their entries. ANNIHILATED cells remain
// ANNIHILATED if their new absolute position corresponds to an ANNIHILATED
// cell; they otherwise transition back as outlined above. this is because
// ANNIHILATION is a property of those glyphs above us, while the other
// three are internal, intrinsic properties. for Sixel, all cells no longer
// covered must be damaged for rerendering, and the sprixel must subsequently
// be displayed at its new position. for Kitty, the sprixel must be deleted,
// and all cells no longer covered but which were previously under an OPAQUE
// cell must be damaged for rerendering (not to erase the bitmap, but because
// they might have changed without being emitted while obstructed by the
// sprixel). the sprixel should be displayed at its new position. using Kitty's
// bitmap movement is also acceptable, rather than a deletion and rerender.
// whichever method is used, it is necessary to recover any ANNIHILATED cells
// before moving or redisplaying the sprixel.
//
// all emissions take place at rasterization time. cell wiping happens at
// rendering time. cell reconstruction happens at rendering time (for
// ANNIHILATED cells which are no longer ANNIHILATED), or at blittime for
// a new RGBA frame.
typedef enum {
  SPRIXCELL_OPAQUE_SIXEL,      // no transparent pixels in this cell
  SPRIXCELL_OPAQUE_KITTY,
  SPRIXCELL_MIXED_SIXEL,       // this cell has both opaque and transparent pixels
  SPRIXCELL_MIXED_KITTY,
  SPRIXCELL_TRANSPARENT,       // all pixels are naturally transparent
  SPRIXCELL_ANNIHILATED,       // this cell has been wiped (all trans)
  SPRIXCELL_ANNIHILATED_TRANS, // this transparent cell is covered
} sprixcell_e;

// a TAM entry is a sprixcell_e state plus a possible auxiliary vector for
// reconstruction of annihilated cells, valid only for SPRIXCELL_ANNIHILATED.
typedef struct tament {
  sprixcell_e state;
  uint8_t* auxvector; // palette entries for sixel, alphas for kitty
} tament;

// a sprixel represents a bitmap, using whatever local protocol is available.
// there is a list of sprixels per ncpile. there ought never be very many
// associated with a context (a dozen or so at max). with the kitty protocol,
// we can register them, and then manipulate them by id. with the sixel
// protocol, we just have to rewrite them. there's a doubly-linked list of
// sprixels per ncpile, to which the pile keeps a head link.
typedef struct sprixel {
  char* glyph;          // glyph; can be quite large
  int glyphlen;         // length of the glyph in bytes
  uint32_t id;          // embedded into gcluster field of nccell, 24 bits
  // both the plane and visual can die before the sprixel does. they are
  // responsible in such a case for NULLing out this link themselves.
  struct ncplane* n;    // associated ncplane
  sprixel_e invalidated;// sprixel invalidation state
  struct sprixel* next;
  struct sprixel* prev;
  int dimy, dimx;       // cell geometry
  int pixy, pixx;       // pixel geometry (might be smaller than cell geo)
  int cellpxy, cellpxx; // cell-pixel geometry at time of creation
  // each tacache entry is one of 0 (standard opaque cell), 1 (cell with
  // some transparency), 2 (annihilated, excised)
  int movedfromy;       // for SPRIXEL_MOVED, the starting absolute position,
  int movedfromx;       // so that we can damage old cells when redrawn
  // only used for kitty-based sprixels
  int parse_start;      // where to start parsing for cell wipes
  // only used for sixel-based sprixels
  struct sixelmap* smap;  // copy of palette indices + transparency bits
  bool wipes_outstanding; // do we need rebuild the sixel next render?
} sprixel;

// A plane is memory for some rectilinear virtual window, plus current cursor
// state for that window, and part of a pile. Each pile has a total order along
// its z-axis. Functions update these virtual planes over a series of API
// calls. Eventually, notcurses_render() is called. We then do a depth buffer
// blit of updated cells. A cell is updated if the topmost plane including that
// cell updates it, not simply if any plane updates it.
//
// A plane may be partially or wholly offscreen--this might occur if the
// screen is resized, for example. Offscreen portions will not be rendered.
// Accesses beyond the borders of a panel, however, are errors.
//
// The framebuffer 'fb' is a set of rows. For scrolling, we interpret it as a
// circular buffer of rows. 'logrow' is the index of the row at the logical top
// of the plane. It only changes from 0 if the plane is scrollable.
typedef struct ncplane {
  nccell* fb;            // "framebuffer" of character cells
  int logrow;            // logical top row, starts at 0, add one for each scroll
  int x, y;              // current cursor location within this plane
  // ncplane_yx() etc. use coordinates relative to the plane to which this
  // plane is bound, but absx/absy are always relative to the terminal origin.
  // they must thus be translated by any function which moves a parent plane.
  int absx, absy;        // origin of the plane relative to the pile's origin
                         //  also used as left and top margin on resize by
                         //  ncplane_resize_marginalized()
  int lenx, leny;        // size of the plane, [0..len{x,y}) is addressable
  egcpool pool;          // attached storage pool for UTF-8 EGCs
  uint64_t channels;     // works the same way as cells

  // a notcurses context is made up of piles, each rooted by one or more root
  // planes. each pile has its own (totally ordered) z-axis.
  struct ncpile* pile;   // pile of which we are a part
  struct ncplane* above; // plane above us, NULL if we're on top
  struct ncplane* below; // plane below us, NULL if we're on bottom

  // every plane is bound to some other plane, unless it is a root plane of a
  // pile. a pile has a set of one or more root planes, all of them siblings.
  // root planes are bound to themselves. the standard plane is always a root
  // plane (since it cannot be reparented). a path exists to a root plane.
  struct ncplane* bnext;  // next in the blist
  struct ncplane** bprev; // blist link back to us
  struct ncplane* blist;  // head of list of bound planes
  struct ncplane* boundto;// plane to which we are bound (ourself for roots)

  sprixel* sprite;       // pointer into the sprixel cache
  tament* tam;           // transparency-annihilation sprite matrix

  void* userptr;         // slot for the user to stick some opaque pointer
  int (*resizecb)(struct ncplane*); // callback after parent is resized
  nccell basecell;       // cell written anywhere that fb[i].gcluster == 0
  char* name;            // used only for debugging
  ncalign_e halign;      // relative to parent plane, for automatic realignment
  ncalign_e valign;      // relative to parent plane, for automatic realignment
  uint16_t stylemask;    // same deal as in a cell
  int margin_b, margin_r;// bottom and right margins, stored for resize
  bool scrolling;        // is scrolling enabled? always disabled by default
} ncplane;

// current presentation state of the terminal. it is carried across render
// instances. initialize everything to 0 on a terminal reset / startup.
typedef struct rasterstate {
  // we assemble the encoded (rasterized) output in a POSIX memstream, and keep
  // it around between uses. this could be a problem if it ever tremendously
  // spiked, but that's a highly unlikely situation.
  char* mstream;  // buffer for rasterizing memstream, see open_memstream(3)
  FILE* mstreamfp;// FILE* for rasterizing memstream
  size_t mstrsize;// size of rendering memstream

  // the current cursor position. this is independent of whether the cursor is
  // visible. it is the cell at which the next write will take place. this is
  // modified by: output, cursor moves, clearing the screen (during refresh).
  int y, x;

  uint32_t curattr;// current attributes set (does not include colors)
  unsigned lastr;  // foreground rgb, overloaded for palindexed fg
  unsigned lastg;
  unsigned lastb;
  unsigned lastbr; // background rgb, overloaded for palindexed bg
  unsigned lastbg;
  unsigned lastbb;
  // we elide a color escape iff the color has not changed between two cells
  bool fgelidable;
  bool bgelidable;
  bool fgpalelidable;
  bool bgpalelidable;
  bool fgdefelidable;
  bool bgdefelidable;

  // need we do a hard cursor update (i.e. did we just emit a pixel graphic)?
  bool hardcursorpos;
} rasterstate;

// Tablets are the toplevel entitites within an ncreel. Each corresponds to
// a single, distinct ncplane.
typedef struct nctablet {
  ncplane* p;                  // border plane, NULL when offscreen
  ncplane* cbp;                // data plane, NULL when offscreen
  struct nctablet* next;
  struct nctablet* prev;
  tabletcb cbfxn;              // application callback to draw cbp
  void* curry;                 // application data provided to cbfxn
} nctablet;

typedef struct ncreel {
  ncplane* p;           // ncplane this ncreel occupies, under tablets
  // doubly-linked list, a circular one when infinity scrolling is in effect.
  // points at the focused tablet (when at least one tablet exists, one must be
  // focused). it will be visibly focused following the next redraw.
  nctablet* tablets;
  nctablet* vft;        // the visibly-focused tablet
  enum {
    LASTDIRECTION_UP,
    LASTDIRECTION_DOWN,
  } direction;          // last direction of travel
  int tabletcount;      // could be derived, but we keep it o(1)
  ncreel_options ropts; // copied in ncreel_create()
} ncreel;

// ncmenu_item and ncmenu_section have internal and (minimal) external forms
typedef struct ncmenu_int_item {
  char* desc;           // utf-8 menu item, NULL for horizontal separator
  ncinput shortcut;     // shortcut, all should be distinct
  int shortcut_offset;  // column offset with desc of shortcut EGC
  char* shortdesc;      // description of shortcut, can be NULL
  int shortdesccols;    // columns occupied by shortcut description
  bool disabled;        // disabled?
} ncmenu_int_item;

typedef struct ncmenu_int_section {
  char* name;             // utf-8 c string
  int itemcount;
  ncmenu_int_item* items; // items, NULL iff itemcount == 0
  ncinput shortcut;       // shortcut, will be underlined if present in name
  int xoff;               // column offset from beginning of menu bar
  int bodycols;           // column width of longest item
  int itemselected;       // current item selected, -1 for no selection
  int shortcut_offset;    // column offset within name of shortcut EGC
  int enabled_item_count; // number of enabled items: section is disabled iff 0
} ncmenu_int_section;

typedef struct ncfdplane {
  ncfdplane_callback cb;      // invoked with fresh hot data
  ncfdplane_done_cb donecb;   // invoked on EOF (if !follow) or error
  void* curry;                // passed to the callbacks
  int fd;                     // we take ownership of the fd, and close it
  bool follow;                // keep trying to read past the end (event-based)
  ncplane* ncp;               // bound ncplane
  pthread_t tid;              // thread servicing this i/o
  bool destroyed;             // set in ncfdplane_destroy() in our own context
} ncfdplane;

typedef struct ncsubproc {
  ncfdplane* nfp;
  pid_t pid;                  // subprocess
  int pidfd;                  // for signalling/watching the subprocess
  pthread_t waittid;          // wait()ing thread if pidfd is not available
  pthread_mutex_t lock;       // guards waited
  bool waited;                // we've wait()ed on it, don't kill/wait further
} ncsubproc;

typedef struct ncreader {
  ncplane* ncp;               // always owned by ncreader
  uint64_t tchannels;         // channels for input text
  uint32_t tattrs;            // attributes for input text
  ncplane* textarea;          // grows as needed iff scrolling is enabled
  int xproject;               // virtual x location of ncp origin on textarea
  bool horscroll;             // is there horizontal panning?
  bool no_cmd_keys;           // are shortcuts disabled?
  bool manage_cursor;         // enable and place a virtual cursor
} ncreader;

typedef struct ncmenu {
  ncplane* ncp;
  int sectioncount;         // must be positive
  ncmenu_int_section* sections; // NULL iff sectioncount == 0
  int unrolledsection;      // currently unrolled section, -1 if none
  int headerwidth;          // minimum space necessary to display all sections
  uint64_t headerchannels;  // styling for header
  uint64_t dissectchannels; // styling for disabled section headers
  uint64_t sectionchannels; // styling for sections
  uint64_t disablechannels; // styling for disabled entries
  bool bottom;              // are we on the bottom (vs top)?
} ncmenu;

typedef struct ncprogbar {
  ncplane* ncp;
  double progress;          // on the range [0, 1]
  uint32_t ulchannel, urchannel, blchannel, brchannel;
  bool retrograde;
} ncprogbar;

typedef struct nctab {
  struct nctabbed* nt; // The nctabbed this belongs to
  tabcb cb;     // tab callback
  char* name;   // tab name
  int namecols; // tab name width in columns
  void* curry;  // user pointer
  struct nctab* prev;
  struct nctab* next;
} nctab;

typedef struct nctabbed {
  ncplane* ncp;          // widget ncplane
  ncplane* p;            // tab content ncplane
  ncplane* hp;           // tab headers ncplane
  // a doubly-linked circular list of tabs
  nctab* leftmost;       // the tab most to the left
  nctab* selected;       // the currently selected tab
  int tabcount;          // tab separator (can be NULL)
  int sepcols;           // separator with in columns
  nctabbed_options opts; // copied in nctabbed_create()
} nctabbed;

typedef struct ncinputlayer {
  int ttyinfd;  // file descriptor for processing input
  unsigned char inputbuf[BUFSIZ];
  // we keep a wee ringbuffer of input queued up for delivery. if
  // inputbuf_occupied == sizeof(inputbuf), there is no room. otherwise, data
  // can be read to inputbuf_write_at until we fill up. the first datum
  // available for the app is at inputbuf_valid_starts iff inputbuf_occupied is
  // not 0. the main purpose is working around bad predictions of escapes.
  unsigned inputbuf_occupied;
  unsigned inputbuf_valid_starts;
  unsigned inputbuf_write_at;
  // number of input events seen. does not belong in ncstats, since it must not
  // be reset (semantics are relied upon by widgets for mouse click detection).
  uint64_t input_events;
  struct esctrie* inputescapes; // trie of input escapes -> ncspecial_keys
} ncinputlayer;

typedef struct ncdirect {
  palette256 palette;        // 256-indexed palette can be used instead of/with RGB
  FILE* ttyfp;               // FILE* for output tty
  int ctermfd;               // fd for controlling terminal
  tinfo tcache;              // terminfo cache
  uint64_t channels;         // current channels
  uint16_t stylemask;        // current styles
  ncinputlayer input;        // input layer; we're in cbreak mode
  struct termios tpreserved; // terminal state upon entry
  // some terminals (e.g. kmscon) return cursor coordinates inverted from the
  // typical order. we detect it the first time ncdirect_cursor_yx() is called.
  bool detected_cursor_inversion; // have we performed inversion testing?
  bool inverted_cursor;      // does the terminal return inverted coordinates?
  bool initialized_readline; // have we initialized Readline?
  uint64_t flags;            // copied in ncdirect_init() from param
} ncdirect;

// Extracellular state for a cell during the render process. There is one
// crender per rendered cell, and they are initialized to all zeroes.
struct crender {
  const ncplane *p; // source of glyph for this cell
  nccell c;
  uint32_t hcfg;       // fg channel prior to HIGHCONTRAST (need full channel)
  sprixel* sprixel;    // bitmap encountered during traversal
  struct {
    // If the glyph we render is from an ncvisual, and has a transparent or
    // blended background, blitter stacking is in effect. This is a complicated
    // issue, but essentially, imagine a bottom block is rendered with a green
    // bottom and transparent top. on a lower plane, a top block is rendered
    // with a red foreground and blue background. Normally, this would result
    // in a blue top and green bottom, but that's not what we ever wanted --
    // what makes sense is a red top and green bottom. So ncvisual rendering
    // sets bits from CELL_BLITTERSTACK_MASK when rendering a cell with a
    // transparent background. When paint() selects a glyph, it checks for these
    // bits. If they are set, any lower planes with CELL_BLITTERSTACK_MASK set
    // take this into account when solving the background color.
    unsigned blittedquads: 4;
    unsigned damaged: 1; // only used in rasterization
    // if CELL_ALPHA_HIGHCONTRAST is in play, we apply the HSV flip once the
    // background is locked in. set highcontrast to indicate this.
    unsigned highcontrast: 1;
    unsigned fgblends: 8;
    unsigned bgblends: 8;
    // we'll need recalculate the foreground relative to the solved background,
    // and then reapply any foreground shading from above the highcontrast
    // declaration. save the foreground state when we go highcontrast.
    unsigned hcfgblends: 8; // number of foreground blends prior to HIGHCONTRAST
    unsigned sprixeled: 1; // have we passed through a sprixel?
    unsigned p_beats_sprixel: 1; // did we solve for our glyph above the bitmap?
  } s;
};

typedef struct ncpile {
  ncplane* top;               // topmost plane, never NULL
  ncplane* bottom;            // bottommost plane, never NULL
  ncplane* roots;             // head of root plane list
  struct crender* crender;    // array (rows * cols crender objects)
  struct notcurses* nc;       // notcurses context
  struct ncpile *prev, *next; // circular list
  size_t crenderlen;          // size of crender vector
  int dimy, dimx;             // rows and cols at time of render
  sprixel* sprixelcache;      // list of sprixels
} ncpile;

// the standard pile can be reached through ->stdplane.
typedef struct notcurses {
  ncplane* stdplane; // standard plane, covers screen

  // the style state of the terminal is carried across render runs
  rasterstate rstate;

  // we keep a copy of the last rendered frame. this facilitates O(1)
  // notcurses_at_yx() and O(1) damage detection (at the cost of some memory).
  nccell* lastframe;// last rasterized framebuffer, NULL until first raster
  egcpool pool;   // egcpool for lastframe

  int lfdimx;     // dimensions of lastframe, unchanged by screen resize
  int lfdimy;     // lfdimx/lfdimy are 0 until first rasterization

  int cursory;    // desired cursor placement according to user.
  int cursorx;    // -1 is don't-care, otherwise moved here after each render.

  pthread_mutex_t statlock; // FIXME align on cacheline
  ncstats stats;  // some statistics across the lifetime of the notcurses ctx
  ncstats stashed_stats; // retain across a notcurses_stats_reset(), to print in closing banner

  FILE* ttyfp;    // FILE* for writing rasterized data
  int ttyfd;      // file descriptor for controlling tty
  ncinputlayer input; // input layer; we're in cbreak mode
  FILE* renderfp; // debugging FILE* to which renderings are written
  tinfo tcache;   // terminfo cache
  struct termios tpreserved; // terminal state upon entry
  pthread_mutex_t pilelock; // guards pile list, locks resize in render
  bool suppress_banner; // from notcurses_options

  // desired margins (best-effort only), copied in from notcurses_options
  int margin_t, margin_b, margin_r, margin_l;
  int loglevel;
  palette256 palette; // 256-indexed palette can be used instead of/with RGB
  bool palette_damage[NCPALETTESIZE];
  unsigned stdio_blocking_save; // was stdio blocking at entry? restore on stop.
} notcurses;

typedef struct blitterargs {
  // FIXME begy/begx are really only of interest to scaling; they ought be
  // consumed there, and blitters ought always work with the scaled output.
  int begy;            // upper left start within visual
  int begx;
  int leny;            // number of source pixels to use
  int lenx;
  uint32_t transcolor; // if non-zero, treat the lower 24 bits as a transparent color
  union { // cell vs pixel-specific arguments
    struct {
      int placey;      // placement within ncplane
      int placex;
      int blendcolors; // use CELL_ALPHA_BLEND
    } cell;            // for cells
    struct {
      int celldimx;    // horizontal pixels per cell
      int celldimy;    // vertical pixels per cell
      int colorregs;   // number of color registers
      sprixel* spx;    // sprixel object
      // in at least mlterm, emitting a sixel makes the cursor visible.
      // if the cursor is hidden, and sprixel_cursor_hack is set, this
      // is set to the civis capability.
      const char* cursor_hack;
    } pixel;           // for pixels
  } u;
} blitterargs;

// scaledy and scaledx are output geometry from scaling; data is output data
// from scaling. we might actually need more pixels due to framing concerns,
// in which case just assume transparent input pixels where needed.
typedef int (*ncblitter)(struct ncplane* n, int linesize, const void* data,
                         int scaledy, int scaledx, const blitterargs* bargs);

// a system for rendering RGBA pixels as text glyphs or sixel/kitty bitmaps
struct blitset {
  ncblitter_e geom;
  int width;        // number of input pixels per output cell, width
  int height;       // number of input pixels per output cell, height
  // the EGCs which form the various levels of a given plotset. if the geometry
  // is wide, things are arranged with the rightmost side increasing most
  // quickly, i.e. it can be indexed as height arrays of 1 + height glyphs. i.e.
  // the first five braille EGCs are all 0 on the left, [0..4] on the right.
  const wchar_t* egcs;
  ncblitter blit;
  const char* name;
  bool fill;
};

#include "blitset.h"

int ncvisual_blitset_geom(const notcurses* nc, const tinfo* tcache,
                          const struct ncvisual* n,
                          const struct ncvisual_options* vopts,
                          int* y, int* x, int* scaley, int* scalex,
                          int* leny, int* lenx, const struct blitset** blitter);

static inline int
ncfputs(const char* ext, FILE* out){
  int r;
#ifdef __USE_GNU
  r = fputs_unlocked(ext, out);
#else
  r = fputs(ext, out);
#endif
  return r;
}

static inline int
ncfputc(char c, FILE* out){
#ifdef __USE_GNU
  return fputc_unlocked(c, out);
#else
  return fputc(c, out);
#endif
}

void reset_stats(ncstats* stats);
void summarize_stats(notcurses* nc);

void update_raster_stats(const struct timespec* time1, const struct timespec* time0, ncstats* stats);
void update_render_stats(const struct timespec* time1, const struct timespec* time0, ncstats* stats);
void update_render_bytes(ncstats* stats, int bytes);
void update_write_stats(const struct timespec* time1, const struct timespec* time0, ncstats* stats, int bytes);

void sigwinch_handler(int signo);

void init_lang(notcurses* nc); // nc may be NULL, only used for logging
int terminfostr(char** gseq, const char* name);

// load |ti| from the terminfo database, which must already have been
// initialized. set |utf8| if we've verified UTF8 output encoding.
int interrogate_terminfo(tinfo* ti, int fd, const char* termname, unsigned utf8);

void free_terminfo_cache(tinfo* ti);

// perform queries that require writing to the terminal, and reading a
// response, rather than simply reading the terminfo database. can result
// in a lengthy delay or even block if the terminal doesn't respond.
int query_term(tinfo* ti, int fd);

// if there were missing elements we wanted from terminfo, bitch about them here
void warn_terminfo(const notcurses* nc, const tinfo* ti);

int resize_callbacks_children(ncplane* n);

static inline ncpile*
ncplane_pile(ncplane* n){
  return n->pile;
}

static inline const ncpile*
ncplane_pile_const(const ncplane* n){
  return n->pile;
}

static inline ncplane*
ncplane_stdplane(ncplane* n){
  return notcurses_stdplane(ncplane_notcurses(n));
}

static inline const ncplane*
ncplane_stdplane_const(const ncplane* n){
  return notcurses_stdplane_const(ncplane_notcurses_const(n));
}

// load all known special keys from terminfo, and build the input sequence trie
int prep_special_keys(ncinputlayer* nc);

// free up the input escapes trie
void input_free_esctrie(struct esctrie** trie);

// initialize libav
int ncvisual_init(int loglevel);

static inline int
fbcellidx(int row, int rowlen, int col){
//fprintf(stderr, "row: %d rowlen: %d col: %d\n", row, rowlen, col);
  return row * rowlen + col;
}

// take a logical 'y' and convert it to the virtual 'y'. see HACKING.md.
static inline int
logical_to_virtual(const ncplane* n, int y){
//fprintf(stderr, "y: %d n->logrow: %d n->leny: %d\n", y, n->logrow, n->leny);
  return (y + n->logrow) % n->leny;
}

static inline int
nfbcellidx(const ncplane* n, int row, int col){
  return fbcellidx(logical_to_virtual(n, row), n->lenx, col);
}

// For our first attempt, O(1) uniform conversion from 8-bit r/g/b down to
// ~2.4-bit 6x6x6 cube + greyscale (assumed on entry; I know no way to
// even semi-portably recover the palette) proceeds via: map each 8-bit to
// a 5-bit target grey. if all 3 components match, select that grey.
// otherwise, c / 42.7 to map to 6 values.
static inline int
rgb_quantize_256(unsigned r, unsigned g, unsigned b){
  const unsigned GREYMASK = 0xf8;
  // if all 5 MSBs match, return grey from 24-member grey ramp or pure
  // black/white from original 16 (0 and 15, respectively)
  if((r & GREYMASK) == (g & GREYMASK) && (g & GREYMASK) == (b & GREYMASK)){
    // 256 / 26 == 9.846
    int gidx = r * 5 / 49 - 1;
    if(gidx < 0){
      return 0;
    }
    if(gidx >= 24){
      return 15;
    }
    return 232 + gidx;
  }
  r /= 43;
  g /= 43;
  b /= 43;
  return r * 36 + g * 6 + b + 16;
}

// We get black, red, green, yellow, blue, magenta, cyan, and white. Ugh. Under
// an additive model: G + R -> Y, G + B -> C, R + B -> M, R + G + B -> W
static inline int
rgb_quantize_8(unsigned r, unsigned g, unsigned b){
  static const int BLACK = 0;
  static const int RED = 1;
  static const int GREEN = 2;
  static const int YELLOW = 3;
  static const int BLUE = 4;
  static const int MAGENTA = 5;
  static const int CYAN = 6;
  static const int WHITE = 7;
  if(r < 128){ // we have no red
    if(g < 128){ // we have no green
      if(b < 128){
        return BLACK;
      }
      return BLUE;
    } // we have green
    if(b < 128){
      return GREEN;
    }
    return CYAN;
  }else if(g < 128){ // we have red, but not green
    if(b < 128){
      return RED;
    }
    return MAGENTA;
  }else if(b < 128){ // we have red and green
    return YELLOW;
  }
  return WHITE;
}

// Given r, g, and b values 0..255, do a weighted average per Rec. 601, and
// return the 8-bit greyscale value (this value will be the r, g, and b value
// for the new color).
static inline int
rgb_greyscale(int r, int g, int b){
  if(r < 0 || r > 255){
    return -1;
  }
  if(g < 0 || g > 255){
    return -1;
  }
  if(b < 0 || b > 255){
    return -1;
  }
  // Use Rec. 601 scaling plus linear approximation of gamma decompression
  float fg = (0.299 * (r / 255.0) + 0.587 * (g / 255.0) + 0.114 * (b / 255.0));
  return fg * 255;
}

// write(2) with retry on partial write or interrupted write
static inline ssize_t
writen(int fd, const void* buf, size_t len){
  ssize_t r;
  size_t w = 0;
  while(w < len){
    if((r = write(fd, (const char*)buf + w, len - w)) < 0){
      if(errno == EAGAIN || errno == EBUSY || errno == EINTR){
        continue;
      }
      return -1;
    }
    w += r;
  }
  return w;
}

static inline int
tty_emit(const char* seq, int fd){
  if(!seq){
    return -1;
  }
  size_t slen = strlen(seq);
  if(writen(fd, seq, slen) < 0){
    return -1;
  }
  return 0;
}

static inline int
term_emit(const char* seq, FILE* out, bool flush){
  if(!seq){
    return -1;
  }
  if(ncfputs(seq, out) == EOF){
//fprintf(stderr, "Error emitting %zub escape (%s)\n", strlen(seq), strerror(errno));
    return -1;
  }
  if(flush){
    while(fflush(out) == EOF){
      if(errno != EAGAIN && errno != EINTR && errno != EBUSY){
        fprintf(stderr, "Error flushing after %zub sequence (%s)\n", strlen(seq), strerror(errno));
        return -1;
      }
    }
  }
  return 0;
}

static inline int
term_bg_palindex(const notcurses* nc, FILE* out, unsigned pal){
  if(nc->tcache.setab == NULL){
    return 0;
  }
  return term_emit(tiparm(nc->tcache.setab, pal), out, false);
}

static inline int
term_fg_palindex(const notcurses* nc, FILE* out, unsigned pal){
  if(nc->tcache.setaf == NULL){
    return 0;
  }
  return term_emit(tiparm(nc->tcache.setaf, pal), out, false);
}

static inline const char*
pool_extended_gcluster(const egcpool* pool, const nccell* c){
  if(cell_simple_p(c)){
    return (const char*)&c->gcluster;
  }
  return egcpool_extended_gcluster(pool, c);
}

static inline nccell*
ncplane_cell_ref_yx(ncplane* n, int y, int x){
  return &n->fb[nfbcellidx(n, y, x)];
}

static inline void
cell_debug(const egcpool* p, const nccell* c){
  fprintf(stderr, "gcluster: %08x %s style: 0x%04x chan: 0x%016jx\n",
				  c->gcluster, egcpool_extended_gcluster(p, c), c->stylemask, c->channels);
}

static inline void
plane_debug(const ncplane* n, bool details){
  int dimy, dimx;
  ncplane_dim_yx(n, &dimy, &dimx);
  fprintf(stderr, "p: %p dim: %d/%d poolsize: %d\n", n, dimy, dimx, n->pool.poolsize);
  if(details){
    for(int y = 0 ; y < 1 ; ++y){
      for(int x = 0 ; x < 10 ; ++x){
        const nccell* c = &n->fb[fbcellidx(y, dimx, x)];
        fprintf(stderr, "[%03d/%03d] ", y, x);
        cell_debug(&n->pool, c);
      }
    }
  }
}

// cell coordinates *within the sprixel*, not absolute
int sprite_wipe(const notcurses* nc, sprixel* s, int y, int x);
int sixel_wipe(sprixel* s, int ycell, int xcell);
// nulls out a cell from a kitty bitmap via changing the alpha value
// throughout to 0. the same trick doesn't work on sixel, but there we
// can just print directly over the bitmap.
int kitty_wipe(sprixel* s, int ycell, int xcell);
int sixel_rebuild(sprixel* s, int ycell, int xcell, uint8_t* auxvec);
int kitty_rebuild(sprixel* s, int ycell, int xcell, uint8_t* auxvec);

void sprixel_free(sprixel* s);
void sprixel_hide(sprixel* s);

int kitty_draw(const ncpile *p, sprixel* s, FILE* out);
int sixel_draw(const ncpile *p, sprixel* s, FILE* out);
// dimy and dimx are cell geometry, not pixel.
sprixel* sprixel_alloc(ncplane* n, int dimy, int dimx);
sprixel* sprixel_recycle(ncplane* n);
// takes ownership of s on success.
int sprixel_load(sprixel* spx, char* s, int bytes, int pixy, int pixx, int parse_start);
int sixel_destroy(const notcurses* nc, const ncpile* p, FILE* out, sprixel* s);
int kitty_destroy(const notcurses* nc, const ncpile* p, FILE* out, sprixel* s);
int kitty_remove(int id, FILE* out);
int kitty_clear_all(int fd);
int sixel_init(int fd);
int sprite_init(const tinfo* t, int fd);
int sprite_clear_all(const tinfo* t, int fd);
int kitty_shutdown(int fd);
int sixel_shutdown(int fd);
// these three all use absolute coordinates
void sprixel_invalidate(sprixel* s, int y, int x);
void sprixel_movefrom(sprixel* s, int y, int x);
void sprixel_debug(FILE* out, const sprixel* s);
void sixelmap_free(struct sixelmap *s);

// create an auxiliary vector suitable for a sprixcell, and zero it out. there
// are two bytes per pixel in the cell. kitty uses only one (for an alpha
// value). sixel uses both (for palette index, and transparency). FIXME fold
// the transparency vector up into 1/8th as many bytes.
uint8_t* sprixel_auxiliary_vector(const sprixel* s);

int sixel_blit(ncplane* nc, int linesize, const void* data,
               int leny, int lenx, const blitterargs* bargs);

int kitty_blit(ncplane* nc, int linesize, const void* data,
               int leny, int lenx, const blitterargs* bargs);

static inline int
sprite_destroy(const notcurses* nc, const ncpile* p, FILE* out, sprixel* s){
  return nc->tcache.pixel_destroy(nc, p, out, s);
}

// precondition: s->invalidated is SPRIXEL_INVALIDATED or SPRIXEL_MOVED.
static inline int
sprite_draw(const notcurses* n, const ncpile* p, sprixel* s, FILE* out){
//sprixel_debug(stderr, s);
  return n->tcache.pixel_draw(p, s, out);
}

static inline int
sprite_rebuild(const notcurses* nc, sprixel* s, int ycell, int xcell){
  const int idx = s->dimx * ycell + xcell;
  int ret = 0;
  // special case the transition back to SPRIXCELL_TRANSPARENT; this can be
  // done in O(1), since the actual glyph needn't change.
  if(s->n->tam[idx].state == SPRIXCELL_ANNIHILATED_TRANS){
    s->n->tam[idx].state = SPRIXCELL_TRANSPARENT;
  }else if(s->n->tam[idx].state == SPRIXCELL_ANNIHILATED){
    // sets the new state itself
    uint8_t* auxvec = s->n->tam[idx].auxvector;
    assert(auxvec);
    ret = nc->tcache.pixel_rebuild(s, ycell, xcell, auxvec);
    free(auxvec);
    s->n->tam[idx].auxvector = NULL;
  }
  // don't upset SPRIXEL_MOVED
  if(s->invalidated == SPRIXEL_QUIESCENT){
    if(s->n->tam[idx].state != SPRIXCELL_TRANSPARENT &&
       s->n->tam[idx].state != SPRIXCELL_ANNIHILATED &&
       s->n->tam[idx].state != SPRIXCELL_ANNIHILATED_TRANS){
      s->invalidated = SPRIXEL_INVALIDATED;
    }
  }
  return ret;
}

// |y| and |x| are scaled geometry on input, and clamped scaled geometry on
// output. |outy| is output geometry on output, and unused on input. output
// geometry is derived from scaled geometry and output requirements (that Sixel
// must be a multiple of six pixels tall). output width is always equal to
// scaled width. all are pixels.
// happy fact: common reported values for maximum sixel height are 256, 1024,
// and 4096...not a single goddamn one of which is divisible by six. augh.
static inline void
clamp_to_sixelmax(const tinfo* t, int* y, int* x, int* outy, ncscale_e scaling){
  if(t->sixel_maxy && *y > t->sixel_maxy){
    *y = t->sixel_maxy;
  }
  *outy = *y;
  if(*outy % t->sprixel_scale_height){
    *outy += t->sprixel_scale_height - (*outy % t->sprixel_scale_height);
    while(*outy > t->sixel_maxy){
      *outy -= t->sprixel_scale_height;
    }
    if(scaling == NCSCALE_STRETCH || *y > *outy){
      *y = *outy;
    }
  }
  if(t->sixel_maxx && *x > t->sixel_maxx){
    *x = t->sixel_maxx;
  }
}

// any sprixcell which does not cover the entirety of the underlying cell
// cannot be SPRIXCELL_OPAQUE. this postprocesses the TAM, flipping any
// such sprixcells to SPRIXCELL_MIXED. |leny| and |lenx| are output geometry
// in pixels. |cdimy| and |cdimx| are output coverage in cells.
static inline void
scrub_tam_boundaries(tament* tam, int leny, int lenx, int cdimy, int cdimx){
  // any sprixcells which don't cover the full cell underneath them cannot
  // be SPRIXCELL_OPAQUE
  const int cols = (lenx + cdimx - 1) / cdimx;
  if(lenx % cdimx){
    for(int y = 0 ; y < (leny + cdimy - 1) / cdimy ; ++y){
      if(tam[y * cols + cols - 1].state == SPRIXCELL_OPAQUE_KITTY){
        tam[y * cols + cols - 1].state = SPRIXCELL_MIXED_KITTY;
      }else if(tam[y * cols + cols - 1].state == SPRIXCELL_OPAQUE_SIXEL){
        tam[y * cols + cols - 1].state = SPRIXCELL_MIXED_SIXEL;
      }
    }
  }
  if(leny % cdimy){
    const int y = (leny + cdimy - 1) / cdimy - 1;
    for(int x = 0 ; x < cols ; ++x){
      if(tam[y * cols + x].state == SPRIXCELL_OPAQUE_KITTY){
        tam[y * cols + x].state = SPRIXCELL_MIXED_KITTY;
      }else if(tam[y * cols + x].state == SPRIXCELL_OPAQUE_SIXEL){
        tam[y * cols + x].state = SPRIXCELL_MIXED_SIXEL;
      }
    }
  }
}

// get the TAM entry for these (absolute) coordinates
static inline sprixcell_e
sprixel_state(const sprixel* s, int y, int x){
  const ncplane* stdn = notcurses_stdplane_const(ncplane_notcurses_const(s->n));
  int localy = y - (s->n->absy - stdn->absy);
  int localx = x - (s->n->absx - stdn->absx);
//fprintf(stderr, "TAM %d at %d/%d (%d/%d)\n", s->n->tam[localy * s->dimx + localx].state, localy, localx, y, x);
  assert(localy >= 0);
  assert(localy < s->dimy);
  assert(localx >= 0);
  assert(localx < s->dimx);
  return s->n->tam[localy * s->dimx + localx].state;
}

static inline void
pool_release(egcpool* pool, nccell* c){
  if(cell_extended_p(c)){
    egcpool_release(pool, cell_egc_idx(c));
  }
  c->gcluster = 0; // don't subject ourselves to double-release problems
  c->width = 0;    // don't subject ourselves to geometric ambiguities
}

// set the nccell 'c' to point into the egcpool at location 'eoffset'
static inline void
set_gcluster_egc(nccell* c, int eoffset){
  c->gcluster = htole(0x01000000ul) + htole(eoffset);
}

// Duplicate one nccell onto another, possibly crossing ncplanes.
static inline int
cell_duplicate_far(egcpool* tpool, nccell* targ, const ncplane* splane, const nccell* c){
  pool_release(tpool, targ);
  targ->stylemask = c->stylemask;
  targ->channels = c->channels;
  targ->width = c->width;
  if(!cell_extended_p(c)){
    targ->gcluster = c->gcluster;
    return 0;
  }
  const char* egc = nccell_extended_gcluster(splane, c);
  size_t ulen = strlen(egc);
  int eoffset = egcpool_stash(tpool, egc, ulen);
  if(eoffset < 0){
    return -1;
  }
  set_gcluster_egc(targ, eoffset);
  return 0;
}

int ncplane_resize_internal(ncplane* n, int keepy, int keepx,
                            int keepleny, int keeplenx, int yoff, int xoff,
                            int ylen, int xlen);

int update_term_dimensions(int fd, int* rows, int* cols, tinfo* tcache);

ALLOC static inline void*
memdup(const void* src, size_t len){
  void* ret = malloc(len);
  if(ret){
    memcpy(ret, src, len);
  }
  return ret;
}

ALLOC void* bgra_to_rgba(const void* data, int rows, int* rowstride, int cols, int alpha);
ALLOC void* rgb_loose_to_rgba(const void* data, int rows, int* rowstride, int cols, int alpha);
ALLOC void* rgb_packed_to_rgba(const void* data, int rows, int* rowstride, int cols, int alpha);

// find the "center" cell of two lengths. in the case of even rows/columns, we
// place the center on the top/left. in such a case there will be one more
// cell to the bottom/right of the center.
static inline void
center_box(int* RESTRICT y, int* RESTRICT x){
  if(y){
    *y = (*y - 1) / 2;
  }
  if(x){
    *x = (*x - 1) / 2;
  }
}

// find the "center" cell of a plane. in the case of even rows/columns, we
// place the center on the top/left. in such a case there will be one more
// cell to the bottom/right of the center.
static inline void
ncplane_center(const ncplane* n, int* RESTRICT y, int* RESTRICT x){
  *y = n->leny;
  *x = n->lenx;
  center_box(y, x);
}

int ncvisual_bounding_box(const struct ncvisual* ncv, int* leny, int* lenx,
                          int* offy, int* offx);

// Our gradient is a 2d lerp among the four corners of the region. We start
// with the observation that each corner ought be its exact specified corner,
// and the middle ought be the exact average of all four corners' components.
// Another observation is that if all four corners are the same, every cell
// ought be the exact same color. From this arises the observation that a
// perimeter element is not affected by the other three sides:
//
//  a corner element is defined by itself
//  a perimeter element is defined by the two points on its side
//  an internal element is defined by all four points
//
// 2D equation of state: solve for each quadrant's contribution (min 2x2):
//
//  X' = (xlen - 1) - X
//  Y' = (ylen - 1) - Y
//  TLC: X' * Y' * TL
//  TRC: X * Y' * TR
//  BLC: X' * Y * BL
//  BRC: X * Y * BR
//  steps: (xlen - 1) * (ylen - 1) [maximum steps away from origin]
//
// Then add TLC + TRC + BLC + BRC + steps / 2, and divide by steps (the
//  steps / 2 is to work around truncate-towards-zero).
static int
calc_gradient_component(unsigned tl, unsigned tr, unsigned bl, unsigned br,
                        int y, int x, int ylen, int xlen){
  const int avm = (ylen - 1) - y;
  const int ahm = (xlen - 1) - x;
  if(xlen < 2){
    if(ylen < 2){
      return tl;
    }
    return (tl * avm + bl * y) / (ylen - 1);
  }
  if(ylen < 2){
    return (tl * ahm + tr * x) / (xlen - 1);
  }
  const int tlc = ahm * avm * tl;
  const int blc = ahm * y * bl;
  const int trc = x * avm * tr;
  const int brc = y * x * br;
  const int divisor = (ylen - 1) * (xlen - 1);
  return ((tlc + blc + trc + brc) + divisor / 2) / divisor;
}

// calculate one of the channels of a gradient at a particular point.
static inline uint32_t
calc_gradient_channel(uint32_t ul, uint32_t ur, uint32_t ll, uint32_t lr,
                      int y, int x, int ylen, int xlen){
  uint32_t chan = 0;
  ncchannel_set_rgb8_clipped(&chan,
                           calc_gradient_component(ncchannel_r(ul), ncchannel_r(ur),
                                                   ncchannel_r(ll), ncchannel_r(lr),
                                                   y, x, ylen, xlen),
                           calc_gradient_component(ncchannel_g(ul), ncchannel_g(ur),
                                                   ncchannel_g(ll), ncchannel_g(lr),
                                                   y, x, ylen, xlen),
                           calc_gradient_component(ncchannel_b(ul), ncchannel_b(ur),
                                                   ncchannel_b(ll), ncchannel_b(lr),
                                                   y, x, ylen, xlen));
  ncchannel_set_alpha(&chan, ncchannel_alpha(ul)); // precondition: all αs are equal
  return chan;
}

// calculate both channels of a gradient at a particular point, storing them
// into `channels'. x and y ought be the location within the gradient.
static inline void
calc_gradient_channels(uint64_t* channels, uint64_t ul, uint64_t ur,
                       uint64_t ll, uint64_t lr, int y, int x,
                       int ylen, int xlen){
  if(!ncchannels_fg_default_p(ul)){
    ncchannels_set_fchannel(channels,
                            calc_gradient_channel(ncchannels_fchannel(ul),
                                                  ncchannels_fchannel(ur),
                                                  ncchannels_fchannel(ll),
                                                  ncchannels_fchannel(lr),
                                                  y, x, ylen, xlen));
  }else{
    ncchannels_set_fg_default(channels);
  }
  if(!ncchannels_bg_default_p(ul)){
    ncchannels_set_bchannel(channels,
                            calc_gradient_channel(ncchannels_bchannel(ul),
                                                  ncchannels_bchannel(ur),
                                                  ncchannels_bchannel(ll),
                                                  ncchannels_bchannel(lr),
                                                  y, x, ylen, xlen));
  }else{
    ncchannels_set_bg_default(channels);
  }
}

// ncdirect needs to "fake" an isolated ncplane as a drawing surface for
// ncvisual_render(), and thus calls these low-level internal functions.
// they are not for general use -- check ncplane_new() and ncplane_destroy().
ncplane* ncplane_new_internal(notcurses* nc, ncplane* n, const ncplane_options* nopts);

void free_plane(ncplane* p);

// heap-allocated formatted output
ALLOC char* ncplane_vprintf_prep(const char* format, va_list ap);

// Resize the provided ncvisual to the specified 'rows' x 'cols', but do not
// change the internals of the ncvisual. Uses oframe.
int ncvisual_blit(struct ncvisual* ncv, int rows, int cols,
                  ncplane* n, const struct blitset* bset,
                  const blitterargs* bargs);

void nclog(const char* fmt, ...);

bool is_linux_console(const notcurses* nc, unsigned no_font_changes);

// logging
#define logerror(nc, fmt, ...) do{ \
  if(nc){ if((nc)->loglevel >= NCLOGLEVEL_ERROR){ \
    nclog("%s:%d:" fmt, __func__, __LINE__, ##__VA_ARGS__); } \
  }else{ fprintf(stderr, "%s:%d:" fmt, __func__, __LINE__, ##__VA_ARGS__); } \
}while(0);

#define logwarn(nc, fmt, ...) do{ \
  if(nc){ if((nc)->loglevel >= NCLOGLEVEL_WARNING){ \
    nclog("%s:%d:" fmt, __func__, __LINE__, ##__VA_ARGS__); } \
  }else{ fprintf(stderr, "%s:%d:" fmt, __func__, __LINE__, ##__VA_ARGS__); } \
}while(0);

#define loginfo(nc, fmt, ...) do{ \
  if(nc){ if((nc)->loglevel >= NCLOGLEVEL_INFO){ \
    nclog("%s:%d:" fmt, __func__, __LINE__, ##__VA_ARGS__); } \
  } }while(0);

#define logverbose(nc, fmt, ...) do{ \
  if(nc){ if((nc)->loglevel >= NCLOGLEVEL_VERBOSE){ \
    nclog("%s:%d:" fmt, __func__, __LINE__, ##__VA_ARGS__); } \
  } }while(0);

#define logdebug(nc, fmt, ...) do{ \
  if(nc){ if((nc)->loglevel >= NCLOGLEVEL_DEBUG){ \
    nclog("%s:%d:" fmt, __func__, __LINE__, ##__VA_ARGS__); } \
  } }while(0);

#define logtrace(nc, fmt, ...) do{ \
  if(nc){ if((nc)->loglevel >= NCLOGLEVEL_TRACE){ \
    nclog("%s:%d:" fmt, __func__, __LINE__, ##__VA_ARGS__); } \
  } }while(0);

int term_setstyle(FILE* out, unsigned cur, unsigned targ, unsigned stylebit,
                  const char* ton, const char* toff);

// how many edges need touch a corner for it to be printed?
static inline unsigned
box_corner_needs(unsigned ctlword){
  return (ctlword & NCBOXCORNER_MASK) >> NCBOXCORNER_SHIFT;
}

// True if the cell does not generate background pixels (i.e., the cell is a
// solid or shaded block, or certain emoji).
static inline bool
cell_nobackground_p(const nccell* c){
  return (c->channels & CELL_NOBACKGROUND_MASK) == CELL_NOBACKGROUND_MASK;
}

// Returns a number 0 <= n <= 15 representing the four quadrants, and which (if
// any) are occupied due to blitting with a transparent background. The mapping
// is {tl, tr, bl, br}.
static inline unsigned
cell_blittedquadrants(const nccell* c){
  return ((c->channels & 0x8000000000000000ull) ? 1 : 0) |
         ((c->channels & 0x0400000000000000ull) ? 2 : 0) |
         ((c->channels & 0x0200000000000000ull) ? 4 : 0) |
         ((c->channels & 0x0100000000000000ull) ? 8 : 0);
}

// Set this whenever blitting an ncvisual, when we have a transparent
// background. In such cases, ncvisuals underneath the cell must be rendered
// slightly differently.
static inline void
cell_set_blitquadrants(nccell* c, unsigned tl, unsigned tr, unsigned bl, unsigned br){
  // FIXME want a static assert that these four constants OR together to
  // equal CELL_BLITTERSTACK_MASK, bah
  uint64_t newval = (tl ? 0x8000000000000000ull : 0) |
                    (tr ? 0x0400000000000000ull : 0) |
                    (bl ? 0x0200000000000000ull : 0) |
                    (br ? 0x0100000000000000ull : 0);
  c->channels = ((c->channels & ~CELL_BLITTERSTACK_MASK) | newval);
}

// Destroy a plane and all its bound descendants.
int ncplane_genocide(ncplane *ncp);

// Extract the 32-bit background channel from a cell.
static inline uint32_t
cell_bchannel(const nccell* cl){
  return ncchannels_bchannel(cl->channels);
}

// Extract those elements of the channel which are common to both foreground
// and background channel representations.
static inline uint32_t
channel_common(uint32_t channel){
  return channel & (CELL_BGDEFAULT_MASK | CELL_BG_RGB_MASK |
                    CELL_BG_PALETTE | CELL_BG_ALPHA_MASK);
}

// Extract those elements of the background channel which may be freely swapped
// with the foreground channel (alpha and coloring info).
static inline uint32_t
cell_bchannel_common(const nccell* cl){
  return channel_common(cell_bchannel(cl));
}

// Extract the 32-bit foreground channel from a cell.
static inline uint32_t
cell_fchannel(const nccell* cl){
  return ncchannels_fchannel(cl->channels);
}

// Extract those elements of the foreground channel which may be freely swapped
// with the background channel (alpha and coloring info).
static inline uint32_t
cell_fchannel_common(const nccell* cl){
  return channel_common(cell_fchannel(cl));
}

// Set the 32-bit background channel of an nccell.
static inline uint64_t
cell_set_bchannel(nccell* cl, uint32_t channel){
  return ncchannels_set_bchannel(&cl->channels, channel);
}

// Set the 32-bit foreground channel of an nccell.
static inline uint64_t
cell_set_fchannel(nccell* cl, uint32_t channel){
  return ncchannels_set_fchannel(&cl->channels, channel);
}

// Returns the result of blending two channels. 'blends' indicates how heavily
// 'c1' ought be weighed. If 'blends' is 0, 'c1' will be entirely replaced by
// 'c2'. If 'c1' is otherwise the default color, 'c1' will not be touched,
// since we can't blend default colors. Likewise, if 'c2' is a default color,
// it will not be used (unless 'blends' is 0).
//
// Palette-indexed colors do not blend. Do not pass me palette-indexed channels!
static inline unsigned
channels_blend(unsigned c1, unsigned c2, unsigned* blends){
  if(ncchannel_alpha(c2) == CELL_ALPHA_TRANSPARENT){
    return c1; // do *not* increment *blends
  }
  bool c2default = ncchannel_default_p(c2);
  if(*blends == 0){
    // don't just return c2, or you set wide status and all kinds of crap
    if(ncchannel_default_p(c2)){
      ncchannel_set_default(&c1);
    }else{
      ncchannel_set(&c1, c2 & CELL_BG_RGB_MASK);
    }
    ncchannel_set_alpha(&c1, ncchannel_alpha(c2));
  }else if(!c2default && !ncchannel_default_p(c1)){
    unsigned rsum, gsum, bsum;
    ncchannel_rgb8(c2, &rsum, &gsum, &bsum);
    rsum = (ncchannel_r(c1) * *blends + rsum) / (*blends + 1);
    gsum = (ncchannel_g(c1) * *blends + gsum) / (*blends + 1);
    bsum = (ncchannel_b(c1) * *blends + bsum) / (*blends + 1);
    ncchannel_set_rgb8(&c1, rsum, gsum, bsum);
    ncchannel_set_alpha(&c1, ncchannel_alpha(c2));
  }
  ++*blends;
  return c1;
}

// do not pass palette-indexed channels!
static inline uint64_t
cell_blend_fchannel(nccell* cl, unsigned channel, unsigned* blends){
  return cell_set_fchannel(cl, channels_blend(cell_fchannel(cl), channel, blends));
}

static inline uint64_t
cell_blend_bchannel(nccell* cl, unsigned channel, unsigned* blends){
  return cell_set_bchannel(cl, channels_blend(cell_bchannel(cl), channel, blends));
}

// examine the UTF-8 EGC in the first |*bytes| bytes of |egc|. if the EGC is
// right-to-left, we make a copy, appending an U+200E to force left-to-right.
// only the first unicode char of the EGC is currently checked FIXME. if the
// EGC is not RTL, we return NULL.
ALLOC static inline char*
egc_rtl(const char* egc, int* bytes){
  wchar_t w;
  mbstate_t mbstate = { };
  size_t r = mbrtowc(&w, egc, *bytes, &mbstate);
  if(r == (size_t)-1 || r == (size_t)-2){
    return NULL;
  }
  const int bidic = uc_bidi_category(w);
//fprintf(stderr, "BIDI CAT (%lc): %d\n", w, bidic);
  if(bidic != UC_BIDI_R && bidic != UC_BIDI_AL && bidic != UC_BIDI_RLE && bidic != UC_BIDI_RLO){
    return NULL;
  }
  // insert U+200E, "LEFT-TO-RIGHT MARK". This ought reset the text direction
  // after emitting a potentially RTL EGC.
  const char LTRMARK[] = "\xe2\x80\x8e";
  char* s = (char*)malloc(*bytes + sizeof(LTRMARK)); // cast for C++ callers
  memcpy(s, egc, *bytes);
  memcpy(s + *bytes, LTRMARK, sizeof(LTRMARK));
  *bytes += strlen(LTRMARK);
  return s;
}

// a sprixel occupies the entirety of its associated plane, usually an entirely
// new, purpose-specific plane. |leny| and |lenx| are output geometry in pixels.
static inline int
plane_blit_sixel(sprixel* spx, char* s, int bytes, int leny, int lenx,
                 int parse_start, tament* tam){
  if(sprixel_load(spx, s, bytes, leny, lenx, parse_start)){
    return -1;
  }
  ncplane* n = spx->n;
  if(n){
//fprintf(stderr, "TAM WAS: %p NOW: %p\n", n->tam, tam);
    if(n->tam != tam){
      free(n->tam);
    }
    n->tam = tam;
    n->sprite = spx;
  }
  return 0;
}

// lowest level of cell+pool setup. if the EGC changes the output to RTL, it
// must be suffixed with a LTR-forcing character by now. The four bits of
// CELL_BLITTERSTACK_MASK ought already be initialized. If gcluster is four
// bytes or fewer, this function cannot fail.
static inline int
pool_blit_direct(egcpool* pool, nccell* c, const char* gcluster, int bytes, int cols){
  pool_release(pool, c);
  if(bytes < 0 || cols < 0){
    return -1;
  }
  c->width = cols;
  if(bytes <= 4){
    c->gcluster = 0;
    memcpy(&c->gcluster, gcluster, bytes);
  }else{
    int eoffset = egcpool_stash(pool, gcluster, bytes);
    if(eoffset < 0){
      return -1;
    }
    set_gcluster_egc(c, eoffset);
  }
  return bytes;
}

// Do an RTL-check, reset the quadrant occupancy bits, and pass the cell down to
// pool_blit_direct().
static inline int
pool_load_direct(egcpool* pool, nccell* c, const char* gcluster, int bytes, int cols){
  char* rtl = NULL;
  c->channels &= ~CELL_NOBACKGROUND_MASK;
  if(bytes >= 0){
    rtl = egc_rtl(gcluster, &bytes); // checks for RTL and adds U+200E if so
  }
  if(rtl){
    gcluster = rtl;
  }
  int r = pool_blit_direct(pool, c, gcluster, bytes, cols);
  free(rtl);
  return r;
}

static inline int
cell_load_direct(ncplane* n, nccell* c, const char* gcluster, int bytes, int cols){
  return pool_load_direct(&n->pool, c, gcluster, bytes, cols);
}

// increment y by 1 and rotate the framebuffer up one line. x moves to 0.
void scroll_down(ncplane* n);

static inline bool
islinebreak(wchar_t wchar){
  // UC_LINE_SEPARATOR + UC_PARAGRAPH_SEPARATOR
  if(wchar == L'\n' || wchar == L'\v' || wchar == L'\f'){
    return true;
  }
  const uint32_t mask = UC_CATEGORY_MASK_Zl | UC_CATEGORY_MASK_Zp;
  return uc_is_general_category_withtable(wchar, mask);
}

static inline bool
iswordbreak(wchar_t wchar){
  const uint32_t mask = UC_CATEGORY_MASK_Z |
                        UC_CATEGORY_MASK_Zs;
  return uc_is_general_category_withtable(wchar, mask);
}

// the heart of damage detection. compare two nccells (from two different
// planes) for equality. if they are equal, return 0. otherwise, dup the second
// onto the first and return non-zero.
static inline int
cellcmp_and_dupfar(egcpool* dampool, nccell* damcell,
                   const ncplane* srcplane, const nccell* srccell){
  if(damcell->stylemask == srccell->stylemask){
    if(damcell->channels == srccell->channels){
      const char* srcegc = nccell_extended_gcluster(srcplane, srccell);
      const char* damegc = pool_extended_gcluster(dampool, damcell);
      if(strcmp(damegc, srcegc) == 0){
        return 0; // EGC match
      }
    }
  }
  cell_duplicate_far(dampool, damcell, srcplane, srccell);
  return 1;
}

int ncinputlayer_init(ncinputlayer* nilayer, FILE* infp);

// FIXME absorb into ncinputlayer_init()
int cbreak_mode(int ttyfd, const struct termios* tpreserved);

int set_fd_nonblocking(int fd, unsigned state, unsigned* oldstate);

int get_tty_fd(notcurses* nc, FILE* ttyfp);

// Given the four channels arguments, verify that:
//
// - if any is default foreground, all are default foreground
// - if any is default background, all are default background
// - all foregrounds must have the same alpha
// - all backgrounds must have the same alpha
// - palette-indexed color must not be used
//
// If you only want to check n < 4 channels, just duplicate one.
bool check_gradient_args(uint64_t ul, uint64_t ur, uint64_t bl, uint64_t br);

int setup_signals(void* nc, bool no_quit_sigs, bool no_winch_sig,
                  int(*handler)(void*));
int drop_signals(void* nc);

// block a few signals for the duration of a write to the terminal.
int block_signals(sigset_t* old_blocked_signals);
int unblock_signals(const sigset_t* old_blocked_signals);

void ncvisual_printbanner(const notcurses* nc);

// alpha comes to us 0--255, but we have only 3 alpha values to map them to
// (opaque, blended, and transparent). it's necessary that we display
// something for any non-zero alpha (see #1540), so the threshold is 1.
// we might want to map this to blended, but we only use opaque and
// transparent for now. if |transcolor| is non-zero, match its lower 24
// bits against each pixel's RGB value, and treat a match as transparent.
static inline bool
rgba_trans_p(uint32_t p, uint32_t transcolor){
  if(ncpixel_a(p) < 192){
    return true;
  }
  if(transcolor && 
      (ncpixel_r(p) == (transcolor & 0xff0000ull)) &&
      (ncpixel_g(p) == (transcolor & 0xff00ull)) &&
      (ncpixel_b(p) == (transcolor & 0xffull))){
    return true;
  }
  return false;
}

// get a non-negative "manhattan distance" between two rgb values
static inline uint32_t
rgb_diff(unsigned r1, unsigned g1, unsigned b1, unsigned r2, unsigned g2, unsigned b2){
  uint32_t distance = 0;
  distance += r1 > r2 ? r1 - r2 : r2 - r1;
  distance += g1 > g2 ? g1 - g2 : g2 - g1;
  distance += b1 > b2 ? b1 - b2 : b2 - b1;
//fprintf(stderr, "RGBDIFF %u %u %u %u %u %u: %u\n", r1, g1, b1, r2, g2, b2, distance);
  return distance;
}

static inline uint64_t
ncdirect_channels(const ncdirect* nc){
  return nc->channels;
}

static inline bool
ncdirect_fg_default_p(const struct ncdirect* nc){
  return ncchannels_fg_default_p(ncdirect_channels(nc));
}

static inline bool
ncdirect_bg_default_p(const struct ncdirect* nc){
  return ncchannels_bg_default_p(ncdirect_channels(nc));
}

int term_fg_rgb8(bool RGBflag, const char* setaf, int colors, FILE* out,
                 unsigned r, unsigned g, unsigned b);

const struct blitset* lookup_blitset(const tinfo* tcache, ncblitter_e setid, bool may_degrade);

static inline int
rgba_blit_dispatch(ncplane* nc, const struct blitset* bset,
                   int linesize, const void* data,
                   int leny, int lenx, const blitterargs* bargs){
  return bset->blit(nc, linesize, data, leny, lenx, bargs);
}

static inline const struct blitset*
rgba_blitter_low(const tinfo* tcache, ncscale_e scale, bool maydegrade,
                 ncblitter_e blitrec) {
  if(blitrec == NCBLIT_DEFAULT){
    blitrec = rgba_blitter_default(tcache, scale);
  }
  return lookup_blitset(tcache, blitrec, maydegrade);
}

// RGBA visuals all use NCBLIT_2x1 by default (or NCBLIT_1x1 if not in
// UTF-8 mode), but an alternative can be specified.
static inline const struct blitset*
rgba_blitter(const struct tinfo* tcache, const struct ncvisual_options* opts) {
  const bool maydegrade = !(opts && (opts->flags & NCVISUAL_OPTION_NODEGRADE));
  const ncscale_e scale = opts ? opts->scaling : NCSCALE_NONE;
  return rgba_blitter_low(tcache, scale, maydegrade, opts ? opts->blitter : NCBLIT_DEFAULT);
}

typedef struct ncvisual_implementation {
  int (*visual_init)(int loglevel);
  void (*visual_printbanner)(const struct notcurses* nc);
  int (*visual_blit)(struct ncvisual* ncv, int rows, int cols, ncplane* n,
                     const struct blitset* bset, const blitterargs* barg);
  struct ncvisual* (*visual_create)(void);
  struct ncvisual* (*visual_from_file)(const char* fname);
  // ncv constructors other than ncvisual_from_file() need to set up the
  // AVFrame* 'frame' according to their own data, which is assumed to
  // have been prepared already in 'ncv'.
  void (*visual_details_seed)(struct ncvisual* ncv);
  int (*visual_decode)(struct ncvisual* nc);
  int (*visual_decode_loop)(struct ncvisual* nc);
  int (*visual_stream)(notcurses* nc, struct ncvisual* ncv, float timescale,
                       ncstreamcb streamer, const struct ncvisual_options* vopts, void* curry);
  char* (*visual_subtitle)(const struct ncvisual* ncv);
  int (*visual_resize)(struct ncvisual* ncv, int rows, int cols);
  void (*visual_destroy)(struct ncvisual* ncv);
  bool canopen_images;
  bool canopen_videos;
} ncvisual_implementation;

// assigned by libnotcurses.so if linked with multimedia
API extern const ncvisual_implementation* visual_implementation;

#undef ALLOC
#undef API

#ifdef __cplusplus
}
#endif

#endif
