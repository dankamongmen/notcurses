#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <notcurses/notcurses.h>
#include "internal.h" // internal headers

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

static inline wchar_t
capbool(const tinfo* ti, bool cap){
  if(ti->caps.utf8){
    return cap ? L'✓' : L'✖';
  }else{
    return cap ? '+' : '-';
  }
}

static int
unicodedumper(const struct notcurses* nc, FILE* fp, tinfo* ti, const char* indent){
  fprintf(fp, "%sutf8%lc quad%lc sex%lc braille%lc images%lc videos%lc\n",
          indent,
          capbool(ti, ti->caps.utf8),
          capbool(ti, ti->caps.quadrants),
          capbool(ti, ti->caps.sextants),
          capbool(ti, ti->caps.braille),
          capbool(ti, notcurses_canopen_images(nc)),
          capbool(ti, notcurses_canopen_videos(nc)));
  if(ti->caps.utf8){
    fprintf(fp, " {%ls} {%ls} ⎧%.122ls⎫        ⎧█ ⎫ 🯰🯱\n",
            NCHALFBLOCKS, NCQUADBLOCKS, NCSEXBLOCKS);
    fprintf(fp, "                           ⎩%ls⎭        ⎪🮋▏⎪ 🯲🯳\n",
            NCSEXBLOCKS + 32);
    fprintf(fp, " ⎧%.6ls%.3ls⎫ ⎧%.6ls%.3ls⎫ ⎧%.6ls%.3ls⎫ ⎧%.6ls%.3ls⎫                                            ⎪🮊▎⎪ 🯴🯵\n",
            NCBOXLIGHTW, NCBOXLIGHTW + 4,
            NCBOXHEAVYW, NCBOXHEAVYW + 4,
            NCBOXROUNDW, NCBOXROUNDW + 4,
            NCBOXDOUBLEW, NCBOXDOUBLEW + 4);
    fprintf(fp, " ⎩%.6ls%.3ls⎭ ⎩%.6ls%.3ls⎭ ⎩%.6ls%.3ls⎭ ⎩%.6ls%.3ls⎭                                            ⎪🮉▍⎪ 🯶🯷\n",
            NCBOXLIGHTW + 2, NCBOXLIGHTW + 5,
            NCBOXHEAVYW + 2, NCBOXHEAVYW + 5,
            NCBOXROUNDW + 2, NCBOXROUNDW + 5,
            NCBOXDOUBLEW + 2, NCBOXDOUBLEW + 5);
    fprintf(fp, " ⎡%.192ls⎤ ⎨▐▌⎬ 🯸🯹\n", NCBRAILLEEGCS);
    fprintf(fp, " ⎢%.192ls⎥ ⎪🮈▋⎪\n", NCBRAILLEEGCS + 64);
    fprintf(fp, " ⎢%.192ls⎥ ⎪🮇▊⎪\n", NCBRAILLEEGCS + 128);
    fprintf(fp, " ⎣%.192ls⎦ ⎪▕▉⎪\n", NCBRAILLEEGCS + 192);
    fprintf(fp, "  ⎛%ls⎞ ▔🭶🭷🭸🭹🭺🭻▁ 🭁 🭂 🭃 🭄 🭅 🭆 🭑 🭐 🭏 🭎 🭍 🭌 🭆🭑 🭄🭏 🭅🭐 🭃🭎 🭂🭍 🭁🭌 🭨🭪 ⎩ █⎭\n",
            NCEIGHTHSBOTTOM);
    fprintf(fp, "  ⎝%ls⎠ ▏🭰🭱🭲🭳🭴🭵▕ 🭒 🭓 🭔 🭕 🭖 🭧 🭜 🭟 🭠 🭡 🭞 🭝 🭧🭜 🭕🭠 🭖🭡 🭔🭟 🭓🭞 🭒🭝 🭪🭨       \n",
            NCEIGHTSTOP);
  }
  return 0;
}

static void
tinfo_debug_bitmaps(const tinfo* ti, FILE* fp, const char* indent){
  if(!ti->pixel_draw){
    fprintf(fp, "%sdidn't detect bitmap graphics support\n", indent);
  }else if(ti->sixel_maxy){
    fprintf(fp, "%smax sixel size: %dx%d colorregs: %u\n",
            indent, ti->sixel_maxy, ti->sixel_maxx, ti->color_registers);
  }else if(ti->color_registers){
    fprintf(fp, "%ssixel colorregs: %u\n", indent, ti->color_registers);
  }else{
    fprintf(fp, "%srgba pixel graphics supported\n", indent);
  }
}

static void
tinfo_debug_style(const tinfo* ti, FILE* fp, const char* name, int esc, int deesc){
  const char* code = get_escape(ti, esc);
  if(code){
    term_emit(code, fp, false);
  }
  fprintf(fp, "%s", name);
  if(code){
    code = get_escape(ti, deesc);
    term_emit(code, fp, false);
  }
}

static inline wchar_t
capyn(const tinfo* ti, const char* cap){
  return capbool(ti, cap);
}

static void
tinfo_debug_caps(const tinfo* ti, FILE* debugfp, const char* indent){
  fprintf(debugfp, "%srgb%lc ccc%lc af%lc ab%lc appsync%lc u7%lc cup%lc vpa%lc hpa%lc sgr%lc sgr0%lc op%lc fgop%lc bgop%lc\n",
          indent,
          capbool(ti, ti->caps.rgb),
          capbool(ti, ti->caps.can_change_colors),
          capyn(ti, get_escape(ti, ESCAPE_SETAF)),
          capyn(ti, get_escape(ti, ESCAPE_SETAB)),
          capyn(ti, get_escape(ti, ESCAPE_BSU)),
          capyn(ti, get_escape(ti, ESCAPE_DSRCPR)),
          capyn(ti, get_escape(ti, ESCAPE_CUP)),
          capyn(ti, get_escape(ti, ESCAPE_VPA)),
          capyn(ti, get_escape(ti, ESCAPE_HPA)),
          capyn(ti, get_escape(ti, ESCAPE_SGR)),
          capyn(ti, get_escape(ti, ESCAPE_SGR0)),
          capyn(ti, get_escape(ti, ESCAPE_OP)),
          capyn(ti, get_escape(ti, ESCAPE_FGOP)),
          capyn(ti, get_escape(ti, ESCAPE_BGOP)));
  fprintf(debugfp, "%sbackground of 0x%06lx is %sconsidered transparent\n", indent, ti->bg_collides_default & 0xfffffful,
                   (ti->bg_collides_default & 0x01000000) ? "" : "not ");
}

static void
tinfo_debug_styles(const tinfo* ti, FILE* debugfp, const char* indent){
  fprintf(debugfp, "%s", indent);
  tinfo_debug_style(ti, debugfp, "bold", ESCAPE_BOLD, ESCAPE_SGR0);
  fprintf(debugfp, " ");
  tinfo_debug_style(ti, debugfp, "ital", ESCAPE_SITM, ESCAPE_RITM);
  fprintf(debugfp, " ");
  tinfo_debug_style(ti, debugfp, "struck", ESCAPE_SMXX, ESCAPE_RMXX);
  fprintf(debugfp, " ");
  tinfo_debug_style(ti, debugfp, "ucurl", ESCAPE_SMULX, ESCAPE_SMULNOX);
  fprintf(debugfp, " ");
  tinfo_debug_style(ti, debugfp, "uline", ESCAPE_SMUL, ESCAPE_RMUL);
  fprintf(debugfp, "\n");
}

int main(void){
  char* mbuf = NULL;
  size_t len = 0;
  FILE* mstream;
  if((mstream = open_memstream(&mbuf, &len)) == NULL){
    return EXIT_FAILURE;
  }
  if(fprintf(mstream, "\n") != 1){
    return EXIT_FAILURE;
  }
  notcurses_options nopts = {
    .flags = NCOPTION_NO_ALTERNATE_SCREEN,
  };
  struct notcurses* nc = notcurses_init(&nopts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  const char indent[] = " ";
  tinfo_debug_caps(&nc->tcache, mstream, indent);
  tinfo_debug_styles(&nc->tcache, mstream, indent);
  tinfo_debug_bitmaps(&nc->tcache, mstream, indent);
  unicodedumper(nc, mstream, &nc->tcache, indent);
  if(fclose(mstream)){
    notcurses_stop(nc);
    fprintf(stderr, "Error closing memstream after %zuB\n", len);
    return EXIT_FAILURE;
  }
  if(writen(fileno(stdout), mbuf, len) < 0){
    notcurses_stop(nc);
    fprintf(stderr, "Error writing %zuB memstream\n", len);
    return EXIT_FAILURE;
  }
  free(mbuf);
  return notcurses_stop(nc) ? EXIT_FAILURE : EXIT_SUCCESS;
}
