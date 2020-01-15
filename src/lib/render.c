#include <ctype.h>
#include <limits.h>
#include <unistd.h>
#include <sys/poll.h>
#include "internal.h"

static void
mutex_unlock(void* vlock){
  pthread_mutex_unlock(vlock);
}

static int
blocking_write(int fd, const char* buf, size_t buflen){
//fprintf(stderr, "writing %zu to %d...\n", buflen, fd);
  size_t written = 0;
  do{
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
  }while(written < buflen);
  return 0;
}

int notcurses_refresh(notcurses* nc){
  int ret;
  pthread_mutex_lock(&nc->lock);
  pthread_cleanup_push(mutex_unlock, &nc->lock);
  if(nc->rstate.mstream == NULL){
    ret = -1; // haven't rendered yet, and thus don't know what should be there
  }else if(blocking_write(nc->ttyfd, nc->rstate.mstream, nc->rstate.mstrsize)){
    ret = -1;
  }else{
    ret = 0;
  }
  pthread_cleanup_pop(1);
  return ret;
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

// reshape the shadow framebuffer to match the stdplane's dimensions, throwing
// away the old one.
static int
reshape_shadow_fb(notcurses* nc){
  if(nc->lfdimx == nc->stdscr->lenx && nc->lfdimy == nc->stdscr->leny){
    return 0; // no change
  }
  const size_t size = sizeof(*nc->lastframe) * nc->stdscr->leny * nc->stdscr->lenx;
  cell* fb = realloc(nc->lastframe, size);
  if(fb == NULL){
    free(nc->lastframe);
    nc->lastframe = NULL;
    nc->lfdimx = 0;
    nc->lfdimy = 0;
    return -1;
  }
  nc->lastframe = fb;
  // FIXME more memset()tery than we need, both wasting work and wrecking
  // damage detection for the upcoming render
  memset(nc->lastframe, 0, size);
  nc->lastframe = fb;
  nc->lfdimy = nc->stdscr->leny;
  nc->lfdimx = nc->stdscr->lenx;
  memset(fb, 0, size);
  egcpool_dump(&nc->pool);
  return 0;
}

static inline void
pool_release(egcpool* pool, cell* c){
  if(!cell_simple_p(c)){
    egcpool_release(pool, cell_egc_idx(c));
    c->gcluster = 0; // don't subject ourselves to double-release problems
  }
}

void cell_release(ncplane* n, cell* c){
  pool_release(&n->pool, c);
}

// Duplicate one cell onto another, possibly crossing ncplanes.
static inline int
cell_duplicate_far(egcpool* tpool, cell* targ, const ncplane* splane, const cell* c){
  pool_release(tpool, targ);
  targ->attrword = c->attrword;
  targ->channels = c->channels;
  if(cell_simple_p(c)){
    targ->gcluster = c->gcluster;
    return !!c->gcluster;
  }
  size_t ulen = strlen(extended_gcluster(splane, c));
//fprintf(stderr, "[%s] (%zu)\n", egcpool_extended_gcluster(&splane->pool, c), strlen(egcpool_extended_gcluster(&splane->pool, c)));
  int eoffset = egcpool_stash(tpool, extended_gcluster(splane, c), ulen);
  if(eoffset < 0){
    return -1;
  }
  targ->gcluster = eoffset + 0x80;
  return ulen;
}

// Duplicate one cell onto another when they share a plane. Convenience wrapper.
int cell_duplicate(ncplane* n, cell* targ, const cell* c){
  return cell_duplicate_far(&n->pool, targ, n, c);
}

// the heart of damage detection. compare two cells (from two different planes)
// for equality. if they are equal, return 0. otherwise, dup the second onto
// the first and return non-zero.
static int
cellcmp_and_dupfar(egcpool* dampool, cell* damcell, const ncplane* srcplane,
                   const cell* srccell){
  if(damcell->attrword == srccell->attrword){
    if(damcell->channels == srccell->channels){
      bool damsimple = cell_simple_p(damcell);
      bool srcsimple = cell_simple_p(srccell);
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

// Extracellular state for a cell during the render process
struct crender {
  int fgblends;
  int bgblends;
  ncplane *p;
  bool damaged;
};

// Paints a single ncplane into the provided framebuffer 'fb'. Whenever a cell
// is locked in, it is compared against the last frame. If it is different, the
// 'damagevec' bitmap is updated with a 1.
static int
paint(notcurses* nc, ncplane* p, struct crender* rvec, cell* fb){
  int y, x, dimy, dimx, offy, offx;
  // don't use ncplane_dim_yx()/ncplane_yx() here, lest we deadlock
  dimy = p->leny;
  dimx = p->lenx;
  offy = p->absy;
  offx = p->absx;
//fprintf(stderr, "PLANE %p %d %d %d %d %d %d\n", p, dimy, dimx, offy, offx, nc->stdscr->leny, nc->stdscr->lenx);
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
    if(absy >= nc->stdscr->leny){
      break;
    }
    for(x = startx ; x < dimx ; ++x){
      const int absx = x + offx;
      if(absx >= nc->stdscr->lenx){
        break;
      }
      cell* targc = &fb[fbcellidx(absy, nc->stdscr->lenx, absx)];
      if(cell_locked_p(targc)){
        continue;
      }
      struct crender* crender = &rvec[fbcellidx(absy, nc->stdscr->lenx, absx)];
      const cell* vis = &p->fb[nfbcellidx(p, y, x)];
      // if we never loaded any content into the cell (or obliterated it by
      // writing in a zero), use the plane's default cell.
      if(vis->gcluster == 0){
        vis = &p->basecell;
      }
      // if we have no character in this cell, we continue to look for a
      // character, but our foreground color will still be used unless it's
      // been set to transparent. if that foreground color is transparent, we
      // still use a character we find here, but its color will come entirely
      // from cells underneath us.
      if(!crender->p){
        // if the following is true, we're a real glyph, and not the right-h
        // hand side of a wide glyph (or the null codepoint).
        if( (targc->gcluster = vis->gcluster) ){ // index copy only
          // we can't plop down a wide glyph if the next cell is beyond the
          // screen, nor if we're bisected by a higher plane.
          if(cell_double_wide_p(vis)){
            // are we on the last column of the real screen? if so, 0x20 us
            if(absx >= nc->stdscr->lenx - 1){
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
        }else if(cell_double_wide_p(vis)){
          cell_set_wide(targc);
        }
      }
      if(cell_fg_alpha(targc) > CELL_ALPHA_OPAQUE && cell_fg_alpha(vis) < CELL_ALPHA_TRANSPARENT){
        cell_blend_fchannel(targc, cell_fchannel(vis), crender->fgblends);
        ++crender->fgblends;
      }
      // Background color takes effect independently of whether we have a
      // glyph. If we've already locked in the background, it has no effect.
      // If it's transparent, it has no effect. Otherwise, update the
      // background channel and balpha.
      if(cell_bg_alpha(targc) > CELL_ALPHA_OPAQUE && cell_bg_alpha(vis) < CELL_ALPHA_TRANSPARENT){
        cell_blend_bchannel(targc, cell_bchannel(vis), crender->bgblends);
        ++crender->bgblends;
      }

      if(cell_locked_p(targc)){
        cell* prevcell = &nc->lastframe[fbcellidx(absy, nc->lfdimx, absx)];
        if(cellcmp_and_dupfar(&nc->pool, prevcell, crender->p, targc)){
/*if(cell_simple_p(prevcell)){
fprintf(stderr, "WROTE %u [%c] to %d/%d (%d/%d)\n", prevcell->gcluster, prevcell->gcluster, y, x, absy, absx);
}else{
fprintf(stderr, "WROTE %u [%s] to %d/%d (%d/%d)\n", prevcell->gcluster, egcpool_extended_gcluster(&nc->pool, prevcell), y, x, absy, absx);
}*/
          crender->damaged = true;
          if(cell_double_wide_p(targc)){
            ncplane* tmpp = crender->p;
            ++crender;
            crender->p = tmpp;
            ++x;
            ++prevcell;
            ++targc;
            targc->gcluster = 0;
            targc->channels = targc[-1].channels;
            targc->attrword = targc[-1].attrword;
            if(cellcmp_and_dupfar(&nc->pool, prevcell, crender->p, targc)){
              crender->damaged = true;
            }
          }
        }
      }
    }
  }
  return 0;
}

// We execute the painter's algorithm, starting from our topmost plane. The
// damagevector should be all zeros on input. On success, it will reflect
// which cells were changed. We solve for each coordinate's cell by walking
// down the z-buffer, looking at intersections with ncplanes. This implies
// locking down the EGC, the attributes, and the channels for each cell.
static inline int
notcurses_render_internal(notcurses* nc, struct crender* rvec){
  if(reshape_shadow_fb(nc)){
    return -1;
  }
  // don't use ncplane_dim_yx()/ncplane_yx() here, lest we deadlock
  int dimy = nc->stdscr->leny;
  int dimx = nc->stdscr->lenx;
  cell* fb = malloc(sizeof(*fb) * dimy * dimx);
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
  ncplane* p = nc->top;
  while(p){
    if(paint(nc, p, rvec, fb)){
      return -1;
    }
    p = p->z;
  }
  for(int y = 0 ; y < dimy ; ++y){
    for(int x = 0 ; x < dimx ; ++x){
      cell* targc = &fb[fbcellidx(y, dimx, x)];
      if(!cell_locked_p(targc)){
        cell* prevcell = &nc->lastframe[fbcellidx(y, dimx, x)];
        if(targc->gcluster == 0){
          targc->gcluster = ' ';
        }
        if(cellcmp_and_dupfar(&nc->pool, prevcell, rvec->p, targc)){
          struct crender* crender = &rvec[fbcellidx(y, dimx, x)];
          crender->damaged = true;
        }
      }
    }
  }
  free(fb);
  return 0;
}

static inline int
ncfputs(const char* ext, FILE* out){
#ifdef __USE_GNU
    return fputs_unlocked(ext, out);
#else
    return fputs(ext, out);
#endif
}

static inline int
ncfputc(char c, FILE* out){
#ifdef __USE_GNU
      return fputc_unlocked(c, out);
#else
      return fputc(c, out);
#endif
}

// write the cell's UTF-8 grapheme cluster to the provided FILE*. returns the
// number of columns occupied by this EGC (only an approximation; it's actually
// a property of the font being used).
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
    if(ncfputs(ext, out) < 0){
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
static int
term_setstyles(const notcurses* nc, FILE* out, uint32_t* curattr, const cell* c,
               bool* normalized){
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
    if(nc->sgr0 && ((cellattr & CELL_STYLE_MASK) == 0)){
      if(term_emit("sgr0", nc->sgr0, out, false) < 0){
        ret = -1;
      }
    }else if(term_emit("sgr", tiparm(nc->sgr, cellattr & CELL_STYLE_STANDOUT,
                                        cellattr & CELL_STYLE_UNDERLINE,
                                        cellattr & CELL_STYLE_REVERSE,
                                        cellattr & CELL_STYLE_BLINK,
                                        cellattr & CELL_STYLE_DIM,
                                        cellattr & CELL_STYLE_BOLD,
                                        cellattr & CELL_STYLE_INVIS,
                                        cellattr & CELL_STYLE_PROTECT, 0),
                                        out, false) < 0){
      ret = -1;
    }
  }
  // sgr will blow away italics if they were set beforehand
  ret |= term_setstyle(out, *curattr, cellattr, CELL_STYLE_ITALIC, nc->italics, nc->italoff);
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
term_esc_rgb(notcurses* nc __attribute__ ((unused)), FILE* out, bool foreground,
             unsigned r, unsigned g, unsigned b){
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
  if(ncfputs(rgbbuf, out) < 0){
    return -1;
  }
  return 0;
}

static int
term_bg_rgb8(notcurses* nc, FILE* out, unsigned r, unsigned g, unsigned b){
  // We typically want to use tputs() and tiperm() to acquire and write the
  // escapes, as these take into account terminal-specific delays, padding,
  // etc. For the case of DirectColor, there is no suitable terminfo entry, but
  // we're also in that case working with hopefully more robust terminals.
  // If it doesn't work, eh, it doesn't work. Fuck the world; save yourself.
  if(nc->RGBflag){
    return term_esc_rgb(nc, out, false, r, g, b);
  }else{
    if(nc->setab == NULL){
      return -1;
    }
    // For 256-color indexed mode, start constructing a palette based off
    // the inputs *if we can change the palette*. If more than 256 are used on
    // a single screen, start... combining close ones? For 8-color mode, simple
    // interpolation. I have no idea what to do for 88 colors. FIXME
    if(nc->colors >= 256){
      term_emit("setab", tiparm(nc->setab, rgb_quantize_256(r, g, b)), out, false);
    }
    return -1;
  }
  return 0;
}

static int
term_fg_rgb8(notcurses* nc, FILE* out, unsigned r, unsigned g, unsigned b){
  // We typically want to use tputs() and tiperm() to acquire and write the
  // escapes, as these take into account terminal-specific delays, padding,
  // etc. For the case of DirectColor, there is no suitable terminfo entry, but
  // we're also in that case working with hopefully more robust terminals.
  // If it doesn't work, eh, it doesn't work. Fuck the world; save yourself.
  if(nc->RGBflag){
    return term_esc_rgb(nc, out, true, r, g, b);
  }else{
    if(nc->setaf == NULL){
      return -1;
    }
    if(nc->colors >= 256){
      term_emit("setaf", tiparm(nc->setaf, rgb_quantize_256(r, g, b)), out, false);
    }
    // For 256-color indexed mode, start constructing a palette based off
    // the inputs *if we can change the palette*. If more than 256 are used on
    // a single screen, start... combining close ones? For 8-color mode, simple
    // interpolation. I have no idea what to do for 88 colors. FIXME
    return -1;
  }
  return 0;
}

// Producing the frame requires three steps:
//  * render -- build up a flat framebuffer from a set of ncplanes
//  * rasterize -- build up a UTF-8 stream of escapes and EGCs
//  * refresh -- write the stream to the emulator
static inline int
notcurses_rasterize(notcurses* nc, const struct crender* rvec){
  FILE* out = nc->rstate.mstreamfp;
  int ret = 0;
  int y, x;
  fseeko(out, 0, SEEK_SET);
  // we only need to emit a coordinate if it was damaged. the damagemap is a
  // bit per coordinate, rows by rows, column by column within a row, with the
  // MSB being the first coordinate.
  size_t damageidx = 0;
  // don't write a clearscreen. we only update things that have been changed.
  // we explicitly move the cursor at the beginning of each output line, so no
  // need to home it expliticly.
  for(y = 0 ; y < nc->stdscr->leny ; ++y){
    // how many characters have we elided? it's not worthwhile to invoke a
    // cursor movement with cup if we only elided one or two. set to INT_MAX
    // whenever we're on a new line. leave room to avoid overflow.
    int needmove = INT_MAX - nc->stdscr->lenx;
    for(x = 0 ; x < nc->stdscr->lenx ; ++x){
      unsigned r, g, b, br, bg, bb;
      const cell* srccell = &nc->lastframe[y * nc->lfdimx + x];
//      cell c;
//      memcpy(c, srccell, sizeof(*c)); // unsafe copy of gcluster
//fprintf(stderr, "COPYING: %d from %p\n", c->gcluster, &nc->pool);
//      const char* egc = pool_egc_copy(&nc->pool, srccell);
//      c->gcluster = 0; // otherwise cell_release() will blow up
      if(!rvec[damageidx].damaged){
        // no need to emit a cell; what we rendered appears to already be
        // here. no updates are performed to elision state nor lastframe.
        ++nc->stats.cellelisions;
        ++needmove;
        if(cell_double_wide_p(srccell)){
          ++needmove;
          ++nc->stats.cellelisions;
        }
      }else{
        ++nc->stats.cellemissions;
        if(needmove == 1 && nc->cuf1){
          ret |= term_emit("cuf1", tiparm(nc->cuf1), out, false);
        }else if(needmove){
          ret |= term_emit("cup", tiparm(nc->cup, y, x), out, false);
        }
        needmove = 0;
        // set the style. this can change the color back to the default; if it
        // does, we need update our elision possibilities.
        bool normalized;
        ret |= term_setstyles(nc, out, &nc->rstate.curattr, srccell, &normalized);
        if(normalized){
          nc->rstate.defaultelidable = true;
          nc->rstate.bgelidable = false;
          nc->rstate.fgelidable = false;
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
        }

        // if our cell has a non-default foreground, we can elide the non-default
        // foreground set iff either:
        //  * the previous was non-default, and matches what we have now, or
        //  * we are a no-foreground glyph (iswspace() is true)
        if(!cell_fg_default_p(srccell)){
          if(!noforeground){
            cell_fg_rgb(srccell, &r, &g, &b);
//fprintf(stderr, "[%03d/%03d] %02x %02x %02x\n", y, x, r, g, b);
            if(nc->rstate.fgelidable && nc->rstate.lastr == r && nc->rstate.lastg == g && nc->rstate.lastb == b){
              ++nc->stats.fgelisions;
            }else{
              ret |= term_fg_rgb8(nc, out, r, g, b);
              ++nc->stats.fgemissions;
              nc->rstate.fgelidable = true;
            }
            nc->rstate.lastr = r; nc->rstate.lastg = g; nc->rstate.lastb = b;
            nc->rstate.defaultelidable = false;
          }else{
            ++nc->stats.fgelisions;
          }
        }
        if(!cell_bg_default_p(srccell)){
          if(!nobackground){
            cell_bg_rgb(srccell, &br, &bg, &bb);
            if(nc->rstate.bgelidable && nc->rstate.lastbr == br && nc->rstate.lastbg == bg && nc->rstate.lastbb == bb){
              ++nc->stats.bgelisions;
            }else{
              ret |= term_bg_rgb8(nc, out, br, bg, bb);
              ++nc->stats.bgemissions;
              nc->rstate.bgelidable = true;
            }
            nc->rstate.lastbr = br; nc->rstate.lastbg = bg; nc->rstate.lastbb = bb;
            nc->rstate.defaultelidable = false;
          }else{
            ++nc->stats.bgelisions;
          }
        }
/*if(cell_simple_p(srccell)){
fprintf(stderr, "RAST %u [%c] to %d/%d\n", srccell->gcluster, srccell->gcluster, y, x);
}else{
fprintf(stderr, "RAST %u [%s] to %d/%d\n", srccell->gcluster, egcpool_extended_gcluster(&nc->pool, srccell), y, x);
}*/
        ret |= term_putc(out, &nc->pool, srccell);
      }
      if(cell_double_wide_p(srccell)){
        ++x;
        ++damageidx;
      }
      ++damageidx;
    }
  }
  ret |= fflush(out);
  //fflush(nc->ttyfp);
  if(blocking_write(nc->ttyfd, nc->rstate.mstream, nc->rstate.mstrsize)){
    ret = -1;
  }
/*fprintf(stderr, "%lu/%lu %lu/%lu %lu/%lu\n", defaultelisions, defaultemissions,
     fgelisions, fgemissions, bgelisions, bgemissions);*/
  if(nc->renderfp){
    fprintf(nc->renderfp, "%s\n", nc->rstate.mstream);
  }
  if(ret < 0){
    return ret;
  }
  return nc->rstate.mstrsize;
}

int notcurses_render(notcurses* nc){
  struct timespec start, done;
  int ret;
  clock_gettime(CLOCK_MONOTONIC_RAW, &start);
  pthread_mutex_lock(&nc->lock);
  pthread_cleanup_push(mutex_unlock, &nc->lock);
  int bytes = -1;
  size_t crenderlen = sizeof(struct crender) * nc->stdscr->leny * nc->stdscr->lenx;
  struct crender* crender = malloc(crenderlen);
  memset(crender, 0, crenderlen);
  if(notcurses_render_internal(nc, crender) == 0){
    bytes = notcurses_rasterize(nc, crender);
  }
  free(crender);
  int dimy, dimx;
  notcurses_resize(nc, &dimy, &dimx);
  clock_gettime(CLOCK_MONOTONIC_RAW, &done);
  update_render_stats(&done, &start, &nc->stats, bytes);
  ret = bytes >= 0 ? 0 : -1;
  pthread_cleanup_pop(1);
  return ret;
}

char* notcurses_at_yx(notcurses* nc, int y, int x, cell* c){
  char* egc = NULL;
  pthread_mutex_lock(&nc->lock);
  if(nc->lastframe){
    if(y >= 0 && y < nc->lfdimy){
      if(x >= 0 || x < nc->lfdimx){
        const cell* srccell = &nc->lastframe[y * nc->lfdimx + x];
        memcpy(c, srccell, sizeof(*c)); // unsafe copy of gcluster
//fprintf(stderr, "COPYING: %d from %p\n", c->gcluster, &nc->pool);
        egc = pool_egc_copy(&nc->pool, srccell);
        c->gcluster = 0; // otherwise cell_release() will blow up
      }
    }
  }
  pthread_mutex_unlock(&nc->lock);
  return egc;
}
