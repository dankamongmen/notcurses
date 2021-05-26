#ifndef NOTCURSES_TERM
#define NOTCURSES_TERM

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

struct ncpile;
struct sprixel;
struct notcurses;

// terminfo cache. FIXME shrink this and kill a pointer deref by writing them
// all into one buffer, and storing 1-biased indices with 0 for NULL.
typedef struct tinfo {
  unsigned colors;// number of colors terminfo reported usable for this screen
  char* sgr;      // set many graphics properties at once
  char* sgr0;     // restore default presentation properties
  char* setaf;    // set foreground color (ANSI)
  char* setab;    // set background color (ANSI)
  char* op;       // set foreground and background color to default
  char* fgop;     // set foreground to default
  char* bgop;     // set background to default
  char* cup;      // move cursor
  char* cuu;      // move N cells up
  char* cub;      // move N cells left
  char* cuf;      // move N cells right
  char* cud;      // move N cells down
  char* cuf1;     // move 1 cell right
  char* home;     // home cursor
  char* civis;    // hide cursor
  char* cnorm;    // restore cursor to default state
  char* hpa;      // horizontal position adjusment (move cursor on row)
  char* vpa;      // vertical position adjustment (move cursor on column)
  char* standout; // NCSTYLE_STANDOUT
  char* uline;    // NCSTYLE_UNDERLINK
  char* reverse;  // NCSTYLE_REVERSE
  char* blink;    // NCSTYLE_BLINK
  char* dim;      // NCSTYLE_DIM
  char* bold;     // NCSTYLE_BOLD
  char* italics;  // NCSTYLE_ITALIC
  char* italoff;  // NCSTYLE_ITALIC (disable)
  char* struck;   // NCSTYLE_STRUCK
  char* struckoff;// NCSTYLE_STRUCK (disable)
  char* initc;    // set a palette entry's RGB value
  char* oc;       // restore original colors
  char* clearscr; // erase screen and home cursor
  char* sc;       // push the cursor location onto the stack
  char* rc;       // pop the cursor location off the stack
  char* smkx;     // enter keypad transmit mode (keypad_xmit)
  char* rmkx;     // leave keypad transmit mode (keypad_local)
  char* getm;     // get mouse events
  char* smcup;    // enter alternate mode
  char* rmcup;    // restore primary mode
  // we use the cell's size in pixels for pixel blitting. this information can
  // be acquired on all terminals with pixel support.
  int cellpixy;   // cell pixel height, might be 0
  int cellpixx;   // cell pixel width, might be 0

  // kitty interprets an RGB background that matches the default background
  // color *as* the default background, meaning it'll be translucent if
  // background_opaque is in use. detect this, and avoid the default if so.
  // bg_collides_default is either 0x0000000 or 0x1RRGGBB.
  uint32_t bg_collides_default;

  // sprixel support. there are several different sprixel protocols, of
  // which we support sixel and kitty. the kitty protocol is used based
  // on TERM heuristics. otherwise, we attempt to detect sixel support, and
  // query the details of the implementation.
  pthread_mutex_t pixel_query; // only query for pixel support once
  int color_registers; // sixel color registers (post pixel_query_done)
  int sixel_maxx, sixel_maxy; // sixel size maxima (post pixel_query_done)
  int (*pixel_destroy)(const struct notcurses* nc, const struct ncpile* p, FILE* out, struct sprixel* s);
  // wipe out a cell's worth of pixels from within a sprixel. for sixel, this
  // means leaving out the pixels (and likely resizes the string). for kitty,
  // this means dialing down their alpha to 0 (in equivalent space).
  int (*pixel_wipe)(struct sprixel* s, int y, int x);
  // perform the inverse of pixel_wipe, restoring an annihilated sprixcell.
  int (*pixel_rebuild)(struct sprixel* s, int y, int x, uint8_t* auxvec);
  int (*pixel_remove)(int id, FILE* out); // kitty only, issue actual delete command
  int (*pixel_init)(int fd);      // called when support is detected
  int (*pixel_draw)(const struct ncpile* p, struct sprixel* s, FILE* out);
  int (*pixel_shutdown)(int fd);  // called during context shutdown
  int (*pixel_clear_all)(int fd); // called during startup, kitty only
  int sprixel_scale_height; // sprixel must be a multiple of this many rows
  bool bitmap_supported;    // do we support bitmaps (post pixel_query_done)?
  bool sprixel_cursor_hack; // do sprixels reset the cursor? (mlterm)
  bool pixel_query_done;    // have we yet performed pixel query?
  // alacritty went rather off the reservation for their sixel support. they
  // reply to DSA with CSI?6c, meaning VT102, but no VT102 had Sixel support,
  // so if the TERM variable contains "alacritty", *and* we get VT102, we go
  // ahead and query XTSMGRAPHICS.
  bool alacritty_sixel_hack;

  bool RGBflag;   // "RGB" flag for 24bpc truecolor
  bool CCCflag;   // "CCC" flag for palette set capability
  bool BCEflag;   // "BCE" flag for erases with background color
  bool AMflag;    // "AM" flag for automatic movement to next line

  // assigned based off nl_langinfo() in notcurses_core_init()
  bool utf8;      // are we using utf-8 encoding, as hoped?

  // these are assigned wholly through TERM-based heuristics
  bool quadrants; // do we have (good, vetted) Unicode 1 quadrant support?
  bool sextants;  // do we have (good, vetted) Unicode 13 sextant support?
  bool braille;   // do we have Braille support? (linux console does not)
} tinfo;

#ifdef __cplusplus
}
#endif

#endif
