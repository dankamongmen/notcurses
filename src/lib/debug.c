#include "internal.h"

int loglevel = NCLOGLEVEL_SILENT;

static inline char
capyn(const char* cap){
  return cap ? 'y' : 'n';
}

static inline char
capbool(bool cap){
  return cap ? 'y' : 'n';
}

static void
tinfo_debug_uline(const tinfo* ti, FILE* debugfp){
  const char* smul = get_escape(ti, ESCAPE_SMUL);
  if(smul){
    term_emit(smul, debugfp, false);
  }
  fprintf(debugfp, "uline");
  if(smul){
    const char* rmul = get_escape(ti, ESCAPE_RMUL);
    term_emit(rmul, debugfp, false);
  }
}

static void
tinfo_debug_ucurl(const tinfo* ti, FILE* debugfp){
  const char* smulx = get_escape(ti, ESCAPE_SMULX);
  if(smulx){
    term_emit(smulx, debugfp, false);
  }
  fprintf(debugfp, "ucurl");
  if(smulx){
    const char* smulnox = get_escape(ti, ESCAPE_SMULNOX);
    term_emit(smulnox, debugfp, false);
  }
}

static void
tinfo_debug_styles(const tinfo* ti, FILE* debugfp, const char* indent){
  fprintf(debugfp, "%s", indent);
  tinfo_debug_ucurl(ti, debugfp);
  fprintf(debugfp, " ");
  tinfo_debug_uline(ti, debugfp);
  fprintf(debugfp, "\n");
}

static void
tinfo_debug_caps(const tinfo* ti, FILE* debugfp, int rows, int cols,
                 unsigned images, unsigned videos){
  const char indent[] = " ";
  fprintf(debugfp, "%scolors: %u rgb: %c ccc: %c setaf: %c setab: %c app-sync: %c\n",
          indent, ti->caps.colors, capbool(ti->caps.rgb), capbool(ti->caps.can_change_colors),
          capyn(get_escape(ti, ESCAPE_SETAF)),
          capyn(get_escape(ti, ESCAPE_SETAB)),
          capyn(get_escape(ti, ESCAPE_BSU)));
  fprintf(debugfp, "%ssgr: %c sgr0: %c op: %c fgop: %c bgop: %c\n",
          indent, capyn(get_escape(ti, ESCAPE_SGR)),
                  capyn(get_escape(ti, ESCAPE_SGR0)),
                  capyn(get_escape(ti, ESCAPE_OP)),
                  capyn(get_escape(ti, ESCAPE_FGOP)),
                  capyn(get_escape(ti, ESCAPE_BGOP)));
  fprintf(debugfp, "%srows: %u cols: %u rpx: %u cpx: %u (%dx%d)\n",
          indent, rows, cols, ti->cellpixy, ti->cellpixx, rows * ti->cellpixy, cols * ti->cellpixx);
  if(!ti->pixel_draw){
    fprintf(debugfp, "%sdidn't detect bitmap graphics support\n", indent);
  }else if(ti->sixel_maxy){
    fprintf(debugfp, "%smax sixel size: %dx%d colorregs: %u\n",
            indent, ti->sixel_maxy, ti->sixel_maxx, ti->color_registers);
  }else if(ti->color_registers){
    fprintf(debugfp, "%ssixel colorregs: %u\n", indent, ti->color_registers);
  }else{
    fprintf(debugfp, "%srgba pixel graphics supported\n", indent);
  }
  tinfo_debug_styles(ti, debugfp, indent);
  fprintf(debugfp, "%sutf8: %c quad: %c sex: %c braille: %c images: %c videos: %c\n",
          indent, capbool(ti->caps.utf8), capbool(ti->caps.quadrants),
          capbool(ti->caps.sextants), capbool(ti->caps.braille),
          capbool(images), capbool(videos));
  if(ti->caps.utf8){
    fprintf(debugfp, "%s{%ls} {%ls} ⎧%.122ls⎫        ⎧█ ⎫ 🯰🯱\n", indent,
            get_blitter_egcs(NCBLIT_2x1), get_blitter_egcs(NCBLIT_2x2),
            get_blitter_egcs(NCBLIT_3x2));
    fprintf(debugfp, "%s                          ⎩%ls⎭        ⎪🮋▏⎪ 🯲🯳\n", indent,
            get_blitter_egcs(NCBLIT_3x2) + 32);
    fprintf(debugfp, "%s⎧%.6ls%.3ls⎫ ⎧%.6ls%.3ls⎫ ⎧%.6ls%.3ls⎫ ⎧%.6ls%.3ls⎫                                            ⎪🮊▎⎪ 🯴🯵\n", indent,
            NCBOXLIGHTW, NCBOXLIGHTW + 4,
            NCBOXHEAVYW, NCBOXHEAVYW + 4,
            NCBOXROUNDW, NCBOXROUNDW + 4,
            NCBOXDOUBLEW, NCBOXDOUBLEW + 4);
    fprintf(debugfp, "%s⎩%.6ls%.3ls⎭ ⎩%.6ls%.3ls⎭ ⎩%.6ls%.3ls⎭ ⎩%.6ls%.3ls⎭                                            ⎪🮉▍⎪ 🯶🯷\n", indent,
            NCBOXLIGHTW + 2, NCBOXLIGHTW + 5,
            NCBOXHEAVYW + 2, NCBOXHEAVYW + 5,
            NCBOXROUNDW + 2, NCBOXROUNDW + 5,
            NCBOXDOUBLEW + 2, NCBOXDOUBLEW + 5);
    fprintf(debugfp, "%s⎡%.192ls⎤ ⎨▐▌⎬ 🯸🯹\n", indent,
            get_blitter_egcs(NCBLIT_BRAILLE));
    fprintf(debugfp, "%s⎢%.192ls⎥ ⎪🮈▋⎪\n", indent,
            get_blitter_egcs(NCBLIT_BRAILLE) + 64);
    fprintf(debugfp, "%s⎢%.192ls⎥ ⎪🮇▊⎪\n", indent,
            get_blitter_egcs(NCBLIT_BRAILLE) + 128);
    fprintf(debugfp, "%s⎣%.192ls⎦ ⎪▕▉⎪\n", indent,
            get_blitter_egcs(NCBLIT_BRAILLE) + 192);
    fprintf(debugfp, "%s ⎛%ls⎞ ▔🭶🭷🭸🭹🭺🭻▁ 🭁 🭂 🭃 🭄 🭅 🭆 🭑 🭐 🭏 🭎 🭍 🭌 🭆🭑 🭄🭏 🭅🭐 🭃🭎 🭂🭍 🭁🭌 🭨🭪 ⎩ █⎭\n", indent, get_blitter_egcs(NCBLIT_8x1));
    fprintf(debugfp, "%s ⎝%s⎠ ▏🭰🭱🭲🭳🭴🭵▕ 🭒 🭓 🭔 🭕 🭖 🭧 🭜 🭟 🭠 🭡 🭞 🭝 🭧🭜 🭕🭠 🭖🭡 🭔🭟 🭓🭞 🭒🭝 🭪🭨       \n", indent, "█🮆🮅🮄▀🮃🮂▔ ");
  }
  fprintf(debugfp, "%sbackground of 0x%06lx is %sconsidered transparent\n", indent, ti->bg_collides_default & 0xfffffful,
                   (ti->bg_collides_default & 0x01000000) ? "" : "not ");
  fprintf(debugfp, "%scup: %c vpa: %c hpa: %c\n",
          indent, capyn(get_escape(ti, ESCAPE_CUP)),
                  capyn(get_escape(ti, ESCAPE_VPA)),
                  capyn(get_escape(ti, ESCAPE_HPA)));
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
