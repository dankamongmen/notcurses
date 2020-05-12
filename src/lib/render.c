#include <ctype.h>
#include <limits.h>
#include <unistd.h>
#include <sys/poll.h>
#include "internal.h"

// Check whether the terminal geometry has changed, and if so, copies what can
// be copied from the old stdscr. Assumes that the screen is always anchored at
// the same origin. Also syncs up lastframe.
int notcurses_resize(notcurses* n, int* restrict rows, int* restrict cols){
  int r, c;
  if(rows == NULL){
    rows = &r;
  }
  if(cols == NULL){
    cols = &c;
  }
  int oldrows = n->stdscr->leny;
  int oldcols = n->stdscr->lenx;
  if(update_term_dimensions(n->ttyfd, rows, cols)){
    return -1;
  }
  n->truecols = *cols;
  *rows -= n->margin_t + n->margin_b;
  if(*rows <= 0){
    *rows = 1;
  }
  *cols -= n->margin_l + n->margin_r;
  if(*cols <= 0){
    *cols = 1;
  }
  if(*rows != n->lfdimy || *cols != n->lfdimx){
    n->lfdimy = *rows;
    n->lfdimx = *cols;
    const size_t size = sizeof(*n->lastframe) * (n->lfdimy * n->lfdimx);
    cell* fb = realloc(n->lastframe, size);
    if(fb == NULL){
      return -1;
    }
    n->lastframe = fb;
    // FIXME more memset()tery than we need, both wasting work and wrecking
    // damage detection for the upcoming render
    memset(n->lastframe, 0, size);
    egcpool_dump(&n->pool);
  }
  if(*rows == oldrows && *cols == oldcols){
    return 0; // no change
  }
  int keepy = *rows;
  if(keepy > oldrows){
    keepy = oldrows;
  }
  int keepx = *cols;
  if(keepx > oldcols){
    keepx = oldcols;
  }
  if(ncplane_resize_internal(n->stdscr, 0, 0, keepy, keepx, 0, 0, *rows, *cols)){
    return -1;
  }
  return 0;
}

static int
blocking_write(int fd, const char* buf, size_t buflen){
//fprintf(stderr, "writing %zu to %d...\n", buflen, fd);
  size_t written = 0;
  while(written < buflen){
    ssize_t w = write(fd, buf + written, buflen - written);
    if(w < 0){
      if(errno != EAGAIN && errno != EWOULDBLOCK){
        return -1;
      }
    }else{
      written += w;
    }
    if(written < buflen){
      struct pollfd pfd = {
        .fd = fd,
        .events = POLLOUT,
        .revents = 0,
      };
      poll(&pfd, 1, -1);
    }
  }
  return 0;
}

static void
update_render_stats(const struct timespec* time1, const struct timespec* time0,
                    ncstats* stats, int bytes){
  int64_t elapsed = timespec_to_ns(time1) - timespec_to_ns(time0);
  //fprintf(stderr, "Rendering took %ld.%03lds\n", elapsed / NANOSECS_IN_SEC,
  //        (elapsed % NANOSECS_IN_SEC) / 1000000);
  if(bytes >= 0){
    stats->render_bytes += bytes;
    if(bytes > stats->render_max_bytes){
      stats->render_max_bytes = bytes;
    }
    if(bytes < stats->render_min_bytes){
      stats->render_min_bytes = bytes;
    }
  }else{
    ++stats->failed_renders;
  }
  if(elapsed > 0){ // don't count clearly incorrect information, egads
    ++stats->renders;
    stats->render_ns += elapsed;
    if(elapsed > stats->render_max_ns){
      stats->render_max_ns = elapsed;
    }
    if(elapsed < stats->render_min_ns){
      stats->render_min_ns = elapsed;
    }
  }
}

void cell_release(ncplane* n, cell* c){
  pool_release(&n->pool, c);
}

// Duplicate one cell onto another when they share a plane. Convenience wrapper.
int cell_duplicate(ncplane* n, cell* targ, const cell* c){
  return cell_duplicate_far(&n->pool, targ, n, c);
}

// the heart of damage detection. compare two cells (from two different planes)
// for equality. if they are equal, return 0. otherwise, dup the second onto
// the first and return non-zero.
static int
cellcmp_and_dupfar(egcpool* dampool, cell* damcell,
                   const ncplane* srcplane, const cell* srccell){

  if(damcell->attrword == srccell->attrword){
    if(damcell->channels == srccell->channels){
      bool srcsimple = cell_simple_p(srccell);
      bool damsimple = cell_simple_p(damcell);
      if(damsimple == srcsimple){
        if(damsimple){
          if(damcell->gcluster == srccell->gcluster){
            return 0; // simple match
          }
        }else{
          const char* damegc = egcpool_extended_gcluster(dampool, damcell);
          const char* srcegc = extended_gcluster(srcplane, srccell);
          if(strcmp(damegc, srcegc) == 0){
            return 0; // EGC match
          }
        }
      }
    }
  }
  cell_duplicate_far(dampool, damcell, srcplane, srccell);
  return 1;
}

// Is this cell locked in? I.e. does it have all three of:
//  * a selected EGC
//  * CELL_ALPHA_OPAQUE foreground channel
//  * CELL_ALPHA_OPAQUE background channel
static inline bool
cell_locked_p(const cell* p){
  if(p->gcluster || cell_double_wide_p(p)){
    if(cell_fg_alpha(p) == CELL_ALPHA_OPAQUE){
      if(cell_bg_alpha(p) == CELL_ALPHA_OPAQUE){
        return 1;
      }
    }
  }
  return 0;
}

// Extracellular state for a cell during the render process. This array is
// passed along to rasterization, which uses only the 'damaged' bools.
struct crender {
  ncplane *p;
  unsigned fgblends;
  unsigned bgblends;
  bool damaged;       // also used in rasterization
  // if CELL_ALPHA_HIGHCONTRAST is in play, we apply the HSV flip once the
  // background is locked in. set highcontrast to indicate this.
  bool highcontrast;
};

// Emit fchannel with RGB changed to contrast effectively against bchannel.
static uint32_t
highcontrast(uint32_t bchannel){
  unsigned r = channel_r(bchannel);
  unsigned g = channel_g(bchannel);
  unsigned b = channel_b(bchannel);
  uint32_t conrgb = 0;
  /*
  float lumi = 0.2126 * r + 0.7152 * g + 0.0722 * b;
  unsigned max = r > g ? r > b ? r : b : g > b ? g : b;
  unsigned min = r < g ? r < b ? r : b : g < b ? g : b;
  float rrgb = r / 255.0;
  float grgb = g / 255.0;
  float brgb = b / 255.0;
  float rrel = rrgb <= 0.03928 ? rrgb / 12.92 : pow(((rrgb + 0.055) / 1.055), 2.4);
  float grel = grgb <= 0.03928 ? grgb / 12.92 : pow(((grgb + 0.055) / 1.055), 2.4);
  float brel = brgb <= 0.03928 ? brgb / 12.92 : pow(((brgb + 0.055) / 1.055), 2.4);
  max = !max ? 1 : max;
  unsigned sat = 10 * (max - min) / max;
  if(sat > 6){
    conrgb = 0xffffff;
  }else{
    conrgb = 0;
  }
  */
  if(r + g + b < 320){
    channel_set(&conrgb, 0xffffff);
  }else{
    channel_set(&conrgb, 0);
  }
  return conrgb;
}

// adjust an otherwise locked-in cell if highcontrast has been requested. this
// should be done at the end of rendering the cell, so that contrast is solved
// against the real background.
static inline void
lock_in_highcontrast(cell* targc, struct crender* crender){
  if(crender->highcontrast){
    // highcontrast weighs the original at 1/4 and the contrast at 3/4
    if(!cell_fg_default_p(targc)){
      crender->fgblends = 3;
      uint32_t fchan = cell_fchannel(targc);
      uint32_t bchan = cell_bchannel(targc);
      cell_set_fchannel(targc, channels_blend(highcontrast(bchan), fchan, &crender->fgblends));
    }else{
      cell_set_fg(targc, highcontrast(cell_bchannel(targc)));
    }
  }
}

// Paints a single ncplane into the provided scratch framebuffer 'fb', and
// ultimately 'lastframe' (we can't always write directly into 'lastframe',
// because we need build state to solve certain cells, and need compare their
// solved result to the last frame). Whenever a cell is locked in, it is
// compared against the last frame. If it is different, the 'rvec' bitmap is updated with a 1. 'pool' is typically nc->pool, but can
// be whatever's backing fb.
static int
paint(ncplane* p, cell* lastframe, struct crender* rvec,
      cell* fb, egcpool* pool, int dstleny, int dstlenx,
      int dstabsy, int dstabsx, int lfdimx){
  int y, x, dimy, dimx, offy, offx;
  ncplane_dim_yx(p, &dimy, &dimx);
  offy = p->absy - dstabsy;
  offx = p->absx - dstabsx;
//fprintf(stderr, "PLANE %p %d %d %d %d %d %d\n", p, dimy, dimx, offy, offx, dstleny, dstlenx);
  // skip content above or to the left of the physical screen
  int starty, startx;
  if(offy < 0){
    starty = -offy;
  }else{
    starty = 0;
  }
  if(offx < 0){
    startx = -offx;
  }else{
    startx = 0;
  }
  for(y = starty ; y < dimy ; ++y){
    const int absy = y + offy;
    // once we've passed the physical screen's bottom, we're done
    if(absy >= dstleny){
      break;
    }
    for(x = startx ; x < dimx ; ++x){
      const int absx = x + offx;
      if(absx >= dstlenx){
        break;
      }
      cell* targc = &fb[fbcellidx(absy, dstlenx, absx)];
      if(cell_locked_p(targc)){
        continue;
      }
      struct crender* crender = &rvec[fbcellidx(absy, dstlenx, absx)];
      const cell* vis = &p->fb[nfbcellidx(p, y, x)];
      // if we never loaded any content into the cell (or obliterated it by
      // writing in a zero), use the plane's base cell.
      if(vis->gcluster == 0 && !cell_wide_right_p(vis)){
        vis = &p->basecell;
      }
      // if we have no character in this cell, we continue to look for a
      // character, but our foreground color will still be used unless it's
      // been set to transparent. if that foreground color is transparent, we
      // still use a character we find here, but its color will come entirely
      // from cells underneath us.
      if(!crender->p){
        // if the following is true, we're a real glyph, and not the right-hand
        // side of a wide glyph (or the null codepoint).
        if( (targc->gcluster = vis->gcluster) ){ // index copy only
          // we can't plop down a wide glyph if the next cell is beyond the
          // screen, nor if we're bisected by a higher plane.
          if(cell_double_wide_p(vis)){
            // are we on the last column of the real screen? if so, 0x20 us
            if(absx >= dstlenx - 1){
              targc->gcluster = ' ';
            // is the next cell occupied? if so, 0x20 us
            }else if(targc[1].gcluster){
//fprintf(stderr, "NULLING out %d/%d (%d/%d) due to %u\n", y, x, absy, absx, targc[1].gcluster);
              targc->gcluster = ' ';
            }else{
              cell_set_wide(targc);
            }
          }
          crender->p = p;
          targc->attrword = vis->attrword;
        }else if(cell_wide_left_p(vis)){
          cell_set_wide(targc);
        }
      }

      // Background color takes effect independently of whether we have a
      // glyph. If we've already locked in the background, it has no effect.
      // If it's transparent, it has no effect. Otherwise, update the
      // background channel and balpha.
      if(cell_bg_palindex_p(vis)){
        if(cell_bg_alpha(targc) == CELL_ALPHA_TRANSPARENT){
          cell_set_bg_palindex(targc, cell_bg_palindex(vis));
        }
      }else if(cell_bg_alpha(targc) > CELL_ALPHA_OPAQUE){
        cell_blend_bchannel(targc, cell_bchannel(vis), &crender->bgblends);
      }
      // Evaluate the background first, in case this is HIGHCONTRAST fg text.
      if(cell_fg_palindex_p(vis)){
        if(cell_fg_alpha(targc) == CELL_ALPHA_TRANSPARENT){
          cell_set_fg_palindex(targc, cell_fg_palindex(vis));
        }
      }else if(cell_fg_alpha(targc) > CELL_ALPHA_OPAQUE){
        if(cell_fg_alpha(vis) == CELL_ALPHA_HIGHCONTRAST){
          crender->highcontrast = true;
        }
        cell_blend_fchannel(targc, cell_fchannel(vis), &crender->fgblends);
        // crender->highcontrast can only be true if we just set it, since we're
        // about to set targc opaque based on crender->highcontrast (and this
        // entire stanza is conditional on targc not being CELL_ALPHA_OPAQUE).
        if(crender->highcontrast){
          cell_set_fg_alpha(targc, CELL_ALPHA_OPAQUE);
        }
      }

      // have we locked this coordinate in as a result of this plane (cells
      // which were already locked in were skipped at the top of the loop)?
      if(cell_locked_p(targc)){
        lock_in_highcontrast(targc, crender);
        cell* prevcell = &lastframe[fbcellidx(absy, lfdimx, absx)];
/*if(cell_simple_p(targc)){
fprintf(stderr, "WROTE %u [%c] to %d/%d (%d/%d)\n", targc->gcluster, prevcell->gcluster, y, x, absy, absx);
}else{
fprintf(stderr, "WROTE %u [%s] to %d/%d (%d/%d)\n", targc->gcluster, extended_gcluster(crender->p, targc), y, x, absy, absx);
}*/
        if(cellcmp_and_dupfar(pool, prevcell, crender->p, targc)){
          crender->damaged = true;
          if(cell_wide_left_p(targc)){
            ncplane* tmpp = crender->p;
            ++crender;
            crender->p = tmpp;
            ++x;
            ++prevcell;
            ++targc;
            targc->gcluster = 0;
            targc->channels = targc[-1].channels;
            targc->attrword = targc[-1].attrword;
            if(cellcmp_and_dupfar(pool, prevcell, crender->p, targc)){
              crender->damaged = true;
            }
          }
        }
      }
    }
  }
  return 0;
}

static void
init_fb(cell* fb, int dimy, int dimx){
  for(int y = 0 ; y < dimy ; ++y){
    for(int x = 0 ; x < dimx ; ++x){
      cell* c = &fb[fbcellidx(y, dimx, x)];
      c->gcluster = 0;
      c->channels = 0;
      c->attrword = 0;
      cell_set_fg_alpha(c, CELL_ALPHA_TRANSPARENT);
      cell_set_bg_alpha(c, CELL_ALPHA_TRANSPARENT);
    }
  }
}

static void
postpaint(cell* fb, cell* lastframe, int dimy, int dimx,
          struct crender* rvec, egcpool* pool){
  for(int y = 0 ; y < dimy ; ++y){
    for(int x = 0 ; x < dimx ; ++x){
      cell* targc = &fb[fbcellidx(y, dimx, x)];
      if(!cell_locked_p(targc)){
        struct crender* crender = &rvec[fbcellidx(y, dimx, x)];
        lock_in_highcontrast(targc, crender);
        cell* prevcell = &lastframe[fbcellidx(y, dimx, x)];
        if(targc->gcluster == 0){
          targc->gcluster = ' ';
        }
        if(cellcmp_and_dupfar(pool, prevcell, crender->p, targc)){
          crender->damaged = true;
        }
      }
    }
  }
}

// FIXME need handle a dst that isn't the standard plane! paint() will only
// paint within the real viewport currently.
int ncplane_mergedown(ncplane* restrict src, ncplane* restrict dst){
  notcurses* nc = src->nc;
  if(dst == NULL){
    dst = nc->stdscr;
  }
  int dimy, dimx;
  ncplane_dim_yx(dst, &dimy, &dimx);
  cell* tmpfb = malloc(sizeof(*tmpfb) * dimy * dimx);
  cell* rendfb = malloc(sizeof(*rendfb) * dimy * dimx);
  const size_t crenderlen = sizeof(struct crender) * dimy * dimx;
  struct crender* rvec = malloc(crenderlen);
  memset(rvec, 0, crenderlen);
  init_fb(tmpfb, dimy, dimx);
  init_fb(rendfb, dimy, dimx);
  if(paint(src, rendfb, rvec, tmpfb, &dst->pool, dst->leny, dst->lenx,
           dst->absy, dst->absx, dst->lenx)){
    free(rvec);
    free(rendfb);
    free(tmpfb);
    return -1;
  }
  if(paint(dst, rendfb, rvec, tmpfb, &dst->pool, dst->leny, dst->lenx,
           dst->absy, dst->absx, dst->lenx)){
    free(rvec);
    free(rendfb);
    free(tmpfb);
    return -1;
  }
  postpaint(tmpfb, rendfb, dimy, dimx, rvec, &dst->pool);
  free(dst->fb);
  dst->fb = rendfb;
  free(tmpfb);
  free(rvec);
  return 0;
}

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

// write the cell's UTF-8 extended grapheme cluster to the provided FILE*.
static int
term_putc(FILE* out, const egcpool* e, const cell* c){
  if(cell_simple_p(c)){
    if(c->gcluster == 0 || iscntrl(c->gcluster)){
// fprintf(stderr, "[ ]\n");
      if(ncfputc(' ', out) == EOF){
        return -1;
      }
    }else{
// fprintf(stderr, "[%c]\n", c->gcluster);
      if(ncfputc(c->gcluster, out) == EOF){
        return -1;
      }
    }
  }else{
    const char* ext = egcpool_extended_gcluster(e, c);
// fprintf(stderr, "[%s]\n", ext);
    if(ncfputs(ext, out) == EOF){
      return -1;
    }
  }
  return 0;
}

// check the current and target style bitmasks against the specified 'stylebit'.
// if they are different, and we have the necessary capability, write the
// applicable terminfo entry to 'out'. returns -1 only on a true error.
static int
term_setstyle(FILE* out, unsigned cur, unsigned targ, unsigned stylebit,
              const char* ton, const char* toff){
  int ret = 0;
  unsigned curon = cur & stylebit;
  unsigned targon = targ & stylebit;
  if(curon != targon){
    if(targon){
      if(ton){
        ret = term_emit("ton", ton, out, false);
      }
    }else{
      if(toff){ // how did this happen? we can turn it on, but not off?
        ret = term_emit("toff", toff, out, false);
      }
    }
  }
  if(ret < 0){
    return -1;
  }
  return 0;
}

// write any escape sequences necessary to set the desired style
static inline int
term_setstyles(FILE* out, uint32_t* curattr, const cell* c, bool* normalized,
               const char* sgr0, const char* sgr, const char* italics,
               const char* italoff){
  *normalized = false;
  uint32_t cellattr = cell_styles(c);
  if(cellattr == *curattr){
    return 0; // happy agreement, change nothing
  }
  int ret = 0;
  // if only italics changed, don't emit any sgr escapes. xor of current and
  // target ought have all 0s in the lower 8 bits if only italics changed.
  if((cellattr ^ *curattr) & 0x00ff0000ul){
    *normalized = true; // FIXME this is pretty conservative
    // if everything's 0, emit the shorter sgr0
    if(sgr0 && ((cellattr & NCSTYLE_MASK) == 0)){
      if(term_emit("sgr0", sgr0, out, false) < 0){
        ret = -1;
      }
    }else if(term_emit("sgr", tiparm(sgr, cellattr & NCSTYLE_STANDOUT,
                                     cellattr & NCSTYLE_UNDERLINE,
                                     cellattr & NCSTYLE_REVERSE,
                                     cellattr & NCSTYLE_BLINK,
                                     cellattr & NCSTYLE_DIM,
                                     cellattr & NCSTYLE_BOLD,
                                     cellattr & NCSTYLE_INVIS,
                                     cellattr & NCSTYLE_PROTECT, 0),
                                     out, false) < 0){
      ret = -1;
    }
  }
  // sgr will blow away italics if they were set beforehand
  ret |= term_setstyle(out, *curattr, cellattr, NCSTYLE_ITALIC, italics, italoff);
  *curattr = cellattr;
  return ret;
}

// u8->str lookup table used in term_esc_rgb below
static const char* NUMBERS[] = {
"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16",
"17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30", "31", "32",
"33", "34", "35", "36", "37", "38", "39", "40", "41", "42", "43", "44", "45", "46", "47", "48",
"49", "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "60", "61", "62", "63", "64",
"65", "66", "67", "68", "69", "70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "80",
"81", "82", "83", "84", "85", "86", "87", "88", "89", "90", "91", "92", "93", "94", "95", "96",
"97", "98", "99", "100", "101", "102", "103", "104", "105", "106", "107", "108", "109", "110", "111", "112",
"113", "114", "115", "116", "117", "118", "119", "120", "121", "122", "123", "124", "125", "126", "127", "128",
"129", "130", "131", "132", "133", "134", "135", "136", "137", "138", "139", "140", "141", "142", "143", "144",
"145", "146", "147", "148", "149", "150", "151", "152", "153", "154", "155", "156", "157", "158", "159", "160",
"161", "162", "163", "164", "165", "166", "167", "168", "169", "170", "171", "172", "173", "174", "175", "176",
"177", "178", "179", "180", "181", "182", "183", "184", "185", "186", "187", "188", "189", "190", "191", "192",
"193", "194", "195", "196", "197", "198", "199", "200", "201", "202", "203", "204", "205", "206", "207", "208",
"209", "210", "211", "212", "213", "214", "215", "216", "217", "218", "219", "220", "221", "222", "223", "224",
"225", "226", "227", "228", "229", "230", "231", "232", "233", "234", "235", "236", "237", "238", "239", "240",
"241", "242", "243", "244", "245", "246", "247", "248", "249", "250", "251", "252", "253", "254", "255", };

static inline int
term_esc_rgb(FILE* out, bool foreground, unsigned r, unsigned g, unsigned b){
  // The correct way to do this is using tiparm+tputs, but doing so (at least
  // as of terminfo 6.1.20191019) both emits ~3% more bytes for a run of 'rgb'
  // and gives rise to some corrupted cells (possibly due to special handling of
  // values < 256; I'm not at this time sure). So we just cons up our own.
  /*if(esc == 4){
    return term_emit("setab", tiparm(nc->setab, (int)((r << 16u) | (g << 8u) | b)), out, false);
  }else if(esc == 3){
    return term_emit("setaf", tiparm(nc->setaf, (int)((r << 16u) | (g << 8u) | b)), out, false);
  }else{
    return -1;
  }*/
  #define RGBESC1 "\x1b" "["
  // we'd like to use the proper ITU T.416 colon syntax i.e. "8:2::", but it is
  // not supported by several terminal emulators :/.
  #define RGBESC2 "8;2;"
  // fprintf() was sitting atop our profiles, so we put the effort into a fast solution
  // here. assemble a buffer using constants and a lookup table. we can use 20
  // bytes in the worst case.
  char rgbbuf[20] = RGBESC1 " " RGBESC2;
  if(foreground){
    rgbbuf[2] = '3';
  }else{
    rgbbuf[2] = '4';
  }
  size_t offset = 7;
  const char* s = NUMBERS[r];
  while( (rgbbuf[offset] = *s) ){
    ++offset;
    ++s;
  }
  rgbbuf[offset++] = ';';
  s = NUMBERS[g];
  while( (rgbbuf[offset] = *s) ){
    ++offset;
    ++s;
  }
  rgbbuf[offset++] = ';';
  s = NUMBERS[b];
  while( (rgbbuf[offset] = *s) ){
    ++offset;
    ++s;
  }
  rgbbuf[offset++] = 'm';
  rgbbuf[offset] = '\0';
  if(ncfputs(rgbbuf, out) == EOF){
    return -1;
  }
  return 0;
}

static inline int
term_bg_rgb8(bool RGBflag, const char* setab, int colors, FILE* out,
             unsigned r, unsigned g, unsigned b){
  // We typically want to use tputs() and tiperm() to acquire and write the
  // escapes, as these take into account terminal-specific delays, padding,
  // etc. For the case of DirectColor, there is no suitable terminfo entry, but
  // we're also in that case working with hopefully more robust terminals.
  // If it doesn't work, eh, it doesn't work. Fuck the world; save yourself.
  if(RGBflag){
    return term_esc_rgb(out, false, r, g, b);
  }else{
    if(setab == NULL){
      return -1;
    }
    // For 256-color indexed mode, start constructing a palette based off
    // the inputs *if we can change the palette*. If more than 256 are used on
    // a single screen, start... combining close ones? For 8-color mode, simple
    // interpolation. I have no idea what to do for 88 colors. FIXME
    if(colors >= 256){
      return term_emit("setab", tiparm(setab, rgb_quantize_256(r, g, b)), out, false);
    }else if(colors >= 8){
      return term_emit("setab", tiparm(setab, rgb_quantize_8(r, g, b)), out, false);
    }
  }
  return 0;
}

static inline int
term_fg_rgb8(bool RGBflag, const char* setaf, int colors, FILE* out,
             unsigned r, unsigned g, unsigned b){
  // We typically want to use tputs() and tiperm() to acquire and write the
  // escapes, as these take into account terminal-specific delays, padding,
  // etc. For the case of DirectColor, there is no suitable terminfo entry, but
  // we're also in that case working with hopefully more robust terminals.
  // If it doesn't work, eh, it doesn't work. Fuck the world; save yourself.
  if(RGBflag){
    return term_esc_rgb(out, true, r, g, b);
  }else{
    if(setaf == NULL){
      return -1;
    }
    // For 256-color indexed mode, start constructing a palette based off
    // the inputs *if we can change the palette*. If more than 256 are used on
    // a single screen, start... combining close ones? For 8-color mode, simple
    // interpolation. I have no idea what to do for 88 colors. FIXME
    if(colors >= 256){
      return term_emit("setaf", tiparm(setaf, rgb_quantize_256(r, g, b)), out, false);
    }else if(colors >= 8){
      return term_emit("setaf", tiparm(setaf, rgb_quantize_8(r, g, b)), out, false);
    }
  }
  return 0;
}

static inline int
ncdirect_style_emit(ncdirect* n, const char* sgr, unsigned stylebits, FILE* out){
  if(sgr == NULL){
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
  n->attrword |= stylebits;
  if(ncdirect_style_emit(n, n->sgr, n->attrword, n->ttyfp)){
    return 0;
  }
  return term_setstyle(n->ttyfp, n->attrword, stylebits, NCSTYLE_ITALIC, n->italics, n->italoff);
}

// turn off any specified stylebits
int ncdirect_styles_off(ncdirect* n, unsigned stylebits){
  n->attrword &= ~stylebits;
  if(ncdirect_style_emit(n, n->sgr, n->attrword, n->ttyfp)){
    return 0;
  }
  return term_setstyle(n->ttyfp, n->attrword, stylebits, NCSTYLE_ITALIC, n->italics, n->italoff);
}

// set the current stylebits to exactly those provided
int ncdirect_styles_set(ncdirect* n, unsigned stylebits){
  n->attrword = stylebits;
  if(ncdirect_style_emit(n, n->sgr, n->attrword, n->ttyfp)){
    return 0;
  }
  return term_setstyle(n->ttyfp, n->attrword, stylebits, NCSTYLE_ITALIC, n->italics, n->italoff);
}

int ncdirect_fg_default(ncdirect* nc){
  if(term_emit("op", nc->op, nc->ttyfp, false) == 0){
    nc->fgdefault = true;
    if(nc->bgdefault){
      return 0;
    }
    return ncdirect_bg(nc, nc->fgrgb);
  }
  return -1;
}

int ncdirect_bg_default(ncdirect* nc){
  if(term_emit("op", nc->op, nc->ttyfp, false) == 0){
    nc->bgdefault = true;
    if(nc->fgdefault){
      return 0;
    }
    return ncdirect_fg(nc, nc->bgrgb);
  }
  return -1;
}

int ncdirect_bg(ncdirect* nc, unsigned rgb){
  if(rgb > 0xffffffu){
    return -1;
  }
  if(term_bg_rgb8(nc->RGBflag, nc->setab, nc->colors, nc->ttyfp,
                  (rgb & 0xff0000u) >> 16u, (rgb & 0xff00u) >> 8u, rgb & 0xffu)){
    return -1;
  }
  nc->bgdefault = false;
  nc->bgrgb = rgb;
  return 0;
}

int ncdirect_fg(ncdirect* nc, unsigned rgb){
  if(rgb > 0xffffffu){
    return -1;
  }
  if(term_fg_rgb8(nc->RGBflag, nc->setaf, nc->colors, nc->ttyfp,
                  (rgb & 0xff0000u) >> 16u, (rgb & 0xff00u) >> 8u, rgb & 0xffu)){
    return -1;
  }
  nc->fgdefault = false;
  nc->fgrgb = rgb;
  return 0;
}

static inline int
update_palette(notcurses* nc, FILE* out){
  if(nc->CCCflag){
    for(size_t damageidx = 0 ; damageidx < sizeof(nc->palette.chans) / sizeof(*nc->palette.chans) ; ++damageidx){
      unsigned r, g, b;
      if(nc->palette_damage[damageidx]){
        channel_rgb(nc->palette.chans[damageidx], &r, &g, &b);
        // Need convert RGB values [0..256) to [0..1000], ugh
        // FIXME need handle HSL case also
        r = r * 1000 / 255;
        g = g * 1000 / 255;
        b = b * 1000 / 255;
        term_emit("initc", tiparm(nc->initc, damageidx, r, g, b), out, false);
        nc->palette_damage[damageidx] = false;
      }
    }
  }
  return 0;
}

// sync the cursor to the specified location with as little overhead as
// possible (with nothing, if already at the right location).
// FIXME fall back to synthesized moves in the absence of capabilities (i.e.
// textronix lacks cup; fake it with horiz+vert moves)
static inline int
stage_cursor(notcurses* nc, FILE* out, int y, int x){
  int ret = 0;
  if(nc->rstate.y == y){ // only need move x
    const int xdiff = x - nc->rstate.x;
    if(xdiff > 0){
      if(xdiff == 1){
        ret = term_emit("cuf1", tiparm(nc->cuf1), out, false);
      }else{
        ret = term_emit("cuf", tiparm(nc->cuf, xdiff), out, false);
      }
      nc->rstate.x = x;
      return ret;
    }else if(xdiff == 0){
      return 0; // no move needed
    }
    // cub1/cub tend to be destructive in my experiments :/
  }
  ret = term_emit("cup", tiparm(nc->cup, y, x), out, false);
  if(ret == 0){
    nc->rstate.x = x;
    nc->rstate.y = y;
  }
  return ret;
}

// Producing the frame requires three steps:
//  * render -- build up a flat framebuffer from a set of ncplanes
//  * rasterize -- build up a UTF-8/ASCII stream of escapes and EGCs
//  * refresh -- write the stream to the emulator

// Takes a rendered frame (a flat framebuffer, where each cell has the desired
// EGC, attribute, and channels), which has been written to nc->lastframe, and
// spits out an optimal sequence of terminal-appropriate escapes and EGCs. There
// should be an rvec entry for each cell, but only the 'damaged' field is used.
// lastframe has *not yet been written to the screen*, i.e. it's only about to
// *become* the last frame rasterized.
static int
notcurses_rasterize(notcurses* nc, const struct crender* rvec){
  FILE* out = nc->rstate.mstreamfp;
  int ret = 0;
  int y, x;
  fseeko(out, 0, SEEK_SET);
  // we only need to emit a coordinate if it was damaged. the damagemap is a
  // bit per coordinate, rows by rows, column by column within a row, with the
  // MSB being the first coordinate.
  // don't write a clearscreen. we only update things that have been changed.
  // we explicitly move the cursor at the beginning of each output line, so no
  // need to home it expliticly.
  update_palette(nc, out);
  for(y = nc->stdscr->absy ; y < nc->stdscr->leny + nc->stdscr->absy ; ++y){
    const int innery = y - nc->stdscr->absy;
    for(x = nc->stdscr->absx ; x < nc->stdscr->lenx + nc->stdscr->absx ; ++x){
      const int innerx = x - nc->stdscr->absx;
      const size_t damageidx = innery * nc->lfdimx + innerx;
      unsigned r, g, b, br, bg, bb, palfg, palbg;
      const cell* srccell = &nc->lastframe[damageidx];
//      cell c;
//      memcpy(c, srccell, sizeof(*c)); // unsafe copy of gcluster
//fprintf(stderr, "COPYING: %d from %p\n", c->gcluster, &nc->pool);
//      const char* egc = pool_egc_copy(&nc->pool, srccell);
//      c->gcluster = 0; // otherwise cell_release() will blow up
      if(!rvec[damageidx].damaged){
        // no need to emit a cell; what we rendered appears to already be
        // here. no updates are performed to elision state nor lastframe.
        ++nc->stats.cellelisions;
        if(cell_wide_left_p(srccell)){
          ++x;
        }
      }else{
        ++nc->stats.cellemissions;
        ret |= stage_cursor(nc, out, y, x);
        // set the style. this can change the color back to the default; if it
        // does, we need update our elision possibilities.
        bool normalized;
        ret |= term_setstyles(out, &nc->rstate.curattr, srccell, &normalized,
                              nc->sgr0, nc->sgr, nc->italics, nc->italoff);
        if(normalized){
          nc->rstate.defaultelidable = true;
          nc->rstate.bgelidable = false;
          nc->rstate.fgelidable = false;
          nc->rstate.bgpalelidable = false;
          nc->rstate.fgpalelidable = false;
        }
        // we allow these to be set distinctly, but terminfo only supports using
        // them both via the 'op' capability. unless we want to generate the 'op'
        // escapes ourselves, if either is set to default, we first send op, and
        // then a turnon for whichever aren't default.

        // if our cell has a default foreground *or* background, we can elide the
        // default set iff one of:
        //  * we are a partial glyph, and the previous was default on both, or
        //  * we are a no-foreground glyph, and the previous was default background, or
        //  * we are a no-background glyph, and the previous was default foreground
        bool noforeground = cell_noforeground_p(srccell);
        bool nobackground = cell_nobackground_p(&nc->pool, srccell);
        if((!noforeground && cell_fg_default_p(srccell)) || (!nobackground && cell_bg_default_p(srccell))){
          if(!nc->rstate.defaultelidable){
            ++nc->stats.defaultemissions;
            ret |= term_emit("op", nc->op, out, false);
          }else{
            ++nc->stats.defaultelisions;
          }
          // if either is not default, this will get turned off
          nc->rstate.defaultelidable = true;
          nc->rstate.fgelidable = false;
          nc->rstate.bgelidable = false;
          nc->rstate.fgpalelidable = false;
          nc->rstate.bgpalelidable = false;
        }
        // if our cell has a non-default foreground, we can elide the non-default
        // foreground set iff either:
        //  * the previous was non-default, and matches what we have now, or
        //  * we are a no-foreground glyph (iswspace() is true)
        if(noforeground){
          ++nc->stats.fgelisions;
        }else if(cell_fg_palindex_p(srccell)){ // palette-indexed foreground
          palfg = cell_fg_palindex(srccell);
          // we overload lastr for the palette index; both are 8 bits
          if(nc->rstate.fgpalelidable && nc->rstate.lastr == palfg){
            ++nc->stats.fgelisions;
          }else{
            ret |= term_fg_palindex(nc, out, palfg);
            ++nc->stats.fgemissions;
            nc->rstate.fgpalelidable = true;
          }
          nc->rstate.lastr = palfg;
          nc->rstate.defaultelidable = false;
          nc->rstate.fgelidable = false;
        }else if(!cell_fg_default_p(srccell)){ // rgb foreground
          cell_fg_rgb(srccell, &r, &g, &b);
          if(nc->rstate.fgelidable && nc->rstate.lastr == r && nc->rstate.lastg == g && nc->rstate.lastb == b){
            ++nc->stats.fgelisions;
          }else{
            ret |= term_fg_rgb8(nc->RGBflag, nc->setaf, nc->colors, out, r, g, b);
            ++nc->stats.fgemissions;
            nc->rstate.fgelidable = true;
          }
          nc->rstate.lastr = r; nc->rstate.lastg = g; nc->rstate.lastb = b;
          nc->rstate.defaultelidable = false;
          nc->rstate.fgpalelidable = false;
        }
        if(nobackground){
          ++nc->stats.bgelisions;
        }else if(cell_bg_palindex_p(srccell)){ // palette-indexed background
          palbg = cell_bg_palindex(srccell);
          if(nc->rstate.bgpalelidable && nc->rstate.lastbr == palbg){
            ++nc->stats.bgelisions;
          }else{
            ret |= term_bg_palindex(nc, out, palbg);
            ++nc->stats.bgemissions;
            nc->rstate.bgpalelidable = true;
          }
          nc->rstate.lastr = palbg;
          nc->rstate.defaultelidable = false;
          nc->rstate.bgelidable = false;
        }else if(!cell_bg_default_p(srccell)){ // rgb background
          if(!nobackground){
            cell_bg_rgb(srccell, &br, &bg, &bb);
            if(nc->rstate.bgelidable && nc->rstate.lastbr == br && nc->rstate.lastbg == bg && nc->rstate.lastbb == bb){
              ++nc->stats.bgelisions;
            }else{
              ret |= term_bg_rgb8(nc->RGBflag, nc->setab, nc->colors, out, br, bg, bb);
              ++nc->stats.bgemissions;
              nc->rstate.bgelidable = true;
            }
            nc->rstate.lastbr = br; nc->rstate.lastbg = bg; nc->rstate.lastbb = bb;
            nc->rstate.defaultelidable = false;
            nc->rstate.bgpalelidable = false;
          }
        }
/*if(cell_simple_p(srccell)){
fprintf(stderr, "RAST %u [%c] to %d/%d\n", srccell->gcluster, srccell->gcluster, y, x);
}else{
fprintf(stderr, "RAST %u [%s] to %d/%d\n", srccell->gcluster, egcpool_extended_gcluster(&nc->pool, srccell), y, x);
}*/
        if(term_putc(out, &nc->pool, srccell) == 0){
          ++nc->rstate.x;
          if(cell_wide_left_p(srccell)){
            ++nc->rstate.x;
            ++x;
          }
          // if the terminal's own motion carried us down to the next line,
          // we need update our concept of the cursor's true y
          /*if(nc->rstate.x >= nc->truecols){
            ++nc->rstate.y; // FIXME not if on last line, right?
            nc->rstate.x = 0;
          }*/
        }else{
          ret = -1;
        }
      }
//fprintf(stderr, "damageidx: %ld\n", damageidx);
    }
  }
  ret |= fflush(out);
  //fflush(nc->ttyfp);
  if(blocking_write(nc->ttyfd, nc->rstate.mstream, nc->rstate.mstrsize)){
    ret = -1;
  }
//fprintf(stderr, "%lu/%lu %lu/%lu %lu/%lu %d\n", nc->stats.defaultelisions, nc->stats.defaultemissions, nc->stats.fgelisions, nc->stats.fgemissions, nc->stats.bgelisions, nc->stats.bgemissions, ret);
  if(nc->renderfp){
    fprintf(nc->renderfp, "%s\n", nc->rstate.mstream);
  }
  if(ret < 0){
    return ret;
  }
  return nc->rstate.mstrsize;
}

// get the cursor to the upper-left corner by one means or another. will clear
// the screen if need be.
static int
home_cursor(notcurses* nc, bool flush){
  int ret = -1;
  if(nc->home){
    ret = term_emit("home", nc->home, nc->ttyfp, flush);
  }else if(nc->cup){
    ret = term_emit("cup", tiparm(nc->cup, 1, 1), nc->ttyfp, flush);
  }else if(nc->clearscr){
    ret = term_emit("clear", nc->clearscr, nc->ttyfp, flush);
  }
  if(ret >= 0){
    nc->rstate.x = 0;
    nc->rstate.y = 0;
  }
  return ret;
}

int notcurses_refresh(notcurses* nc, int* restrict dimy, int* restrict dimx){
  if(notcurses_resize(nc, dimy, dimx)){
    return -1;
  }
  if(nc->lfdimx == 0 || nc->lfdimy == 0){
    return 0;
  }
  if(home_cursor(nc, true)){
    return -1;
  }
  const int count = (nc->lfdimx > nc->stdscr->lenx ? nc->lfdimx : nc->stdscr->lenx) *
                    (nc->lfdimy > nc->stdscr->leny ? nc->lfdimy : nc->stdscr->leny);
  struct crender* rvec = malloc(count * sizeof(*rvec));
  if(rvec == NULL){
    return -1;
  }
  memset(rvec, 0, count * sizeof(*rvec));
  for(int i = 0 ; i < count ; ++i){
    rvec[i].damaged = true;
  }
  int ret = notcurses_rasterize(nc, rvec);
  free(rvec);
  if(ret < 0){
    return -1;
  }
  return 0;
}

// We execute the painter's algorithm, starting from our topmost plane. The
// damagevector should be all zeros on input. On success, it will reflect
// which cells were changed. We solve for each coordinate's cell by walking
// down the z-buffer, looking at intersections with ncplanes. This implies
// locking down the EGC, the attributes, and the channels for each cell.
static int
notcurses_render_internal(notcurses* nc, struct crender* rvec){
  int dimy, dimx;
  ncplane_dim_yx(nc->stdscr, &dimy, &dimx);
  cell* fb = malloc(sizeof(*fb) * dimy * dimx);
  init_fb(fb, dimy, dimx);
  ncplane* p = nc->top;
  while(p){
    if(paint(p, nc->lastframe, rvec, fb, &nc->pool,
             nc->stdscr->leny, nc->stdscr->lenx,
             nc->stdscr->absy, nc->stdscr->absx, nc->lfdimx)){
      free(fb);
      return -1;
    }
    p = p->z;
  }
  postpaint(fb, nc->lastframe, dimy, dimx, rvec, &nc->pool);
  free(fb);
  return 0;
}

int notcurses_render(notcurses* nc){
  struct timespec start, done;
  int ret;
  clock_gettime(CLOCK_MONOTONIC, &start);
  int dimy, dimx;
  notcurses_resize(nc, &dimy, &dimx);
  int bytes = -1;
  const size_t crenderlen = sizeof(struct crender) * nc->stdscr->leny * nc->stdscr->lenx;
  struct crender* crender = malloc(crenderlen);
  memset(crender, 0, crenderlen);
  if(notcurses_render_internal(nc, crender) == 0){
    bytes = notcurses_rasterize(nc, crender);
  }
  free(crender);
  clock_gettime(CLOCK_MONOTONIC, &done);
  update_render_stats(&done, &start, &nc->stats, bytes);
  ret = bytes >= 0 ? 0 : -1;
  return ret;
}

char* notcurses_at_yx(notcurses* nc, int yoff, int xoff, uint32_t* attrword, uint64_t* channels){
  char* egc = NULL;
  if(nc->lastframe){
    if(yoff >= 0 && yoff < nc->lfdimy){
      if(xoff >= 0 || xoff < nc->lfdimx){
        const cell* srccell = &nc->lastframe[yoff * nc->lfdimx + xoff];
        *attrword = srccell->attrword;
        *channels = srccell->channels;
//fprintf(stderr, "COPYING: %d from %p\n", srccell->gcluster, &nc->pool);
        egc = pool_egc_copy(&nc->pool, srccell);
      }
    }
  }
  return egc;
}
