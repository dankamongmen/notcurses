#include <poll.h>
#include <ctype.h>
#include <limits.h>
#include <unistd.h>
#include <notcurses/direct.h>
#include "internal.h"

// Check whether the terminal geometry has changed, and if so, copies what can
// be copied from the old lastframe. Assumes that the screen is always anchored
// at the same origin.
static int
notcurses_resize_internal(ncplane* pp, int* restrict rows, int* restrict cols){
  notcurses* n = ncplane_notcurses(pp);
  int r, c;
  if(rows == NULL){
    rows = &r;
  }
  if(cols == NULL){
    cols = &c;
  }
  ncpile* pile = ncplane_pile(pp);
  int oldrows = pile->dimy;
  int oldcols = pile->dimx;
  *rows = oldrows;
  *cols = oldcols;
  if(update_term_dimensions(n->ttyfd, rows, cols, &n->tcache)){
    return -1;
  }
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
    nccell* fb = realloc(n->lastframe, size);
    if(fb == NULL){
      return -1;
    }
    n->lastframe = fb;
    // FIXME more memset()tery than we need, both wasting work and wrecking
    // damage detection for the upcoming render
    memset(n->lastframe, 0, size);
    egcpool_dump(&n->pool);
  }
//fprintf(stderr, "r: %d or: %d c: %d oc: %d\n", *rows, oldrows, *cols, oldcols);
  if(*rows == oldrows && *cols == oldcols){
    return 0; // no change
  }
  pile->dimy = *rows;
  pile->dimx = *cols;
  int ret = 0;
//notcurses_debug(n, stderr);
  for(ncplane* rootn = pile->roots ; rootn ; rootn = rootn->bnext){
    if(rootn->resizecb){
      ret |= rootn->resizecb(rootn);
    }
  }
  return ret;
}

static int
notcurses_resize(notcurses* n, int* restrict rows, int* restrict cols){
  pthread_mutex_lock(&n->pilelock);
  int ret = notcurses_resize_internal(notcurses_stdplane(n), rows, cols);
  pthread_mutex_unlock(&n->pilelock);
  return ret;
}

static int
blocking_write(int fd, const char* buf, size_t buflen){
//fprintf(stderr, "writing %zu to %d...\n", buflen, fd);
  size_t written = 0;
  while(written < buflen){
    ssize_t w = write(fd, buf + written, buflen - written);
    if(w < 0){
      if(errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR){
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

// update timings for writeout. only call on success. call only under statlock.
static void
update_write_stats(const struct timespec* time1, const struct timespec* time0,
                   ncstats* stats, int bytes){
  if(bytes >= 0){
    const int64_t elapsed = timespec_to_ns(time1) - timespec_to_ns(time0);
    if(elapsed > 0){ // don't count clearly incorrect information, egads
      ++stats->writeouts;
      stats->writeout_ns += elapsed;
      if(elapsed > stats->writeout_max_ns){
        stats->writeout_max_ns = elapsed;
      }
      if(elapsed < stats->writeout_min_ns){
        stats->writeout_min_ns = elapsed;
      }
    }
  }else{
    ++stats->failed_writeouts;
  }
}

// negative 'bytes' is recorded as a failure. call only while holding statlock.
static void
update_render_bytes(ncstats* stats, int bytes){
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
}

// call only while holding statlock.
static void
update_render_stats(const struct timespec* time1, const struct timespec* time0,
                    ncstats* stats){
  const int64_t elapsed = timespec_to_ns(time1) - timespec_to_ns(time0);
  //fprintf(stderr, "Rendering took %ld.%03lds\n", elapsed / NANOSECS_IN_SEC,
  //        (elapsed % NANOSECS_IN_SEC) / 1000000);
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

// call only while holding statlock.
static void
update_raster_stats(const struct timespec* time1, const struct timespec* time0,
                    ncstats* stats){
  const int64_t elapsed = timespec_to_ns(time1) - timespec_to_ns(time0);
  //fprintf(stderr, "Rasterizing took %ld.%03lds\n", elapsed / NANOSECS_IN_SEC,
  //        (elapsed % NANOSECS_IN_SEC) / 1000000);
  if(elapsed > 0){ // don't count clearly incorrect information, egads
    stats->raster_ns += elapsed;
    if(elapsed > stats->raster_max_ns){
      stats->raster_max_ns = elapsed;
    }
    if(elapsed < stats->raster_min_ns){
      stats->raster_min_ns = elapsed;
    }
  }
}

void cell_release(ncplane* n, nccell* c){
  pool_release(&n->pool, c);
}

// Duplicate one cell onto another when they share a plane. Convenience wrapper.
int cell_duplicate(ncplane* n, nccell* targ, const nccell* c){
  if(cell_duplicate_far(&n->pool, targ, n, c) < 0){
    logerror(ncplane_notcurses(n), "Failed duplicating cell\n");
    return -1;
  }
  return 0;
}

// Extracellular state for a cell during the render process. There is one
// crender per rendered cell, and they are initialized to all zeroes.
struct crender {
  const ncplane *p; // source of glyph for this cell
  nccell c;
  uint32_t hcfg;       // fg channel prior to HIGHCONTRAST (need full channel)
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
  } s;
};

// Emit fchannel with RGB changed to contrast effectively against bchannel.
static uint32_t
highcontrast(uint32_t bchannel){
  unsigned r = channel_r(bchannel);
  unsigned g = channel_g(bchannel);
  unsigned b = channel_b(bchannel);
  uint32_t conrgb = 0;
  if(r + g + b < 320){
    channel_set(&conrgb, 0xffffff);
  }else{
    channel_set(&conrgb, 0);
  }
  return conrgb;
}

// Paints a single ncplane 'p' into the provided scratch framebuffer 'fb' (we
// can't always write directly into lastframe, because we need build state to
// solve certain cells, and need compare their solved result to the last frame).
//
//  dstleny: leny of target rendering area described by rvec
//  dstlenx: lenx of target rendering area described by rvec
//  dstabsy: absy of target rendering area (relative to terminal)
//  dstabsx: absx of target rendering area (relative to terminal)
//
// only those cells where 'p' intersects with the target rendering area are
// rendered.
static void
paint(const ncplane* p, struct crender* rvec, int dstleny, int dstlenx,
      int dstabsy, int dstabsx){
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
    if(absy >= dstleny || absy < 0){
      break;
    }
    for(x = startx ; x < dimx ; ++x){ // iteration for each cell
      const int absx = x + offx;
      if(absx >= dstlenx || absx < 0){
        break;
      }
      struct crender* crender = &rvec[fbcellidx(absy, dstlenx, absx)];
      nccell* targc = &crender->c;
      if(cell_wide_right_p(targc)){
        continue;
      }

      if(cell_fg_alpha(targc) > CELL_ALPHA_OPAQUE){
        const nccell* vis = &p->fb[nfbcellidx(p, y, x)];
        if(cell_fg_default_p(vis)){
          vis = &p->basecell;
        }
        if(cell_fg_palindex_p(vis)){
          if(cell_fg_alpha(targc) == CELL_ALPHA_TRANSPARENT){
            cell_set_fg_palindex(targc, cell_fg_palindex(vis));
          }
        }else{
          if(cell_fg_alpha(vis) == CELL_ALPHA_HIGHCONTRAST){
            crender->s.highcontrast = true;
            crender->s.hcfgblends = crender->s.fgblends;
            crender->hcfg = cell_fchannel(targc);
          }
          unsigned fgblends = crender->s.fgblends;
          cell_blend_fchannel(targc, cell_fchannel(vis), &fgblends);
          crender->s.fgblends = fgblends;
          // crender->highcontrast can only be true if we just set it, since we're
          // about to set targc opaque based on crender->highcontrast (and this
          // entire stanza is conditional on targc not being CELL_ALPHA_OPAQUE).
          if(crender->s.highcontrast){
            cell_set_fg_alpha(targc, CELL_ALPHA_OPAQUE);
          }
        }
      }

      // Background color takes effect independently of whether we have a
      // glyph. If we've already locked in the background, it has no effect.
      // If it's transparent, it has no effect. Otherwise, update the
      // background channel and balpha.
      // Evaluate the background first, in case we have HIGHCONTRAST fg text.
      if(cell_bg_alpha(targc) > CELL_ALPHA_OPAQUE){
        const nccell* vis = &p->fb[nfbcellidx(p, y, x)];
        // to be on the blitter stacking path, we need
        //  1) crender->s.blittedquads to be non-zero (we're below semigraphics)
        //  2) cell_blittedquadrants(vis) to be non-zero (we're semigraphics)
        //  3) somewhere crender is 0, blittedquads is 1 (we're visible)
        if(!crender->s.blittedquads || !((~crender->s.blittedquads) & cell_blittedquadrants(vis))){
          if(cell_bg_default_p(vis)){
            vis = &p->basecell;
          }
          if(cell_bg_palindex_p(vis)){
            if(cell_bg_alpha(targc) == CELL_ALPHA_TRANSPARENT){
              cell_set_bg_palindex(targc, cell_bg_palindex(vis));
            }
          }else{
            unsigned bgblends = crender->s.bgblends;
            cell_blend_bchannel(targc, cell_bchannel(vis), &bgblends);
            crender->s.bgblends = bgblends;
          }
        }else{ // use the local foreground; we're stacking blittings
          if(cell_fg_default_p(vis)){
            vis = &p->basecell;
          }
          if(cell_fg_palindex_p(vis)){
            if(cell_bg_alpha(targc) == CELL_ALPHA_TRANSPARENT){
              cell_set_bg_palindex(targc, cell_fg_palindex(vis));
            }
          }else{
            unsigned bgblends = crender->s.bgblends;
            cell_blend_bchannel(targc, cell_fchannel(vis), &bgblends);
            crender->s.bgblends = bgblends;
          }
          crender->s.blittedquads = 0;
        }
      }

      // if we never loaded any content into the cell (or obliterated it by
      // writing in a zero), use the plane's base cell.
      // if we have no character in this cell, we continue to look for a
      // character, but our foreground color will still be used unless it's
      // been set to transparent. if that foreground color is transparent, we
      // still use a character we find here, but its color will come entirely
      // from cells underneath us.
      if(!crender->p){
        const nccell* vis = &p->fb[nfbcellidx(p, y, x)];
        if(vis->gcluster == 0 && !cell_double_wide_p(vis)){
          vis = &p->basecell;
        }
        // if the following is true, we're a real glyph, and not the right-hand
        // side of a wide glyph (nor the null codepoint).
        if( (targc->gcluster = vis->gcluster) ){ // index copy only
          crender->s.blittedquads = cell_blittedquadrants(vis);
          // we can't plop down a wide glyph if the next cell is beyond the
          // screen, nor if we're bisected by a higher plane.
          if(cell_double_wide_p(vis)){
            // are we on the last column of the real screen? if so, 0x20 us
            if(absx >= dstlenx - 1){
              targc->gcluster = htole(' ');
              targc->width = 1;
            // is the next cell occupied? if so, 0x20 us
            }else if(crender[1].c.gcluster){
//fprintf(stderr, "NULLING out %d/%d (%d/%d) due to %u\n", y, x, absy, absx, crender[1].c.gcluster);
              targc->gcluster = htole(' ');
              targc->width = 1;
            }else{
              targc->stylemask = vis->stylemask;
              targc->width = vis->width;
            }
          }else{
            targc->stylemask = vis->stylemask;
            targc->width = vis->width;
          }
          cell_set_pixels(targc, cell_pixels_p(vis));
          crender->p = p;
        }else if(cell_wide_right_p(vis)){
          crender->p = p;
          targc->width = 0;
        }
      }
    }
  }
}

// it's not a pure memset(), because CELL_ALPHA_OPAQUE is the zero value, and
// we need CELL_ALPHA_TRANSPARENT
static inline void
init_rvec(struct crender* rvec, int totalcells){
  struct crender c = {};
  cell_set_fg_alpha(&c.c, CELL_ALPHA_TRANSPARENT);
  cell_set_bg_alpha(&c.c, CELL_ALPHA_TRANSPARENT);
  for(int t = 0 ; t < totalcells ; ++t){
    memcpy(&rvec[t], &c, sizeof(c));
  }
}

// adjust an otherwise locked-in cell if highcontrast has been requested. this
// should be done at the end of rendering the cell, so that contrast is solved
// against the real background.
static inline void
lock_in_highcontrast(nccell* targc, struct crender* crender){
  if(cell_fg_alpha(targc) == CELL_ALPHA_TRANSPARENT){
    cell_set_fg_default(targc);
  }
  if(cell_bg_alpha(targc) == CELL_ALPHA_TRANSPARENT){
    cell_set_bg_default(targc);
  }
  if(crender->s.highcontrast){
    // highcontrast weighs the original at 1/4 and the contrast at 3/4
    if(!cell_fg_default_p(targc)){
      unsigned fgblends = 3;
      uint32_t fchan = cell_fchannel(targc);
      uint32_t bchan = cell_bchannel(targc);
      uint32_t hchan = channels_blend(highcontrast(bchan), fchan, &fgblends);
      cell_set_fchannel(targc, hchan);
      fgblends = crender->s.hcfgblends;
      hchan = channels_blend(hchan, crender->hcfg, &fgblends);
      cell_set_fchannel(targc, hchan);
    }else{
      cell_set_fg_rgb(targc, highcontrast(cell_bchannel(targc)));
    }
  }
}

// Postpaint a single cell (multiple if it is a multicolumn EGC). This means
// checking for and locking in high-contrast, checking for damage, and updating
// 'lastframe' for any cells which are damaged.
static inline void
postpaint_cell(nccell* lastframe, int dimx, struct crender* crender,
               egcpool* pool, int y, int* x){
  nccell* targc = &crender->c;
  lock_in_highcontrast(targc, crender);
  nccell* prevcell = &lastframe[fbcellidx(y, dimx, *x)];
  if(cellcmp_and_dupfar(pool, prevcell, crender->p, targc) > 0){
    crender->s.damaged = true;
    assert(!cell_wide_right_p(targc));
    const int width = targc->width;
    for(int i = 1 ; i < width ; ++i){
      const ncplane* tmpp = crender->p;
      ++crender;
      crender->p = tmpp;
      ++*x;
      ++prevcell;
      targc = &crender->c;
      targc->gcluster = 0;
      targc->channels = crender[-i].c.channels;
      targc->stylemask = crender[-i].c.stylemask;
      if(cellcmp_and_dupfar(pool, prevcell, crender->p, targc) > 0){
        crender->s.damaged = true;
      }
    }
  }
}


// iterate over the rendered frame, adjusting the foreground colors for any
// cells marked CELL_ALPHA_HIGHCONTRAST, and clearing any cell covered by a
// wide glyph to its left.
//
// FIXME this cannot be performed at render time (we don't yet know the
//       lastframe, and thus can't compute damage), but we *could* unite it
//       with rasterization--factor out the single cell iteration...
// FIXME can we not do the blend a single time here, if we track sums in
//       paint()? tried this before and didn't get a win...
static void
postpaint(nccell* lastframe, int dimy, int dimx, struct crender* rvec, egcpool* pool){
  for(int y = 0 ; y < dimy ; ++y){
    for(int x = 0 ; x < dimx ; ++x){
      struct crender* crender = &rvec[fbcellidx(y, dimx, x)];
      postpaint_cell(lastframe, dimx, crender, pool, y, &x);
    }
  }
}

// merging one plane down onto another is basically just performing a render
// using only these two planes, with the result written to the lower plane.
int ncplane_mergedown(const ncplane* restrict src, ncplane* restrict dst,
                      int begsrcy, int begsrcx, int leny, int lenx,
                      int dsty, int dstx){
//fprintf(stderr, "Merging down %d/%d @ %d/%d to %d/%d\n", leny, lenx, begsrcy, begsrcx, dsty, dstx);
  if(dsty >= dst->leny || dstx >= dst->lenx){
    logerror(ncplane_notcurses(dst), "Dest origin %d/%d ≥ dest dimensions %d/%d\n",
             dsty, dstx, dst->leny, dst->lenx);
    return -1;
  }
  if(dst->leny - leny < dsty || dst->lenx - lenx < dstx){
    logerror(ncplane_notcurses(dst), "Dest len %d/%d ≥ dest dimensions %d/%d\n",
             leny, lenx, dst->leny, dst->lenx);
    return -1;
  }
  if(begsrcy >= src->leny || begsrcx >= src->lenx){
    logerror(ncplane_notcurses(dst), "Source origin %d/%d ≥ source dimensions %d/%d\n",
             begsrcy, begsrcx, src->leny, src->lenx);
    return -1;
  }
  if(src->leny - leny < begsrcy || src->lenx - lenx < begsrcx){
    logerror(ncplane_notcurses(dst), "Source len %d/%d ≥ source dimensions %d/%d\n",
             leny, lenx, src->leny, src->lenx);
    return -1;
  }
  const int totalcells = dst->leny * dst->lenx;
  nccell* rendfb = calloc(sizeof(*rendfb), totalcells);
  const size_t crenderlen = sizeof(struct crender) * totalcells;
  struct crender* rvec = malloc(crenderlen);
  if(!rendfb || !rvec){
    logerror(ncplane_notcurses(dst), "Error allocating render state for %dx%d\n", leny, lenx);
    free(rendfb);
    free(rvec);
    return -1;
  }
  init_rvec(rvec, totalcells);
  paint(src, rvec, dst->leny, dst->lenx, dst->absy, dst->absx);
  paint(dst, rvec, dst->leny, dst->lenx, dst->absy, dst->absx);
//fprintf(stderr, "Postpaint start (%dx%d)\n", dst->leny, dst->lenx);
  postpaint(rendfb, dst->leny, dst->lenx, rvec, &dst->pool);
//fprintf(stderr, "Postpaint done (%dx%d)\n", dst->leny, dst->lenx);
  free(dst->fb);
  dst->fb = rendfb;
  free(rvec);
  return 0;
}

int ncplane_mergedown_simple(const ncplane* restrict src, ncplane* restrict dst){
  const notcurses* nc = ncplane_notcurses_const(src);
  if(dst == NULL){
    dst = nc->stdplane;
  }
  int dimy, dimx;
  ncplane_dim_yx(dst, &dimy, &dimx);
  return ncplane_mergedown(src, dst, 0, 0, ncplane_dim_y(src), ncplane_dim_x(src), 0, 0);
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

// write the nccell's UTF-8 extended grapheme cluster to the provided FILE*.
static int
term_putc(FILE* out, const egcpool* e, const nccell* c){
  if(cell_simple_p(c)){
//fprintf(stderr, "[%.4s] %08x\n", (const char*)&c->gcluster, c->gcluster); }
    // we must not have any 'cntrl' characters at this point
    if(c->gcluster == 0){
      if(ncfputc(' ', out) == EOF){
        return -1;
      }
    }else if(ncfputs((const char*)&c->gcluster, out) == EOF){
      return -1;
    }
  }else{
    if(ncfputs(egcpool_extended_gcluster(e, c), out) == EOF){
      return -1;
    }
  }
  return 0;
}

// check the current and target style bitmasks against the specified 'stylebit'.
// if they are different, and we have the necessary capability, write the
// applicable terminfo entry to 'out'. returns -1 only on a true error.
int term_setstyle(FILE* out, unsigned cur, unsigned targ, unsigned stylebit,
                  const char* ton, const char* toff){
  int ret = 0;
  unsigned curon = cur & stylebit;
  unsigned targon = targ & stylebit;
  if(curon != targon){
    if(targon){
      if(ton){
        ret = term_emit(ton, out, false);
      }
    }else{
      if(toff){ // how did this happen? we can turn it on, but not off?
        ret = term_emit(toff, out, false);
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
term_setstyles(FILE* out, notcurses* nc, const nccell* c){
  bool normalized = false;
  uint32_t cellattr = cell_styles(c);
  if(cellattr == nc->rstate.curattr){
    return 0; // happy agreement, change nothing
  }
  int ret = 0;
  // if only italics changed, don't emit any sgr escapes. xor of current and
  // target ought have all 0s in the lower 8 bits if only italics changed.
  if((cellattr ^ nc->rstate.curattr) & 0xfful){
    normalized = true; // FIXME this is pretty conservative
    // if everything's 0, emit the shorter sgr0
    if(nc->tcache.sgr0 && ((cellattr & NCSTYLE_MASK) == 0)){
      if(term_emit(nc->tcache.sgr0, out, false) < 0){
        ret = -1;
      }
    }else if(term_emit(tiparm(nc->tcache.sgr, cellattr & NCSTYLE_STANDOUT,
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
    // sgr will blow away italics/struck if they were set beforehand
    nc->rstate.curattr &= !(NCSTYLE_ITALIC | NCSTYLE_STRUCK);
  }
  ret |= term_setstyle(out, nc->rstate.curattr, cellattr, NCSTYLE_ITALIC,
                       nc->tcache.italics, nc->tcache.italoff);
  ret |= term_setstyle(out, nc->rstate.curattr, cellattr, NCSTYLE_STRUCK,
                       nc->tcache.struck, nc->tcache.struckoff);
  nc->rstate.curattr = cellattr;
  if(normalized){
    nc->rstate.fgdefelidable = true;
    nc->rstate.bgdefelidable = true;
    nc->rstate.bgelidable = false;
    nc->rstate.fgelidable = false;
    nc->rstate.bgpalelidable = false;
    nc->rstate.fgpalelidable = false;
  }
  return ret;
}

// u8->str lookup table used in term_esc_rgb below
static const char* const NUMBERS[] = {
"0;", "1;", "2;", "3;", "4;", "5;", "6;", "7;", "8;", "9;", "10;", "11;", "12;", "13;", "14;", "15;", "16;",
"17;", "18;", "19;", "20;", "21;", "22;", "23;", "24;", "25;", "26;", "27;", "28;", "29;", "30;", "31;", "32;",
"33;", "34;", "35;", "36;", "37;", "38;", "39;", "40;", "41;", "42;", "43;", "44;", "45;", "46;", "47;", "48;",
"49;", "50;", "51;", "52;", "53;", "54;", "55;", "56;", "57;", "58;", "59;", "60;", "61;", "62;", "63;", "64;",
"65;", "66;", "67;", "68;", "69;", "70;", "71;", "72;", "73;", "74;", "75;", "76;", "77;", "78;", "79;", "80;",
"81;", "82;", "83;", "84;", "85;", "86;", "87;", "88;", "89;", "90;", "91;", "92;", "93;", "94;", "95;", "96;",
"97;", "98;", "99;", "100;", "101;", "102;", "103;", "104;", "105;", "106;", "107;", "108;", "109;", "110;", "111;", "112;",
"113;", "114;", "115;", "116;", "117;", "118;", "119;", "120;", "121;", "122;", "123;", "124;", "125;", "126;", "127;", "128;",
"129;", "130;", "131;", "132;", "133;", "134;", "135;", "136;", "137;", "138;", "139;", "140;", "141;", "142;", "143;", "144;",
"145;", "146;", "147;", "148;", "149;", "150;", "151;", "152;", "153;", "154;", "155;", "156;", "157;", "158;", "159;", "160;",
"161;", "162;", "163;", "164;", "165;", "166;", "167;", "168;", "169;", "170;", "171;", "172;", "173;", "174;", "175;", "176;",
"177;", "178;", "179;", "180;", "181;", "182;", "183;", "184;", "185;", "186;", "187;", "188;", "189;", "190;", "191;", "192;",
"193;", "194;", "195;", "196;", "197;", "198;", "199;", "200;", "201;", "202;", "203;", "204;", "205;", "206;", "207;", "208;",
"209;", "210;", "211;", "212;", "213;", "214;", "215;", "216;", "217;", "218;", "219;", "220;", "221;", "222;", "223;", "224;",
"225;", "226;", "227;", "228;", "229;", "230;", "231;", "232;", "233;", "234;", "235;", "236;", "237;", "238;", "239;", "240;",
"241;", "242;", "243;", "244;", "245;", "246;", "247;", "248;", "249;", "250;", "251;", "252;", "253;", "254;", "255;", };

static inline int
term_esc_rgb(FILE* out, bool foreground, unsigned r, unsigned g, unsigned b){
  // The correct way to do this is using tiparm+tputs, but doing so (at least
  // as of terminfo 6.1.20191019) both emits ~3% more bytes for a run of 'rgb'
  // and gives rise to some inaccurate colors (possibly due to special handling
  // of values < 256; I'm not at this time sure). So we just cons up our own.
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
  while(*s){
    rgbbuf[offset++] = *s++;
  }
  s = NUMBERS[g];
  while(*s){
    rgbbuf[offset++] = *s++;
  }
  s = NUMBERS[b];
  while(*s != ';'){
    rgbbuf[offset++] = *s++;
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
             unsigned r, unsigned g, unsigned b, uint32_t bg_collides_default){
  // We typically want to use tputs() and tiperm() to acquire and write the
  // escapes, as these take into account terminal-specific delays, padding,
  // etc. For the case of DirectColor, there is no suitable terminfo entry, but
  // we're also in that case working with hopefully more robust terminals.
  // If it doesn't work, eh, it doesn't work. Fuck the world; save yourself.
  if(RGBflag){
    if(bg_collides_default){
      if((r == (bg_collides_default & 0xff0000lu)) &&
         (g == (bg_collides_default & 0xff00lu)) &&
         (b == (bg_collides_default & 0xfflu))){
        ++b; // what if it's 255 FIXME
      }
    }
    return term_esc_rgb(out, false, r, g, b);
  }else{
    if(setab == NULL){
      return 0;
    }
    // For 256-color indexed mode, start constructing a palette based off
    // the inputs *if we can change the palette*. If more than 256 are used on
    // a single screen, start... combining close ones? For 8-color mode, simple
    // interpolation. I have no idea what to do for 88 colors. FIXME
    if(colors >= 256){
      return term_emit(tiparm(setab, rgb_quantize_256(r, g, b)), out, false);
    }else if(colors >= 8){
      return term_emit(tiparm(setab, rgb_quantize_8(r, g, b)), out, false);
    }
  }
  return 0;
}

int term_fg_rgb8(bool RGBflag, const char* setaf, int colors, FILE* out,
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
      return 0;
    }
    // For 256-color indexed mode, start constructing a palette based off
    // the inputs *if we can change the palette*. If more than 256 are used on
    // a single screen, start... combining close ones? For 8-color mode, simple
    // interpolation. I have no idea what to do for 88 colors. FIXME
    if(colors >= 256){
      return term_emit(tiparm(setaf, rgb_quantize_256(r, g, b)), out, false);
    }else if(colors >= 8){
      return term_emit(tiparm(setaf, rgb_quantize_8(r, g, b)), out, false);
    }
  }
  return 0;
}

static inline int
update_palette(notcurses* nc, FILE* out){
  if(nc->tcache.CCCflag){
    for(size_t damageidx = 0 ; damageidx < sizeof(nc->palette.chans) / sizeof(*nc->palette.chans) ; ++damageidx){
      unsigned r, g, b;
      if(nc->palette_damage[damageidx]){
        channel_rgb8(nc->palette.chans[damageidx], &r, &g, &b);
        // Need convert RGB values [0..256) to [0..1000], ugh
        // FIXME need handle HSL case also
        r = r * 1000 / 255;
        g = g * 1000 / 255;
        b = b * 1000 / 255;
        term_emit(tiparm(nc->tcache.initc, damageidx, r, g, b), out, false);
        nc->palette_damage[damageidx] = false;
      }
    }
  }
  return 0;
}

// sync the drawing position to the specified location with as little overhead
// as possible (with nothing, if already at the right location). we prefer
// absolute horizontal moves (hpa) to relative ones, in the rare event that
// our understanding of our horizontal location is faulty.
// FIXME fall back to synthesized moves in the absence of capabilities (i.e.
// textronix lacks cup; fake it with horiz+vert moves)
// if hardcursorpos is non-zero, we always perform a cup
static inline int
goto_location(notcurses* nc, FILE* out, int y, int x, bool* hardcursorpos){
//fprintf(stderr, "going to %d/%d from %d/%d hard: %u\n", y, x, nc->rstate.y, nc->rstate.x, hardcursorpos);
  int ret = 0;
  // if we don't have hpa, force a cup even if we're only 1 char away. the only
  // terminal i know supporting cup sans hpa is vt100, and vt100 can suck it.
  // you can't use cuf for backwards moves anyway; again, vt100 can suck it.
  if(nc->rstate.y == y && nc->tcache.hpa && !hardcursorpos){ // only need move x
    if(nc->rstate.x == x){ // needn't move shit
      return 0;
    }
    if(x == nc->rstate.x + 1 && nc->tcache.cuf1){
      ret = term_emit(nc->tcache.cuf1, out, false);
    }else{
      ret = term_emit(tiparm(nc->tcache.hpa, x), out, false);
    }
  }else{
    // cup is required, no need to check for existence
    ret = term_emit(tiparm(nc->tcache.cup, y, x), out, false);
    *hardcursorpos = 0;
  }
  nc->rstate.x = x;
  nc->rstate.y = y;
  return ret;
}

// at least one of the foreground and background are the default. emit the
// necessary return to default (if one is necessary), and update rstate.
static inline int
raster_defaults(notcurses* nc, bool fgdef, bool bgdef, FILE* out){
  if(!nc->tcache.op){ // if we don't have op, we don't have fgop/bgop
    return 0;
  }
  bool mustsetfg = fgdef && !nc->rstate.fgdefelidable;
  bool mustsetbg = bgdef && !nc->rstate.bgdefelidable;
  if(!mustsetfg && !mustsetbg){ // don't need emit anything
    ++nc->stats.defaultelisions;
    return 0;
  }else if((mustsetfg && mustsetbg) || !nc->tcache.fgop){
    if(term_emit(nc->tcache.op, out, false)){
      return -1;
    }
    nc->rstate.fgdefelidable = true;
    nc->rstate.bgdefelidable = true;
    nc->rstate.fgelidable = false;
    nc->rstate.bgelidable = false;
    nc->rstate.fgpalelidable = false;
    nc->rstate.bgpalelidable = false;
  }else if(mustsetfg){
    if(term_emit(nc->tcache.fgop, out, false)){
      return -1;
    }
    nc->rstate.fgdefelidable = true;
    nc->rstate.fgelidable = false;
    nc->rstate.fgpalelidable = false;
  }else{
    if(term_emit(nc->tcache.bgop, out, false)){
      return -1;
    }
    nc->rstate.bgdefelidable = true;
    nc->rstate.bgelidable = false;
    nc->rstate.bgpalelidable = false;
  }
  ++nc->stats.defaultemissions;
  return 0;
}

// these are unlikely, so we leave it uninlined
static int
emit_fg_palindex(notcurses* nc, FILE* out, const nccell* srccell){
  unsigned palfg = cell_fg_palindex(srccell);
  // we overload lastr for the palette index; both are 8 bits
  if(nc->rstate.fgpalelidable && nc->rstate.lastr == palfg){
    ++nc->stats.fgelisions;
  }else{
    if(term_fg_palindex(nc, out, palfg)){
      return -1;
    }
    ++nc->stats.fgemissions;
    nc->rstate.fgpalelidable = true;
  }
  nc->rstate.lastr = palfg;
  nc->rstate.fgdefelidable = false;
  nc->rstate.fgelidable = false;
  return 0;
}

static int
emit_bg_palindex(notcurses* nc, FILE* out, const nccell* srccell){
  unsigned palbg = cell_bg_palindex(srccell);
  if(nc->rstate.bgpalelidable && nc->rstate.lastbr == palbg){
    ++nc->stats.bgelisions;
  }else{
    if(term_bg_palindex(nc, out, palbg)){
      return -1;
    }
    ++nc->stats.bgemissions;
    nc->rstate.bgpalelidable = true;
  }
  nc->rstate.lastr = palbg;
  nc->rstate.bgdefelidable = false;
  nc->rstate.bgelidable = false;
  return 0;
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
notcurses_rasterize_inner(notcurses* nc, const ncpile* p, FILE* out){
  const struct crender* rvec = p->crender;
  int y, x;
  fseeko(out, 0, SEEK_SET);
  // we only need to emit a coordinate if it was damaged. the damagemap is a
  // bit per coordinate, one per struct crender.
  // don't write a clearscreen. we only update things that have been changed.
  // we explicitly move the cursor at the beginning of each output line, so no
  // need to home it expliticly.
  update_palette(nc, out);
//fprintf(stderr, "pile %p ymax: %d xmax: %d\n", p, p->dimy + nc->stdplane->absy, p->dimx + nc->stdplane->absx);
  for(y = nc->stdplane->absy ; y < p->dimy + nc->stdplane->absy ; ++y){
    const int innery = y - nc->stdplane->absy;
    for(x = nc->stdplane->absx ; x < p->dimx + nc->stdplane->absx ; ++x){
      const int innerx = x - nc->stdplane->absx;
      const size_t damageidx = innery * nc->lfdimx + innerx;
      unsigned r, g, b, br, bg, bb;
      const nccell* srccell = &nc->lastframe[damageidx];
      if(!rvec[damageidx].s.damaged){
        // no need to emit a cell; what we rendered appears to already be
        // here. no updates are performed to elision state nor lastframe.
        ++nc->stats.cellelisions;
        if(cell_wide_left_p(srccell)){
          ++x;
        }
      }else{
        ++nc->stats.cellemissions;
        if(goto_location(nc, out, y, x, &nc->rstate.hardcursorpos)){
          return -1;
        }
        if(!cell_pixels_p(srccell)){
          // set the style. this can change the color back to the default; if it
          // does, we need update our elision possibilities.
          if(term_setstyles(out, nc, srccell)){
            return -1;
          }
          // if our cell has a default foreground *or* background, we can elide
          // the default set iff one of:
          //  * we are a partial glyph, and the previous was default on both, or
          //  * we are a no-foreground glyph, and the previous was default background, or
          //  * we are a no-background glyph, and the previous was default foreground
          bool nobackground = cell_nobackground_p(srccell);
          if((cell_fg_default_p(srccell)) || (!nobackground && cell_bg_default_p(srccell))){
            if(raster_defaults(nc, cell_fg_default_p(srccell),
                              !nobackground && cell_bg_default_p(srccell), out)){
              return -1;
            }
          }
          // if our cell has a non-default foreground, we can elide the
          // non-default foreground set iff either:
          //  * the previous was non-default, and matches what we have now, or
          //  * we are a no-foreground glyph (iswspace() is true)
          if(cell_fg_palindex_p(srccell)){ // palette-indexed foreground
            if(emit_fg_palindex(nc, out, srccell)){
              return -1;
            }
          }else if(!cell_fg_default_p(srccell)){ // rgb foreground
            cell_fg_rgb8(srccell, &r, &g, &b);
            if(nc->rstate.fgelidable && nc->rstate.lastr == r && nc->rstate.lastg == g && nc->rstate.lastb == b){
              ++nc->stats.fgelisions;
            }else{
              if(term_fg_rgb8(nc->tcache.RGBflag, nc->tcache.setaf, nc->tcache.colors, out, r, g, b)){
                return -1;
              }
              ++nc->stats.fgemissions;
              nc->rstate.fgelidable = true;
            }
            nc->rstate.lastr = r; nc->rstate.lastg = g; nc->rstate.lastb = b;
            nc->rstate.fgdefelidable = false;
            nc->rstate.fgpalelidable = false;
          }
          // if our cell has a non-default background, we can elide the
          // non-default background set iff either:
          //  * we do not use the background, because the cell is all-foreground,
          //  * the previous was non-default, and matches what we have now, or
          if(nobackground){
            ++nc->stats.bgelisions;
          }else if(cell_bg_palindex_p(srccell)){ // palette-indexed background
            if(emit_bg_palindex(nc, out, srccell)){
              return -1;
            }
          }else if(!cell_bg_default_p(srccell)){ // rgb background
            cell_bg_rgb8(srccell, &br, &bg, &bb);
            if(nc->rstate.bgelidable && nc->rstate.lastbr == br && nc->rstate.lastbg == bg && nc->rstate.lastbb == bb){
              ++nc->stats.bgelisions;
            }else{
              if(term_bg_rgb8(nc->tcache.RGBflag, nc->tcache.setab,
                              nc->tcache.colors, out, br, bg, bb,
                              nc->tcache.bg_collides_default)){
                return -1;
              }
              ++nc->stats.bgemissions;
              nc->rstate.bgelidable = true;
            }
            nc->rstate.lastbr = br; nc->rstate.lastbg = bg; nc->rstate.lastbb = bb;
            nc->rstate.bgdefelidable = false;
            nc->rstate.bgpalelidable = false;
          }
        }else{
          nc->rstate.hardcursorpos = true;
        }
//fprintf(stderr, "RAST %08x [%s] to %d/%d cols: %u %016lx\n", srccell->gcluster, pool_extended_gcluster(&nc->pool, srccell), y, x, srccell->width, srccell->channels);
        if(term_putc(out, &nc->pool, srccell)){
          return -1;
        }
        // if we just emitted a sixel, always force a hard cursor relocation
        ++nc->rstate.x;
        if(srccell->width >= 2){
          x += srccell->width - 1;
          nc->rstate.x += srccell->width - 1;
        }
      }
//fprintf(stderr, "damageidx: %ld\n", damageidx);
    }
  }
  if(fflush(out)){
    return -1;
  }
  return nc->rstate.mstrsize;
}

// rasterize the rendered frame, and blockingly write it out to the terminal.
static int
raster_and_write(notcurses* nc, const ncpile* p, FILE* out){
  if(notcurses_rasterize_inner(nc, p, out) < 0){
    return -1;
  }
  int ret = 0;
  //fflush(nc->ttyfp);
  if(blocking_write(fileno(nc->ttyfp), nc->rstate.mstream, nc->rstate.mstrsize)){
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

// if the cursor is enabled, store its location and disable it. then, once done
// rasterizing, enable it afresh, moving it to the stored location. if left on
// during rasterization, we'll get grotesque flicker. 'out' is a memstream
// used to collect a buffer.
static inline int
notcurses_rasterize(notcurses* nc, const ncpile* p, FILE* out){
  const int cursory = nc->cursory;
  const int cursorx = nc->cursorx;
  if(cursory >= 0){ // either both are good, or neither is
    notcurses_cursor_disable(nc);
  }
  int ret = raster_and_write(nc, p, out);
  if(cursory >= 0){
    notcurses_cursor_enable(nc, cursory, cursorx);
  }
  return ret;
}

// get the cursor to the upper-left corner by one means or another. will clear
// the screen if need be.
static int
home_cursor(notcurses* nc, bool flush){
  int ret = -1;
  if(nc->tcache.home){
    ret = term_emit(nc->tcache.home, nc->ttyfp, flush);
  }else if(nc->tcache.cup){
    ret = term_emit(tiparm(nc->tcache.cup, 1, 1), nc->ttyfp, flush);
  }else if(nc->tcache.clearscr){
    ret = term_emit(nc->tcache.clearscr, nc->ttyfp, flush);
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
  ncpile p;
  p.dimy = nc->stdplane->leny;
  p.dimx = nc->stdplane->lenx;
  const int count = (nc->lfdimx > p.dimx ? nc->lfdimx : p.dimx) *
                    (nc->lfdimy > p.dimy ? nc->lfdimy : p.dimy);
  p.crender = malloc(count * sizeof(*p.crender));
  if(p.crender == NULL){
    return -1;
  }
  memset(p.crender, 0, count * sizeof(*p.crender));
  for(int i = 0 ; i < count ; ++i){
    p.crender[i].s.damaged = true;
  }
  int ret = notcurses_rasterize(nc, &p, nc->rstate.mstreamfp);
  free(p.crender);
  if(ret < 0){
    return -1;
  }
  ++nc->stats.refreshes;
  return 0;
}

int notcurses_render_to_file(notcurses* nc, FILE* fp){
  if(nc->lfdimx == 0 || nc->lfdimy == 0){
    return 0;
  }
  char* rastered = NULL;
  size_t rastbytes = 0;
  FILE* out = open_memstream(&rastered, &rastbytes);
  if(out == NULL){
    return -1;
  }
  ncpile p;
  p.dimy = nc->stdplane->leny;
  p.dimx = nc->stdplane->lenx;
  const int count = (nc->lfdimx > p.dimx ? nc->lfdimx : p.dimx) *
                    (nc->lfdimy > p.dimy ? nc->lfdimy : p.dimy);
  p.crender = malloc(count * sizeof(*p.crender));
  if(p.crender == NULL){
    fclose(out);
    free(rastered);
    return -1;
  }
  memset(p.crender, 0, count * sizeof(*p.crender));
  for(int i = 0 ; i < count ; ++i){
    p.crender[i].s.damaged = true;
  }
  int ret = raster_and_write(nc, &p, out);
  free(p.crender);
  if(ret > 0){
    if(fprintf(fp, "%s", rastered) == ret){
      ret = 0;
    }else{
      ret = -1;
    }
  }
  fclose(out);
  free(rastered);
  return ret;
}


// We execute the painter's algorithm, starting from our topmost plane. The
// damagevector should be all zeros on input. On success, it will reflect
// which cells were changed. We solve for each coordinate's cell by walking
// down the z-buffer, looking at intersections with ncplanes. This implies
// locking down the EGC, the attributes, and the channels for each cell.
static void
ncpile_render_internal(ncplane* n, struct crender* rvec, int leny, int lenx,
                       int absy, int absx){
  ncplane* p = ncplane_pile(n)->top;
  while(p){
    paint(p, rvec, leny, lenx, absy, absx);
    p = p->below;
  }
}

int ncpile_rasterize(ncplane* n){
  struct timespec start, rasterdone, writedone;
  clock_gettime(CLOCK_MONOTONIC, &start);
  const ncpile* pile = ncplane_pile(n);
  struct notcurses* nc = ncplane_notcurses(n);
  const int miny = pile->dimy < nc->lfdimy ? pile->dimy : nc->lfdimy;
  const int minx = pile->dimx < nc->lfdimx ? pile->dimx : nc->lfdimx;
  postpaint(nc->lastframe, miny, minx, pile->crender, &nc->pool);
  clock_gettime(CLOCK_MONOTONIC, &rasterdone);
  int bytes = notcurses_rasterize(nc, pile, nc->rstate.mstreamfp);
  // accepts -1 as an indication of failure
  clock_gettime(CLOCK_MONOTONIC, &writedone);
  pthread_mutex_lock(&nc->statlock);
  update_render_bytes(&nc->stats, bytes);
  update_raster_stats(&rasterdone, &start, &nc->stats);
  update_write_stats(&writedone, &rasterdone, &nc->stats, bytes);
  pthread_mutex_unlock(&nc->statlock);
  if(bytes < 0){
    return -1;
  }
  return 0;
}

// ensure the crender vector of 'n' is properly sized for 'n'->dimy x 'n'->dimx,
// and initialize the rvec afresh for a new render.
static int
engorge_crender_vector(ncpile* n){
  if(n->dimy <= 0 || n->dimx <= 0){
    return -1;
  }
  const size_t crenderlen = n->dimy * n->dimx; // desired size
//fprintf(stderr, "crlen: %d y: %d x:%d\n", crenderlen, dimy, dimx);
  if(crenderlen != n->crenderlen){
    struct crender* tmp = realloc(n->crender, sizeof(*tmp) * crenderlen);
    if(tmp == NULL){
      return -1;
    }
    n->crender = tmp;
    n->crenderlen = crenderlen;
  }
  init_rvec(n->crender, crenderlen);
  return 0;
}

int ncpile_render(ncplane* n){
  struct timespec start, renderdone;
  clock_gettime(CLOCK_MONOTONIC, &start);
  notcurses* nc = ncplane_notcurses(n);
  ncpile* pile = ncplane_pile(n);
  // update our notion of screen geometry, and render against that
  notcurses_resize_internal(n, NULL, NULL);
  if(engorge_crender_vector(pile)){
    return -1;
  }
  // FIXME notcurses_stdplane() doesn't belong here
  ncpile_render_internal(n, pile->crender, pile->dimy, pile->dimx,
                         notcurses_stdplane(nc)->absy,
                         notcurses_stdplane(nc)->absx);
  clock_gettime(CLOCK_MONOTONIC, &renderdone);
  pthread_mutex_lock(&nc->statlock);
  update_render_stats(&renderdone, &start, &nc->stats);
  pthread_mutex_unlock(&nc->statlock);
  return 0;
}

int notcurses_render(notcurses* nc){
  ncplane* stdn = notcurses_stdplane(nc);
  if(ncpile_render(stdn)){
    return -1;
  }
  return(ncpile_rasterize(stdn));
}

// for now, we just run the top half of notcurses_render(), and copy out the
// memstream from within rstate. we want to allocate our own here, and return
// it, to avoid the copy, but we need feed the params through to do so FIXME.
int notcurses_render_to_buffer(notcurses* nc, char** buf, size_t* buflen){
  ncplane* stdn = notcurses_stdplane(nc);
  if(ncpile_render(stdn)){
    return -1;
  }
  int bytes = notcurses_rasterize_inner(nc, ncplane_pile(stdn), nc->rstate.mstreamfp);
  pthread_mutex_lock(&nc->statlock);
  update_render_bytes(&nc->stats, bytes);
  pthread_mutex_unlock(&nc->statlock);
  if(bytes < 0){
    return -1;
  }
  *buf = memdup(nc->rstate.mstreamfp, nc->rstate.mstrsize);
  if(buf == NULL){
    return -1;
  }
  *buflen = nc->rstate.mstrsize;
  return 0;
}

// copy the UTF8-encoded EGC out of the cell, whether simple or complex. the
// result is not tied to the ncplane, and persists across erases / destruction.
static inline char*
pool_egc_copy(const egcpool* e, const nccell* c){
  if(cell_simple_p(c)){
    return strdup((const char*)&c->gcluster);
  }
  return strdup(egcpool_extended_gcluster(e, c));
}

char* notcurses_at_yx(notcurses* nc, int yoff, int xoff, uint16_t* stylemask, uint64_t* channels){
  char* egc = NULL;
  if(nc->lastframe){
    if(yoff >= 0 && yoff < nc->lfdimy){
      if(xoff >= 0 || xoff < nc->lfdimx){
        const nccell* srccell = &nc->lastframe[yoff * nc->lfdimx + xoff];
        if(stylemask){
          *stylemask = srccell->stylemask;
        }
        if(channels){
          *channels = srccell->channels;
        }
//fprintf(stderr, "COPYING: %d from %p\n", srccell->gcluster, &nc->pool);
        egc = pool_egc_copy(&nc->pool, srccell);
      }
    }
  }
  return egc;
}

int ncdirect_set_bg_rgb(ncdirect* nc, unsigned rgb){
  if(rgb > 0xffffffu){
    return -1;
  }
  // FIXME need verify we're not palette, either
  if(!ncdirect_bg_default_p(nc) && channels_bg_rgb(nc->channels) == rgb){
    return 0;
  }
  if(term_bg_rgb8(nc->tcache.RGBflag, nc->tcache.setab, nc->tcache.colors, nc->ttyfp,
                  (rgb & 0xff0000u) >> 16u, (rgb & 0xff00u) >> 8u, rgb & 0xffu,
                  nc->tcache.bg_collides_default)){
    return -1;
  }
  channels_set_bg_rgb(&nc->channels, rgb);
  return 0;
}

int ncdirect_set_fg_rgb(ncdirect* nc, unsigned rgb){
  if(rgb > 0xffffffu){
    return -1;
  }
  // FIXME need verify we're not palette, either
  if(!ncdirect_fg_default_p(nc) && channels_fg_rgb(nc->channels) == rgb){
    return 0;
  }
  if(term_fg_rgb8(nc->tcache.RGBflag, nc->tcache.setaf, nc->tcache.colors, nc->ttyfp,
                  (rgb & 0xff0000u) >> 16u, (rgb & 0xff00u) >> 8u, rgb & 0xffu)){
    return -1;
  }
  channels_set_fg_rgb(&nc->channels, rgb);
  return 0;
}

int notcurses_cursor_enable(notcurses* nc, int y, int x){
  if(y < 0 || x < 0){
    logerror(nc, "Illegal cursor placement: %d, %d\n", y, x);
    return -1;
  }
  if(y >= nc->stdplane->leny || x >= nc->stdplane->lenx){
    logerror(nc, "Illegal cursor placement: %d, %d\n", y, x);
    return -1;
  }
  // if we're already at the demanded location, we must already be visible, and
  // we needn't move the cursor -- return success immediately.
  if(nc->cursory == y && nc->cursorx == x){
    return 0;
  }
  if(nc->ttyfd < 0 || !nc->tcache.cnorm){
    return -1;
  }
  bool hardcursorpos = false;
  if(goto_location(nc, nc->ttyfp, y + nc->stdplane->absy, x + nc->stdplane->absx, &hardcursorpos)){
    return -1;
  }
  // if we were already positive, we're already visible, no need to write cnorm
  if(nc->cursory >= 0 && nc->cursorx >= 0){
    nc->cursory = y;
    nc->cursorx = x;
    return 0;
  }
  if(tty_emit(nc->tcache.cnorm, nc->ttyfd) || fflush(nc->ttyfp) == EOF){
    return -1;
  }
  nc->cursory = y;
  nc->cursorx = x;
  return 0;
}

int notcurses_cursor_disable(notcurses* nc){
  if(nc->cursorx < 0 || nc->cursory < 0){
    logerror(nc, "Cursor is not enabled\n");
    return -1;
  }
  if(nc->ttyfd >= 0){
    if(nc->tcache.civis){
      if(!tty_emit(nc->tcache.civis, nc->ttyfd) && !fflush(nc->ttyfp)){
        nc->cursory = -1;
        nc->cursorx = -1;
        return 0;
      }
    }
  }
  return -1;
}
