#include <ncurses.h> // needed for some definitions, see terminfo(3ncurses)
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <unistd.h>
#include <termios.h>
#include "version.h"
#include "visual-details.h"
#include "internal.h"

int ncdirect_putc(ncdirect* nc, uint64_t channels, const char* egc){
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
  return fprintf(nc->ttyfp, "%s", egc);
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
  if(!nc->tcache.clear){
    return -1; // FIXME scroll output off the screen
  }
  return term_emit("clear", nc->tcache.clear, nc->ttyfp, true);
}

int ncdirect_dim_x(const ncdirect* nc){
  int x;
  if(nc->ctermfd >= 0){
    if(update_term_dimensions(nc->ctermfd, NULL, &x) == 0){
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
    if(update_term_dimensions(nc->ctermfd, &y, NULL) == 0){
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

// no terminfo capability for this. dangerous!
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
  if(n->tcache.sc == NULL){
    return -1;
  }
  return term_emit("sc", n->tcache.sc, n->ttyfp, false);
}

int ncdirect_cursor_pop(ncdirect* n){
  if(n->tcache.rc == NULL){
    return -1;
  }
  return term_emit("rc", n->tcache.rc, n->ttyfp, false);
}

static int
ncdirect_dump_plane(ncdirect* n, const ncplane* np, int xoff){
  const int totx = ncdirect_dim_x(n);
  const int toty = ncdirect_dim_y(n);
  int dimy, dimx;
  ncplane_dim_yx(np, &dimy, &dimx);
fprintf(stderr, "rasterizing %dx%d\n", dimy, dimx);
  for(int y = 0 ; y < dimy ; ++y){
    if(ncdirect_cursor_move_yx(n, -1, xoff)){
      return -1;
    }
    for(int x = 0 ; x < xoff ; ++x){
      if(putchar(' ') == EOF){
        return -1;
      }
    }
    for(int x = 0 ; x < dimx ; ++x){
      uint32_t attrword;
      uint64_t channels;
      char* egc = ncplane_at_yx(np, y, x, &attrword, &channels);
      if(egc == NULL){
        return -1;
      }
      ncdirect_fg(n, channels_fg(channels));
      ncdirect_bg(n, channels_bg(channels));
//fprintf(stderr, "%03d/%03d [%s] (%03dx%03d)\n", y, x, egc, dimy, dimx);
      if(printf("%s", strlen(egc) == 0 ? " " : egc) < 0){
        return -1;
      }
    }
    // FIXME mystifyingly, we require this cursor_left() when using 2x2, but must
    // not have it when using 2x1 (we insert blank lines otherwise). don't paper
    // over it with a conditional, but instead get to the bottom of this FIXME.
    if(dimx < totx){
      ncdirect_bg_default(n);
      if(putchar('\n') == EOF){
        return -1;
      }
    }
    if(y == toty){
      ncdirect_cursor_down(n, 1);
    }
  }
  return 0;
}

nc_err_e ncdirect_render_image(ncdirect* n, const char* file, ncalign_e align,
                               ncblitter_e blitter, ncscale_e scale){
  nc_err_e ret;
  struct ncvisual* ncv = ncvisual_from_file(file, &ret);
  if(ncv == NULL){
    return ret;
  }
//fprintf(stderr, "OUR DATA: %p rows/cols: %d/%d\n", ncv->data, ncv->rows, ncv->cols);
  int leny = ncv->rows; // we allow it to freely scroll
  int lenx = ncv->cols;
  if(leny == 0 || lenx == 0){
    ncvisual_destroy(ncv);
    return NCERR_DECODE;
  }
//fprintf(stderr, "render %d/%d to %d+%dx%d scaling: %d\n", ncv->rows, ncv->cols, leny, lenx, scale);
  auto bset = rgba_blitter_low(n->utf8, scale, true, blitter);
  if(!bset){
    return NCERR_INVALID_ARG;
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
  struct ncplane* faken = ncplane_create(NULL, NULL,
                                         disprows / encoding_y_scale(bset),
                                         dispcols,// / encoding_x_scale(bset),
                                         0, 0, NULL);
  if(faken == NULL){
    return NCERR_NOMEM;
  }
  if(ncvisual_blit(ncv, disprows, dispcols, faken, bset,
                   0, 0, 0, 0, leny, lenx, false)){
    ncvisual_destroy(ncv);
    free_plane(faken);
    return NCERR_SYSTEM;
  }
  ncvisual_destroy(ncv);
  int xoff = 0;
  if(ncdirect_dump_plane(n, faken, xoff)){
    return NCERR_SYSTEM;
  }
  free_plane(faken);
  return NCERR_SUCCESS;
}

int ncdirect_fg_palindex(ncdirect* nc, int pidx){
  return term_emit("setaf", tiparm(nc->tcache.setaf, pidx), nc->ttyfp, false);
}

int ncdirect_bg_palindex(ncdirect* nc, int pidx){
  return term_emit("setab", tiparm(nc->tcache.setab, pidx), nc->ttyfp, false);
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

int ncdirect_vprintf_aligned(ncdirect* n, int y, ncalign_e align, const char* fmt, va_list ap){
  char* r = ncplane_vprintf_prep(fmt, ap);
  if(r == NULL){
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
  if(ctermid(cbuf) == NULL){
    return -1;
  }
  return open(cbuf, O_RDWR | O_CLOEXEC);
}

ncdirect* ncdirect_init(const char* termtype, FILE* outfp){
  if(outfp == NULL){
    outfp = stdout;
  }
  auto ret = new ncdirect{};
  if(ret == NULL){
    return ret;
  }
  ret->ttyfp = outfp;
  memset(&ret->palette, 0, sizeof(ret->palette));
  // we don't need a controlling tty for everything we do; allow a failure here
  ret->ctermfd = get_controlling_tty(ret->ttyfp);
  int termerr;
  if(setupterm(termtype, ret->ctermfd, &termerr) != OK){
    fprintf(stderr, "Terminfo error %d (see terminfo(3ncurses))\n", termerr);
    delete(ret);
    return NULL;
  }
  if(interrogate_terminfo(&ret->tcache)){
    delete(ret);
    return NULL;
  }
  ret->fgdefault = ret->bgdefault = true;
  ret->fgrgb = ret->bgrgb = 0;
  ncdirect_styles_set(ret, 0);
  const char* encoding = nl_langinfo(CODESET);
  if(encoding && strcmp(encoding, "UTF-8") == 0){
    ret->utf8 = true;
  }else if(encoding && strcmp(encoding, "ANSI_X3.4-1968") == 0){
    ret->utf8 = false;
  }
  return ret;
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
      ret |= close(nc->ctermfd);
    }
    delete(nc);
  }
  return ret;
}

