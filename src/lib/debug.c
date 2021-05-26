#include "internal.h"

static inline char
capyn(const char* cap){
  return cap ? 'y' : 'n';
}

static inline char
capbool(bool cap){
  return cap ? 'y' : 'n';
}

static void
tinfo_debug_caps(const tinfo* ti, FILE* debugfp, int rows, int cols,
                 unsigned images, unsigned videos){
  const char indent[] = " ";
  fprintf(debugfp, "%sColors: %u rgb: %c ccc: %c setaf: %c setab: %c\n",
          indent, ti->colors, capbool(ti->RGBflag), capbool(ti->CCCflag), capyn(ti->setaf), capyn(ti->setab));
  fprintf(debugfp, "%ssgr: %c sgr0: %c\n",
          indent, capyn(ti->sgr), capyn(ti->sgr0));
  fprintf(debugfp, "%sop: %c fgop: %c bgop: %c\n",
          indent, capyn(ti->op), capyn(ti->fgop), capyn(ti->bgop));
  fprintf(debugfp, "%srows: %u cols: %u rpx: %u cpx: %u (%dx%d)\n",
          indent, rows, cols, ti->cellpixy, ti->cellpixx, rows * ti->cellpixy, cols * ti->cellpixx);
  if(!ti->pixel_query_done){
    fprintf(debugfp, "%sno bitmap graphics information yet\n", indent);
  }else{
    if(!ti->bitmap_supported){
      fprintf(debugfp, "%sdidn't detect bitmap graphics support\n", indent);
    }else if(ti->sixel_maxy || ti->color_registers){
      fprintf(debugfp, "%smax sixel size: %dx%d colorregs: %u\n",
              indent, ti->sixel_maxy, ti->sixel_maxx, ti->color_registers);
    }else{
      fprintf(debugfp, "%sRGBA pixel graphics supported\n", indent);
    }
  }
  fprintf(debugfp, "%sUTF8: %c quad: %c sex: %c braille: %c images: %c videos: %c\n",
          indent, capbool(ti->utf8), capbool(ti->quadrants),
          capbool(ti->sextants), capbool(ti->braille),
          capbool(images), capbool(videos));
  if(ti->bg_collides_default){
    fprintf(debugfp, "%sbackground of 0x%06lx is considered transparent\n", indent, ti->bg_collides_default & 0xfffffful);
  }else{
    fprintf(debugfp, "%sbackground isn't interpreted as transparent\n", indent);
  }
  fprintf(debugfp, "%scup: %c vpa: %c hpa: %c\n",
          indent, capyn(get_escape(ti, ESCAPE_CUP)), capyn(ti->vpa), capyn(ti->hpa));
}

void notcurses_debug_caps(const notcurses* nc, FILE* debugfp){
  int rows, cols;
  notcurses_stddim_yx_const(nc, &rows, &cols);
  bool images, videos;
  images = notcurses_canopen_images(nc);
  videos = notcurses_canopen_videos(nc);
  tinfo_debug_caps(&nc->tcache, debugfp, rows, cols, images, videos);
}

static void
ncpile_debug(const ncpile* p, FILE* debugfp){
  fprintf(debugfp, "  ************************* %16p pile ****************************\n", p);
  const ncplane* n = p->top;
  const ncplane* prev = NULL;
  int planeidx = 0;
  while(n){
    fprintf(debugfp, "%04d off y: %3d x: %3d geom y: %3d x: %3d curs y: %3d x: %3d %p %.4s\n",
            planeidx, n->absy, n->absx, n->leny, n->lenx, n->y, n->x, n, n->name);
    if(n->boundto || n->bnext || n->bprev || n->blist){
      fprintf(debugfp, " bound %p ← %p → %p binds %p\n",
              n->boundto, n->bprev, n->bnext, n->blist);
    }
    if(n->bprev && (*n->bprev != n)){
      fprintf(stderr, " WARNING: expected *->bprev %p, got %p\n", n, *n->bprev);
    }
    if(n->above != prev){
      fprintf(stderr, " WARNING: expected ->above %p, got %p\n", prev, n->above);
    }
    if(ncplane_pile_const(n) != p){
      fprintf(stderr, " WARNING: expected pile %p, got %p\n", p, ncplane_pile_const(n));
    }
    prev = n;
    n = n->below;
    ++planeidx;
  }
  if(p->bottom != prev){
    fprintf(stderr, " WARNING: expected ->bottom %p, got %p\n", prev, p->bottom);
  }
}

void notcurses_debug(const notcurses* nc, FILE* debugfp){
  const ncpile* p = ncplane_pile(nc->stdplane);
  fprintf(debugfp, " -------------------------- notcurses debug state -----------------------------\n");
  const ncpile* p0 = p;
  do{
    ncpile_debug(p0, debugfp);
    const ncpile* prev = p0;
    p0 = p0->next;
    if(p0->prev != prev){
      fprintf(stderr, "WARNING: expected ->prev %p, got %p\n", prev, p0->prev);
    }
  }while(p != p0);
  fprintf(debugfp, " ______________________________________________________________________________\n");
}
