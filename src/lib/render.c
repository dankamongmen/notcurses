#include <ctype.h>
#include <unistd.h>
#include <sys/poll.h>
#include "internal.h"

static inline uint64_t
timespec_to_ns(const struct timespec* t){
  return t->tv_sec * NANOSECS_IN_SEC + t->tv_nsec;
}

static void
mutex_unlock(void* vlock){
  pthread_mutex_unlock(vlock);
}

static int
blocking_write(int fd, const char* buf, size_t buflen){
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
  if(nc->mstream == NULL){
    ret = -1; // haven't rendered yet, and thus don't know what should be there
  }else if(blocking_write(nc->ttyfd, nc->mstream, nc->mstrsize)){
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

// determine the best palette for the current frame, and write the necessary
// escape sequences to 'out'. for now, we just assume the ANSI palettes. at
// 256 colors, this is the 16 normal ones, 6x6x6 color cubes, and 32 greys.
// it's probably better to sample the darker regions rather than cover so much
// chroma, but whatever....FIXME
static inline int
prep_optimized_palette(notcurses* nc, FILE* out __attribute__ ((unused))){
  if(nc->RGBflag){
    return 0; // DirectColor, no need to write palette
  }
  if(!nc->CCCflag){
    return 0; // can't change palette
  }
  // FIXME
  return 0;
}

/*
// reshape the shadow framebuffer to match the stdplane's dimensions, throwing
// away the old one.
static int
reshape_shadow_fb(notcurses* nc){
  const size_t size = sizeof(nc->shadowbuf) * nc->stdscr->leny * nc->stdscr->lenx;
  cell* fb = malloc(size);
  if(fb == NULL){
    return -1;
  }
  free(nc->shadowbuf);
  nc->shadowbuf = fb;
  nc->shadowy = nc->stdscr->leny;
  nc->shadowx = nc->stdscr->lenx;
  memset(fb, 0, size);
  return 0;
}
*/

// Find the topmost cell for this coordinate by walking down the z-buffer,
// looking for an intersecting ncplane. Once we've found one, check it for
// transparency in either the back- or foreground. If the alpha channel is
// active, keep descending and blending until we hit opacity, or bedrock. We
// recurse to find opacity, and blend the result into what we have. The
// 'findfore' and 'findback' bools control our recursion--there's no point in
// going further down when a color is locked in, so don't (for instance) recurse
// further when we have a transparent foreground and opaque background atop an
// opaque foreground and transparent background. The cell we ultimately return
// (a const ref to 'c') is backed by '*retp' via rawdog copy; the caller must
// not call cell_release() upon it, nor use it beyond the scope of the render.
//
// So, as we go down, we find planes which can have impact on the result. Once
// we've locked the result in (base case), write the deep values we have to 'c'.
// Then, as we come back up, blend them as appropriate. The actual glyph is
// whichever one occurs at the top with a non-transparent α (α < 3). To effect
// tail recursion, though, we instead write first, and then recurse, blending
// as we descend. α <= 0 is opaque. α >= 3 is fully transparent.
static ncplane*
dig_visible_cell(cell* c, int y, int x, ncplane* p, int falpha, int balpha,
                 bool* damage){
  // once we decide on our glyph, it cannot be changed by anything below, so
  // lock in this plane for the actual cell return.
  ncplane* glyphplane = NULL;
  while(p){
    // where in the plane this coordinate would be, based off absy/absx. the
    // true origin is 0,0, so abs=2,2 means coordinate 3,3 would be 1,1, while
    // abs=-2,-2 would make coordinate 3,3 relative 5, 5.
    int poffx, poffy;
    poffy = y - p->absy;
    poffx = x - p->absx;
    if(poffy < p->leny && poffy >= 0){
      if(poffx < p->lenx && poffx >= 0){ // p is valid for this y, x
        const cell* vis = &p->fb[fbcellidx(p, poffy, poffx)];
        // if we never loaded any content into the cell (or obliterated it by
        // writing in a zero), use the plane's background cell.
        if(vis->gcluster == 0){
          vis = &p->defcell;
        }
        int nalpha;
        if(falpha > 0 && (nalpha = cell_get_fg_alpha(vis)) < CELL_ALPHA_TRANS){
          if(c->gcluster == 0){ // never write fully trans glyphs, never replace
            if( (c->gcluster = vis->gcluster) ){ // index copy only
              glyphplane = p; // must return this ncplane for this glyph
              c->attrword = vis->attrword;
              cell_set_fchannel(c, cell_get_fchannel(vis)); // FIXME blend it in
              falpha -= (CELL_ALPHA_TRANS - nalpha); // FIXME blend it in
              if(p->damage[poffy]){
                *damage = true;
                p->damage[poffy] = false;
              }
            }
          }
        }
        if(balpha > 0 && (nalpha = cell_get_bg_alpha(vis)) < CELL_ALPHA_TRANS){
          cell_set_bchannel(c, cell_get_bchannel(vis)); // FIXME blend it in
          balpha -= (CELL_ALPHA_TRANS - nalpha);
          if(p->damage[poffy]){
            *damage = true;
            p->damage[poffy] = false;
          }
        }
        if((falpha <= 0 && balpha <= 0) || !p->z){ // done!
          return glyphplane ? glyphplane : p;
        }
      }
    }
    p = p->z;
  }
  // should never happen for valid y, x thanks to the stdplane. you fucked up!
  return NULL;
}

static inline ncplane*
visible_cell(cell* c, int y, int x, ncplane* n, bool* damage){
  cell_init(c);
  return dig_visible_cell(c, y, x, n, CELL_ALPHA_TRANS, CELL_ALPHA_TRANS, damage);
}

// write the cell's UTF-8 grapheme cluster to the provided FILE*. returns the
// number of columns occupied by this EGC (only an approximation; it's actually
// a property of the font being used).
static int
term_putc(FILE* out, const ncplane* n, const cell* c){
  if(cell_simple_p(c)){
    if(c->gcluster == 0 || iscntrl(c->gcluster)){
// fprintf(stderr, "[ ]\n");
      if(fputc_unlocked(' ', out) == EOF){
        return -1;
      }
    }else{
// fprintf(stderr, "[%c]\n", c->gcluster);
      if(fputc_unlocked(c->gcluster, out) == EOF){
        return -1;
      }
    }
  }else{
    const char* ext = extended_gcluster(n, c);
// fprintf(stderr, "[%s]\n", ext);
    if(fputs_unlocked(ext, out) < 0){ // FIXME check for short write?
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

// 3 for foreground, 4 for background, ugh FIXME
static int
term_esc_rgb(notcurses* nc __attribute__ ((unused)), FILE* out, int esc,
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
  #define RGBESC2 "8;2;"
                                    // rrr;ggg;bbbm
  char rgbesc[] = RGBESC1 " " RGBESC2 "            ";
  int len = strlen(RGBESC1);
  rgbesc[len++] = esc;
  len += strlen(RGBESC2);
  if(r > 99){
    rgbesc[len++] = r / 100 + '0';
  }
  if(r > 9){
    rgbesc[len++] = (r % 100) / 10 + '0';
  }
  rgbesc[len++] = (r % 10) + '0';
  rgbesc[len++] = ';';
  if(g > 99){ rgbesc[len++] = g / 100 + '0'; }
  if(g > 9){ rgbesc[len++] = (g % 100) / 10 + '0'; }
  rgbesc[len++] = g % 10 + '0';
  rgbesc[len++] = ';';
  if(b > 99){ rgbesc[len++] = b / 100 + '0'; }
  if(b > 9){ rgbesc[len++] = (b % 100) / 10 + '0'; }
  rgbesc[len++] = b % 10 + '0';
  rgbesc[len++] = 'm';
  rgbesc[len] = '\0';
  int w;
  if((w = fputs_unlocked(rgbesc, out)) < len){
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
    return term_esc_rgb(nc, out, '4', r, g, b);
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
    return term_esc_rgb(nc, out, '3', r, g, b);
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

static inline int
notcurses_render_internal(notcurses* nc){
  int ret = 0;
  int y, x;
  FILE* out = nc->mstreamfp;
  fseeko(out, 0, SEEK_SET);
  // don't write a clearscreen. we only update things that have been changed.
  // we explicitly move the cursor at the beginning of each output line, so no
  // need to home it expliticly.
  prep_optimized_palette(nc, out); // FIXME do what on failure?
  uint32_t curattr = 0; // current attributes set (does not include colors)
  // FIXME as of at least gcc 9.2.1, we get a false -Wmaybe-uninitialized below
  // when using these without explicit initializations. for the life of me, i
  // can't see any such path, and valgrind is cool with it, so what ya gonna do?
  unsigned lastr = 0, lastg = 0, lastb = 0;
  unsigned lastbr = 0, lastbg = 0, lastbb = 0;
  // we can elide a color escape iff the color has not changed between the two
  // cells and the current cell uses no defaults, or if both the current and
  // the last used both defaults.
  bool fgelidable = false, bgelidable = false, defaultelidable = false;
  /*if(nc->stdscr->leny != nc->shadowy || nc->stdscr->lenx != nc->shadowx){
    reshape_shadow_fb(nc);
  }*/
  for(y = 0 ; y < nc->stdscr->leny ; ++y){
    bool linedamaged = false; // have we repositioned the cursor to start line?
    bool newdamage = nc->damage[y];
// fprintf(stderr, "nc->damage[%d] (%p) = %u\n", y, nc->damage + y, nc->damage[y]);
    if(newdamage){
      nc->damage[y] = 0;
    }
    // move to the beginning of the line, in case our accounting was befouled
    // by wider- (or narrower-) than-reported characters
    for(x = 0 ; x < nc->stdscr->lenx ; ++x){
      unsigned r, g, b, br, bg, bb;
      ncplane* p;
      cell c; // no need to initialize
      p = visible_cell(&c, y, x, nc->top, &newdamage);
      assert(p);
      // don't try to print a wide character on the last column; it'll instead
      // be printed on the next line. they probably shouldn't be admitted, but
      // we can end up with one due to a resize.
      if((x + 1 >= nc->stdscr->lenx && cell_double_wide_p(&c))){
        continue;
      }
      if(!linedamaged){
        if(newdamage){
          term_emit("cup", tiparm(nc->cup, y, x), out, false);
          nc->stats.cellelisions += x;
          nc->stats.cellemissions += (nc->stdscr->lenx - x);
          linedamaged = true;
        }else{
          continue;
        }
      }
      // set the style. this can change the color back to the default; if it
      // does, we need update our elision possibilities.
      bool normalized;
      term_setstyles(nc, out, &curattr, &c, &normalized);
      if(normalized){
        defaultelidable = true;
        bgelidable = false;
        fgelidable = false;
      }
      // we allow these to be set distinctly, but terminfo only supports using
      // them both via the 'op' capability. unless we want to generate the 'op'
      // escapes ourselves, if either is set to default, we first send op, and
      // then a turnon for whichever aren't default.

      // we can elide the default set iff the previous used both defaults
      if(cell_fg_default_p(&c) || cell_bg_default_p(&c)){
        if(!defaultelidable){
          ++nc->stats.defaultemissions;
          term_emit("op", nc->op, out, false);
        }else{
          ++nc->stats.defaultelisions;
        }
        // if either is not default, this will get turned off
        defaultelidable = true;
        fgelidable = false;
        bgelidable = false;
      }

      // we can elide the foreground set iff the previous used fg and matched
      if(!cell_fg_default_p(&c)){
        cell_get_fg_rgb(&c, &r, &g, &b);
        if(fgelidable && lastr == r && lastg == g && lastb == b){
          ++nc->stats.fgelisions;
        }else{
          term_fg_rgb8(nc, out, r, g, b);
          ++nc->stats.fgemissions;
          fgelidable = true;
        }
        lastr = r; lastg = g; lastb = b;
        defaultelidable = false;
      }
      if(!cell_bg_default_p(&c)){
        cell_get_bg_rgb(&c, &br, &bg, &bb);
        if(bgelidable && lastbr == br && lastbg == bg && lastbb == bb){
          ++nc->stats.bgelisions;
        }else{
          term_bg_rgb8(nc, out, br, bg, bb);
          ++nc->stats.bgemissions;
          bgelidable = true;
        }
        lastbr = br; lastbg = bg; lastbb = bb;
        defaultelidable = false;
      }
// fprintf(stderr, "[%02d/%02d] 0x%02x 0x%02x 0x%02x %p\n", y, x, r, g, b, p);
      term_putc(out, p, &c);
      if(cell_double_wide_p(&c)){
        ++x;
      }
    }
    if(linedamaged == false){
      nc->stats.cellelisions += x;
    }
  }
  ret |= fflush(out);
  fflush(nc->ttyfp);
  if(blocking_write(nc->ttyfd, nc->mstream, nc->mstrsize)){
    ret = -1;
  }
/*fprintf(stderr, "%lu/%lu %lu/%lu %lu/%lu\n", defaultelisions, defaultemissions,
     fgelisions, fgemissions, bgelisions, bgemissions);*/
  if(nc->renderfp){
    fprintf(nc->renderfp, "%s\n", nc->mstream);
  }
  return nc->mstrsize;
}

int notcurses_render(notcurses* nc){
  struct timespec start, done;
  int ret;
  clock_gettime(CLOCK_MONOTONIC_RAW, &start);
  pthread_mutex_lock(&nc->lock);
  pthread_cleanup_push(mutex_unlock, &nc->lock);
  int bytes = notcurses_render_internal(nc);
  int dimy, dimx;
  notcurses_resize(nc, &dimy, &dimx);
  clock_gettime(CLOCK_MONOTONIC_RAW, &done);
  update_render_stats(&done, &start, &nc->stats, bytes);
  ret = bytes >= 0 ? 0 : -1;
  pthread_cleanup_pop(1);
  return ret;
}

