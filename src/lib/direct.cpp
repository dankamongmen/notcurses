#include <ncurses.h> // needed for some definitions, see terminfo(3ncurses)
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <termios.h>
#include "version.h"
#include "visual-details.h"
#include "notcurses/direct.h"
#include "internal.h"

int ncdirect_putstr(ncdirect* nc, uint64_t channels, const char* utf8){
  if(channels_fg_default_p(channels)){
    if(ncdirect_fg_default(nc)){
      return -1;
    }
  }else if(ncdirect_fg(nc, channels_fg(channels))){
    return -1;
  }
  if(channels_bg_default_p(channels)){
    if(ncdirect_bg_default(nc)){
      return -1;
    }
  }else if(ncdirect_bg(nc, channels_bg(channels))){
    return -1;
  }
  return fprintf(nc->ttyfp, "%s", utf8);
}

int ncdirect_cursor_up(ncdirect* nc, int num){
  if(num < 0){
    return -1;
  }
  if(!nc->tcache.cuu){
    return -1;
  }
  return term_emit("cuu", tiparm(nc->tcache.cuu, num), nc->ttyfp, false);
}

int ncdirect_cursor_left(ncdirect* nc, int num){
  if(num < 0){
    return -1;
  }
  if(!nc->tcache.cub){
    return -1;
  }
  return term_emit("cub", tiparm(nc->tcache.cub, num), nc->ttyfp, false);
}

int ncdirect_cursor_right(ncdirect* nc, int num){
  if(num < 0){
    return -1;
  }
  if(!nc->tcache.cuf){ // FIXME fall back to cuf1
    return -1;
  }
  return term_emit("cuf", tiparm(nc->tcache.cuf, num), nc->ttyfp, false);
}

int ncdirect_cursor_down(ncdirect* nc, int num){
  if(num < 0){
    return -1;
  }
  if(!nc->tcache.cud){
    return -1;
  }
  return term_emit("cud", tiparm(nc->tcache.cud, num), nc->ttyfp, false);
}

int ncdirect_clear(ncdirect* nc){
  if(!nc->tcache.clearscr){
    return -1; // FIXME scroll output off the screen
  }
  return term_emit("clear", nc->tcache.clearscr, nc->ttyfp, true);
}

int ncdirect_dim_x(const ncdirect* nc){
  int x;
  if(nc->ctermfd >= 0){
    if(update_term_dimensions(nc->ctermfd, nullptr, &x) == 0){
      return x;
    }
  }else{
    return 80; // lol
  }
  return -1;
}

int ncdirect_dim_y(const ncdirect* nc){
  int y;
  if(nc->ctermfd >= 0){
    if(update_term_dimensions(nc->ctermfd, &y, nullptr) == 0){
      return y;
    }
  }else{
    return 24; // lol
  }
  return -1;
}

int ncdirect_cursor_enable(ncdirect* nc){
  if(!nc->tcache.cnorm){
    return -1;
  }
  return term_emit("cnorm", nc->tcache.cnorm, nc->ttyfp, true);
}

int ncdirect_cursor_disable(ncdirect* nc){
  if(!nc->tcache.civis){
    return -1;
  }
  return term_emit("civis", nc->tcache.civis, nc->ttyfp, true);
}

int ncdirect_cursor_move_yx(ncdirect* n, int y, int x){
  if(y == -1){ // keep row the same, horizontal move only
    if(!n->tcache.hpa){
      return -1;
    }
    return term_emit("hpa", tiparm(n->tcache.hpa, x), n->ttyfp, false);
  }else if(x == -1){ // keep column the same, vertical move only
    if(!n->tcache.vpa){
      return -1;
    }
    return term_emit("vpa", tiparm(n->tcache.vpa, y), n->ttyfp, false);
  }
  if(n->tcache.cup){
    return term_emit("cup", tiparm(n->tcache.cup, y, x), n->ttyfp, false);
  }else if(n->tcache.vpa && n->tcache.hpa){
    if(term_emit("hpa", tiparm(n->tcache.hpa, x), n->ttyfp, false) == 0 &&
       term_emit("vpa", tiparm(n->tcache.vpa, y), n->ttyfp, false) == 0){
      return 0;
    }
  }
  return -1;
}

static int
cursor_yx_get(int ttyfd, int* y, int* x){
  if(write(ttyfd, "\033[6n", 4) != 4){
    return -1;
  }
  bool done = false;
  enum { // what we expect now
    CURSOR_ESC, // 27 (0x1b)
    CURSOR_LSQUARE,
    CURSOR_ROW, // delimited by a semicolon
    CURSOR_COLUMN,
    CURSOR_R,
  } state = CURSOR_ESC;
  int row = 0, column = 0;
  char in;
  while(read(ttyfd, &in, 1) == 1){
    bool valid = false;
    switch(state){
      case CURSOR_ESC: valid = (in == '\x1b'); state = CURSOR_LSQUARE; break;
      case CURSOR_LSQUARE: valid = (in == '['); state = CURSOR_ROW; break;
      case CURSOR_ROW:
        if(isdigit(in)){
          row *= 10;
          row += in - '0';
          valid = true;
        }else if(in == ';'){
          state = CURSOR_COLUMN;
          valid = true;
        }
        break;
      case CURSOR_COLUMN:
        if(isdigit(in)){
          column *= 10;
          column += in - '0';
          valid = true;
        }else if(in == 'R'){
          state = CURSOR_R;
          valid = true;
        }
        break;
      case CURSOR_R: default: // logical error, whoops
        break;
    }
    if(!valid){
      fprintf(stderr, "Unexpected result from terminal: %d\n", in);
      break;
    }
    if(state == CURSOR_R){
      done = true;
      break;
    }
  }
  if(!done){
    return -1;
  }
  if(y){
    *y = row;
  }
  if(x){
    *x = column;
  }
  return 0;
}

// no terminfo capability for this. dangerous! kmscon and possibly other
// terminals report y and x inverted from the normal form; we ought detect this
// using the algorithm in https://github.com/dankamongmen/notcurses/issues/784.
int ncdirect_cursor_yx(ncdirect* n, int* y, int* x){
  struct termios termio, oldtermios;
  // this only works for real terminals
  if(n->ctermfd < 0){
    return -1;
  }
  if(tcgetattr(n->ctermfd, &termio)){
    fprintf(stderr, "Couldn't get terminal info from %d (%s)\n", n->ctermfd, strerror(errno));
    return -1;
  }
  memcpy(&oldtermios, &termio, sizeof(termio));
  termio.c_lflag &= ~(ICANON | ECHO);
  if(tcsetattr(n->ctermfd, TCSAFLUSH, &termio)){
    fprintf(stderr, "Couldn't put terminal into cbreak mode via %d (%s)\n",
            n->ctermfd, strerror(errno));
    return -1;
  }
  int ret = cursor_yx_get(n->ctermfd, y, x);
  if(tcsetattr(n->ctermfd, TCSANOW, &oldtermios)){
    fprintf(stderr, "Couldn't restore terminal mode on %d (%s)\n",
            n->ctermfd, strerror(errno)); // don't return error for this
  }
  if(y){
    --*y;
  }
  if(x){
    --*x;
  }
  return ret;
}

int ncdirect_cursor_push(ncdirect* n){
  if(n->tcache.sc == nullptr){
    return -1;
  }
  return term_emit("sc", n->tcache.sc, n->ttyfp, false);
}

int ncdirect_cursor_pop(ncdirect* n){
  if(n->tcache.rc == nullptr){
    return -1;
  }
  return term_emit("rc", n->tcache.rc, n->ttyfp, false);
}

static inline int
ncdirect_align(const struct ncdirect* n, ncalign_e align, int c){
  if(align == NCALIGN_LEFT){
    return 0;
  }
  int cols = ncdirect_dim_x(n);
  if(c > cols){
    return 0;
  }
  if(align == NCALIGN_CENTER){
    return (cols - c) / 2;
  }else if(align == NCALIGN_RIGHT){
    return cols - c;
  }
  return INT_MAX;
}

static int
ncdirect_dump_plane(ncdirect* n, const ncplane* np, int xoff){
  const int totx = ncdirect_dim_x(n);
  const int toty = ncdirect_dim_y(n);
  int dimy, dimx;
  ncplane_dim_yx(np, &dimy, &dimx);
//fprintf(stderr, "rasterizing %dx%d+%d\n", dimy, dimx, xoff);
  assert(dimx + xoff <= totx);
  for(int y = 0 ; y < dimy ; ++y){
    if(xoff){
      if(ncdirect_cursor_move_yx(n, -1, xoff)){
        return -1;
      }
    }
    for(int x = 0 ; x < dimx ; ++x){
      uint16_t stylemask;
      uint64_t channels;
      char* egc = ncplane_at_yx(np, y, x, &stylemask, &channels);
      if(egc == nullptr){
        return -1;
      }
      ncdirect_fg(n, channels_fg(channels));
      ncdirect_bg(n, channels_bg(channels));
//fprintf(stderr, "%03d/%03d [%s] (%03dx%03d)\n", y, x, egc, dimy, dimx);
      if(fprintf(n->ttyfp, "%s", strlen(egc) == 0 ? " " : egc) < 0){
        free(egc);
        return -1;
      }
      free(egc);
    }
    if(dimx < totx){
      ncdirect_bg_default(n);
      if(putc('\n', n->ttyfp) == EOF){
        return -1;
      }
    }
    if(y == toty){
      if(ncdirect_cursor_down(n, 1)){
        return -1;
      }
    }
  }
  return 0;
}

int ncdirect_render_image(ncdirect* n, const char* file, ncalign_e align,
                          ncblitter_e blitter, ncscale_e scale){
  struct ncvisual* ncv = ncvisual_from_file(file);
  if(ncv == nullptr){
    return -1;
  }
//fprintf(stderr, "OUR DATA: %p rows/cols: %d/%d\n", ncv->data, ncv->rows, ncv->cols);
  int leny = ncv->rows; // we allow it to freely scroll
  int lenx = ncv->cols;
  if(leny == 0 || lenx == 0){
    ncvisual_destroy(ncv);
    return -1;
  }
//fprintf(stderr, "render %d/%d to %d+%dx%d scaling: %d\n", ncv->rows, ncv->cols, leny, lenx, scale);
  auto bset = rgba_blitter_low(n->utf8, scale, true, blitter);
  if(!bset){
    return -1;
  }
  int disprows, dispcols;
  if(scale != NCSCALE_NONE){
    dispcols = ncdirect_dim_x(n) * encoding_x_scale(bset);
    disprows = ncdirect_dim_y(n) * encoding_y_scale(bset);
    if(scale == NCSCALE_SCALE){
      scale_visual(ncv, &disprows, &dispcols);
    }
  }else{
    disprows = ncv->rows;
    dispcols = ncv->cols / encoding_x_scale(bset);
  }
  leny = (leny / (double)ncv->rows) * ((double)disprows);
  lenx = (lenx / (double)ncv->cols) * ((double)dispcols);
//fprintf(stderr, "render: %d+%d of %d/%d stride %u %p\n", leny, lenx, ncv->rows, ncv->cols, ncv->rowstride, ncv->data);
  struct ncplane* faken = ncplane_create(nullptr, nullptr,
                                         disprows / encoding_y_scale(bset),
                                         dispcols / encoding_x_scale(bset),
                                         0, 0, nullptr, nullptr);
  if(faken == nullptr){
    return -1;
  }
  if(ncvisual_blit(ncv, disprows, dispcols, faken, bset,
                   0, 0, 0, 0, leny, lenx, false)){
    ncvisual_destroy(ncv);
    free_plane(faken);
    return -1;
  }
  ncvisual_destroy(ncv);
  int xoff = ncdirect_align(n, align, lenx / encoding_x_scale(bset));
  if(ncdirect_dump_plane(n, faken, xoff)){
    return -1;
  }
  while(fflush(stdout) == EOF && errno == EAGAIN){
    ;
  }
  free_plane(faken);
  ncdirect_fg_default(n);
  ncdirect_bg_default(n);
  return 0;
}

int ncdirect_fg_palindex(ncdirect* nc, int pidx){
  return term_emit("setaf", tiparm(nc->tcache.setaf, pidx), nc->ttyfp, false);
}

int ncdirect_bg_palindex(ncdirect* nc, int pidx){
  return term_emit("setab", tiparm(nc->tcache.setab, pidx), nc->ttyfp, false);
}

int ncdirect_vprintf_aligned(ncdirect* n, int y, ncalign_e align, const char* fmt, va_list ap){
  char* r = ncplane_vprintf_prep(fmt, ap);
  if(r == nullptr){
    return -1;
  }
  const size_t len = strlen(r);
  const int x = ncdirect_align(n, align, len);
  if(ncdirect_cursor_move_yx(n, y, x)){
    free(r);
    return -1;
  }
  int ret = puts(r);
  free(r);
  if(ret == EOF){
    return -1;
  }
  return ret;
}

int ncdirect_printf_aligned(ncdirect* n, int y, ncalign_e align, const char* fmt, ...){
  va_list va;
  va_start(va, fmt);
  int ret = ncdirect_vprintf_aligned(n, y, align, fmt, va);
  va_end(va);
  return ret;
}

int get_controlling_tty(FILE* ttyfp){
  int fd = fileno(ttyfp);
  if(fd > 0 && isatty(fd)){
    if((fd = dup(fd)) >= 0){
      return fd;
    }
  }
  char cbuf[L_ctermid + 1];
  if(ctermid(cbuf) == nullptr){
    return -1;
  }
  return open(cbuf, O_RDWR | O_CLOEXEC);
}

ncdirect* ncdirect_init(const char* termtype, FILE* outfp, uint64_t flags){
  if(flags){ // allow them through with warning
    logwarn((struct notcurses*)NULL, "Passed non-zero flags 0x%016jx, but no flags are defined\n", (uintmax_t)flags);
  }
  if(outfp == nullptr){
    outfp = stdout;
  }
  auto ret = new ncdirect{};
  if(ret == nullptr){
    return ret;
  }
  ret->ttyfp = outfp;
  memset(&ret->palette, 0, sizeof(ret->palette));
  init_lang(nullptr);
  const char* encoding = nl_langinfo(CODESET);
  if(encoding && strcmp(encoding, "UTF-8") == 0){
    ret->utf8 = true;
  }
  // we don't need a controlling tty for everything we do; allow a failure here
  if((ret->ctermfd = get_controlling_tty(ret->ttyfp)) >= 0){
    if(cbreak_mode(ret->ctermfd, &ret->tpreserved)){
      goto err;
    }
  }
  if(ncinputlayer_init(&ret->input, stdin)){
    goto err;
  }
  int termerr;
  if(setupterm(termtype, ret->ctermfd, &termerr) != OK){
    fprintf(stderr, "Terminfo error %d (see terminfo(3ncurses))\n", termerr);
    goto err;
  }
  if(ncvisual_init(ffmpeg_log_level(NCLOGLEVEL_SILENT))){
    goto err;
  }
  if(interrogate_terminfo(&ret->tcache)){
    goto err;
  }
  ret->fgdefault = ret->bgdefault = true;
  ret->fgrgb = ret->bgrgb = 0;
  ncdirect_styles_set(ret, 0);
  return ret;

err:
  if(ret->ctermfd >= 0){
    tcsetattr(ret->ctermfd, TCSANOW, &ret->tpreserved);
  }
  delete(ret);
  return nullptr;
}

int ncdirect_stop(ncdirect* nc){
  int ret = 0;
  if(nc){
    if(nc->tcache.op && term_emit("op", nc->tcache.op, nc->ttyfp, true)){
      ret = -1;
    }
    if(nc->tcache.sgr0 && term_emit("sgr0", nc->tcache.sgr0, nc->ttyfp, true)){
      ret = -1;
    }
    if(nc->tcache.oc && term_emit("oc", nc->tcache.oc, nc->ttyfp, true)){
      ret = -1;
    }
    if(nc->ctermfd >= 0){
      if(nc->tcache.cnorm && tty_emit("cnorm", nc->tcache.cnorm, nc->ctermfd)){
        ret = -1;
      }
      ret |= tcsetattr(nc->ctermfd, TCSANOW, &nc->tpreserved);
      ret |= close(nc->ctermfd);
    }
    input_free_esctrie(&nc->input.inputescapes);
    delete(nc);
  }
  return ret;
}

static inline int
ncdirect_style_emit(ncdirect* n, const char* sgr, unsigned stylebits, FILE* out){
  if(sgr == nullptr){
    return -1;
  }
  int r = term_emit("sgr", tiparm(sgr, stylebits & NCSTYLE_STANDOUT,
                                  stylebits & NCSTYLE_UNDERLINE,
                                  stylebits & NCSTYLE_REVERSE,
                                  stylebits & NCSTYLE_BLINK,
                                  stylebits & NCSTYLE_DIM,
                                  stylebits & NCSTYLE_BOLD,
                                  stylebits & NCSTYLE_INVIS,
                                  stylebits & NCSTYLE_PROTECT, 0), out, false);
  // sgr resets colors, so set them back up if not defaults
  if(r == 0){
    if(!n->fgdefault){
      r |= ncdirect_fg(n, n->fgrgb);
    }
    if(!n->bgdefault){
      r |= ncdirect_bg(n, n->bgrgb);
    }
  }
  return r;
}

int ncdirect_styles_on(ncdirect* n, unsigned stylebits){
  uint32_t stylemask = n->stylemask | stylebits;
  if(ncdirect_style_emit(n, n->tcache.sgr, stylemask, n->ttyfp) == 0){
    if(term_setstyle(n->ttyfp, n->stylemask, stylemask, NCSTYLE_ITALIC,
                     n->tcache.italics, n->tcache.italoff) == 0){
      n->stylemask = stylemask;
      return 0;
    }
  }
  return -1;
}

// turn off any specified stylebits
int ncdirect_styles_off(ncdirect* n, unsigned stylebits){
  uint32_t stylemask = n->stylemask & ~stylebits;
  if(ncdirect_style_emit(n, n->tcache.sgr, stylemask, n->ttyfp) == 0){
    if(term_setstyle(n->ttyfp, n->stylemask, stylemask, NCSTYLE_ITALIC,
                     n->tcache.italics, n->tcache.italoff) == 0){
      n->stylemask = stylemask;
      return 0;
    }
  }
  return -1;
}

// set the current stylebits to exactly those provided
int ncdirect_styles_set(ncdirect* n, unsigned stylebits){
  uint32_t stylemask = stylebits;
  if(ncdirect_style_emit(n, n->tcache.sgr, stylemask, n->ttyfp) == 0){
    if(term_setstyle(n->ttyfp, n->stylemask, stylemask, NCSTYLE_ITALIC,
                     n->tcache.italics, n->tcache.italoff) == 0){
      n->stylemask = stylemask;
      return 0;
    }
  }
  return -1;
}

unsigned ncdirect_palette_size(const ncdirect* nc){
  return nc->tcache.colors;
}

int ncdirect_fg_default(ncdirect* nc){
  if(term_emit("op", nc->tcache.op, nc->ttyfp, false) == 0){
    nc->fgdefault = true;
    if(nc->bgdefault){
      return 0;
    }
    return ncdirect_bg(nc, nc->bgrgb);
  }
  return -1;
}

int ncdirect_bg_default(ncdirect* nc){
  if(term_emit("op", nc->tcache.op, nc->ttyfp, false) == 0){
    nc->bgdefault = true;
    if(nc->fgdefault){
      return 0;
    }
    return ncdirect_fg(nc, nc->fgrgb);
  }
  return -1;
}

int ncdirect_hline_interp(ncdirect* n, const char* egc, int len,
                          uint64_t c1, uint64_t c2){
  unsigned ur, ug, ub;
  int r1, g1, b1, r2, g2, b2;
  int br1, bg1, bb1, br2, bg2, bb2;
  channels_fg_rgb(c1, &ur, &ug, &ub);
  r1 = ur; g1 = ug; b1 = ub;
  channels_fg_rgb(c2, &ur, &ug, &ub);
  r2 = ur; g2 = ug; b2 = ub;
  channels_bg_rgb(c1, &ur, &ug, &ub);
  br1 = ur; bg1 = ug; bb1 = ub;
  channels_bg_rgb(c2, &ur, &ug, &ub);
  br2 = ur; bg2 = ug; bb2 = ub;
  int deltr = r2 - r1;
  int deltg = g2 - g1;
  int deltb = b2 - b1;
  int deltbr = br2 - br1;
  int deltbg = bg2 - bg1;
  int deltbb = bb2 - bb1;
  int ret;
  bool fgdef = false, bgdef = false;
  if(channels_fg_default_p(c1) && channels_fg_default_p(c2)){
    fgdef = true;
  }
  if(channels_bg_default_p(c1) && channels_bg_default_p(c2)){
    bgdef = true;
  }
  for(ret = 0 ; ret < len ; ++ret){
    int r = (deltr * ret) / len + r1;
    int g = (deltg * ret) / len + g1;
    int b = (deltb * ret) / len + b1;
    int br = (deltbr * ret) / len + br1;
    int bg = (deltbg * ret) / len + bg1;
    int bb = (deltbb * ret) / len + bb1;
    if(!fgdef){
      ncdirect_fg_rgb(n, r, g, b);
    }
    if(!bgdef){
      ncdirect_bg_rgb(n, br, bg, bb);
    }
    if(fprintf(n->ttyfp, "%s", egc) < 0){
      break;
    }
  }
  return ret;
}

int ncdirect_vline_interp(ncdirect* n, const char* egc, int len,
                          uint64_t c1, uint64_t c2){
  unsigned ur, ug, ub;
  int r1, g1, b1, r2, g2, b2;
  int br1, bg1, bb1, br2, bg2, bb2;
  channels_fg_rgb(c1, &ur, &ug, &ub);
  r1 = ur; g1 = ug; b1 = ub;
  channels_fg_rgb(c2, &ur, &ug, &ub);
  r2 = ur; g2 = ug; b2 = ub;
  channels_bg_rgb(c1, &ur, &ug, &ub);
  br1 = ur; bg1 = ug; bb1 = ub;
  channels_bg_rgb(c2, &ur, &ug, &ub);
  br2 = ur; bg2 = ug; bb2 = ub;
  int deltr = (r2 - r1) / (len + 1);
  int deltg = (g2 - g1) / (len + 1);
  int deltb = (b2 - b1) / (len + 1);
  int deltbr = (br2 - br1) / (len + 1);
  int deltbg = (bg2 - bg1) / (len + 1);
  int deltbb = (bb2 - bb1) / (len + 1);
  int ret;
  bool fgdef = false, bgdef = false;
  if(channels_fg_default_p(c1) && channels_fg_default_p(c2)){
    fgdef = true;
  }
  if(channels_bg_default_p(c1) && channels_bg_default_p(c2)){
    bgdef = true;
  }
  for(ret = 0 ; ret < len ; ++ret){
    r1 += deltr;
    g1 += deltg;
    b1 += deltb;
    br1 += deltbr;
    bg1 += deltbg;
    bb1 += deltbb;
    uint64_t channels = 0;
    if(!fgdef){
      channels_set_fg_rgb(&channels, r1, g1, b1);
    }
    if(!bgdef){
      channels_set_bg_rgb(&channels, br1, bg1, bb1);
    }
    if(ncdirect_putstr(n, channels, egc) <= 0){
      break;
    }
    if(len - ret > 1){
      if(ncdirect_cursor_down(n, 1) || ncdirect_cursor_left(n, 1)){
        break;
      }
    }
  }
  return ret;
}

//  wchars: wchar_t[6] mapping to UL, UR, BL, BR, HL, VL.
//  they cannot be complex EGCs, but only a single wchar_t, alas.
int ncdirect_box(ncdirect* n, uint64_t ul, uint64_t ur,
                 uint64_t ll, uint64_t lr, const wchar_t* wchars,
                 int ylen, int xlen, unsigned ctlword){
  if(xlen < 2 || ylen < 2){
    return -1;
  }
  char hl[WCHAR_MAX_UTF8BYTES + 1];
  char vl[WCHAR_MAX_UTF8BYTES + 1];
  unsigned edges;
  edges = !(ctlword & NCBOXMASK_TOP) + !(ctlword & NCBOXMASK_LEFT);
  if(edges >= box_corner_needs(ctlword)){
    ncdirect_fg(n, channels_fg(ul));
    ncdirect_bg(n, channels_bg(ul));
    if(fprintf(n->ttyfp, "%lc", wchars[0]) < 0){
      return -1;
    }
  }else{
    ncdirect_cursor_right(n, 1);
  }
  mbstate_t ps = {};
  size_t bytes;
  if((bytes = wcrtomb(hl, wchars[4], &ps)) == (size_t)-1){
    return -1;
  }
  hl[bytes] = '\0';
  memset(&ps, 0, sizeof(ps));
  if((bytes = wcrtomb(vl, wchars[5], &ps)) == (size_t)-1){
    return -1;
  }
  vl[bytes] = '\0';
  if(!(ctlword & NCBOXMASK_TOP)){ // draw top border, if called for
    if(xlen > 2){
      if(ncdirect_hline_interp(n, hl, xlen - 2, ul, ur) < 0){
        return -1;
      }
    }
  }else{
    ncdirect_cursor_right(n, xlen - 2);
  }
  edges = !(ctlword & NCBOXMASK_TOP) + !(ctlword & NCBOXMASK_RIGHT);
  if(edges >= box_corner_needs(ctlword)){
    ncdirect_fg(n, channels_fg(ur));
    ncdirect_bg(n, channels_bg(ur));
    if(fprintf(n->ttyfp, "%lc", wchars[1]) < 0){
      return -1;
    }
    ncdirect_cursor_left(n, xlen);
  }else{
    ncdirect_cursor_left(n, xlen - 1);
  }
  ncdirect_cursor_down(n, 1);
  // middle rows (vertical lines)
  if(ylen > 2){
    if(!(ctlword & NCBOXMASK_LEFT)){
      if(ncdirect_vline_interp(n, vl, ylen - 2, ul, ll) < 0){
        return -1;
      }
      ncdirect_cursor_right(n, xlen - 2);
      ncdirect_cursor_up(n, ylen - 3);
    }else{
      ncdirect_cursor_right(n, xlen - 1);
    }
    if(!(ctlword & NCBOXMASK_RIGHT)){
      if(ncdirect_vline_interp(n, vl, ylen - 2, ur, lr) < 0){
        return -1;
      }
      ncdirect_cursor_left(n, xlen);
    }else{
      ncdirect_cursor_left(n, xlen - 1);
    }
  }
  ncdirect_cursor_down(n, 1);
  // bottom line
  edges = !(ctlword & NCBOXMASK_BOTTOM) + !(ctlword & NCBOXMASK_LEFT);
  if(edges >= box_corner_needs(ctlword)){
    ncdirect_fg(n, channels_fg(ll));
    ncdirect_bg(n, channels_bg(ll));
    if(fprintf(n->ttyfp, "%lc", wchars[2]) < 0){
      return -1;
    }
  }else{
    ncdirect_cursor_right(n, 1);
  }
  if(!(ctlword & NCBOXMASK_BOTTOM)){
    if(xlen > 2){
      if(ncdirect_hline_interp(n, hl, xlen - 2, ll, lr) < 0){
        return -1;
      }
    }
  }else{
    ncdirect_cursor_right(n, xlen - 2);
  }
  edges = !(ctlword & NCBOXMASK_BOTTOM) + !(ctlword & NCBOXMASK_RIGHT);
  if(edges >= box_corner_needs(ctlword)){
    ncdirect_fg(n, channels_fg(lr));
    ncdirect_bg(n, channels_bg(lr));
    if(fprintf(n->ttyfp, "%lc", wchars[3]) < 0){
      return -1;
    }
  }
  return 0;
}

int ncdirect_rounded_box(ncdirect* n, uint64_t ul, uint64_t ur,
                         uint64_t ll, uint64_t lr,
                         int ylen, int xlen, unsigned ctlword){
  return ncdirect_box(n, ul, ur, ll, lr, L"╭╮╰╯─│", ylen, xlen, ctlword);
}

int ncdirect_double_box(ncdirect* n, uint64_t ul, uint64_t ur,
                         uint64_t ll, uint64_t lr,
                         int ylen, int xlen, unsigned ctlword){
  return ncdirect_box(n, ul, ur, ll, lr, L"╔╗╚╝═║", ylen, xlen, ctlword);
}

// Can we load images? This requires being built against FFmpeg/OIIO.
bool ncdirect_canopen_images(const ncdirect* n){
  (void)n;
#ifdef USE_FFMPEG
  return true;
#else
#ifdef USE_OIIO
  return true;
#endif
#endif
  return false;
}

// Is our encoding UTF-8? Requires LANG being set to a UTF8 locale.
bool ncdirect_canutf8(const ncdirect* n){
  return n->utf8;
}

int ncdirect_flush(const ncdirect* nc){
  if(fflush(nc->ttyfp) == EOF){
    return -1;
  }
  return 0;
}
