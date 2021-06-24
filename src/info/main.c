#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <notcurses/notcurses.h>
#include "internal.h" // internal headers

static inline wchar_t
capbool(const tinfo* ti, bool cap){
  if(ti->caps.utf8){
    return cap ? L'✓' : L'✖';
  }else{
    return cap ? '+' : '-';
  }
}

static int
unicodedumper(const struct notcurses* nc, struct ncplane* n, tinfo* ti, const char* indent){
  ncplane_printf(n, "%sutf8%lc quad%lc sex%lc braille%lc images%lc videos%lc\n",
                 indent,
                 capbool(ti, ti->caps.utf8),
                 capbool(ti, ti->caps.quadrants),
                 capbool(ti, ti->caps.sextants),
                 capbool(ti, ti->caps.braille),
                 capbool(ti, notcurses_canopen_images(nc)),
                 capbool(ti, notcurses_canopen_videos(nc)));
  if(ti->caps.utf8){
    ncplane_printf(n, " {%ls} {%ls} ⎧%.122ls⎫        ⎧█ ⎫ 🯰🯱\n",
                   NCHALFBLOCKS, NCQUADBLOCKS, NCSEXBLOCKS);
    ncplane_printf(n, "                           ⎩%ls⎭        ⎪🮋▏⎪ 🯲🯳\n",
                   NCSEXBLOCKS + 32);
    ncplane_printf(n, " ⎧%.6ls%.3ls⎫ ⎧%.6ls%.3ls⎫ ⎧%.6ls%.3ls⎫ ⎧%.6ls%.3ls⎫                                            ⎪🮊▎⎪ 🯴🯵\n",
                   NCBOXLIGHTW, NCBOXLIGHTW + 4,
                   NCBOXHEAVYW, NCBOXHEAVYW + 4,
                   NCBOXROUNDW, NCBOXROUNDW + 4,
                   NCBOXDOUBLEW, NCBOXDOUBLEW + 4);
    ncplane_printf(n, " ⎩%.6ls%.3ls⎭ ⎩%.6ls%.3ls⎭ ⎩%.6ls%.3ls⎭ ⎩%.6ls%.3ls⎭                                            ⎪🮉▍⎪ 🯶🯷\n",
                   NCBOXLIGHTW + 2, NCBOXLIGHTW + 5,
                   NCBOXHEAVYW + 2, NCBOXHEAVYW + 5,
                   NCBOXROUNDW + 2, NCBOXROUNDW + 5,
                   NCBOXDOUBLEW + 2, NCBOXDOUBLEW + 5);
    ncplane_printf(n, " ⎡%.192ls⎤ ⎨▐▌⎬ 🯸🯹\n", NCBRAILLEEGCS);
    ncplane_printf(n, " ⎢%.192ls⎥ ⎪🮈▋⎪\n", NCBRAILLEEGCS + 64);
    ncplane_printf(n, " ⎢%.192ls⎥ ⎪🮇▊⎪\n", NCBRAILLEEGCS + 128);
    ncplane_printf(n, " ⎣%.192ls⎦ ⎪▕▉⎪\n", NCBRAILLEEGCS + 192);
    ncplane_printf(n, "  ⎛%ls⎞ ▔🭶🭷🭸🭹🭺🭻▁ 🭁 🭂 🭃 🭄 🭅 🭆 🭑 🭐 🭏 🭎 🭍 🭌 🭆🭑 🭄🭏 🭅🭐 🭃🭎 🭂🭍 🭁🭌 🭨🭪 ⎩ █⎭\n",
                   NCEIGHTHSBOTTOM);
    ncplane_printf(n, "  ⎝%ls⎠ ▏🭰🭱🭲🭳🭴🭵▕ 🭒 🭓 🭔 🭕 🭖 🭧 🭜 🭟 🭠 🭡 🭞 🭝 🭧🭜 🭕🭠 🭖🭡 🭔🭟 🭓🭞 🭒🭝 🭪🭨       \n",
                   NCEIGHTSTOP);
  }
  return 0;
}

static void
tinfo_debug_bitmaps(struct ncplane* n, const tinfo* ti, const char* indent){
  if(!ti->pixel_draw){
    ncplane_printf(n, "%sdidn't detect bitmap graphics support\n", indent);
  }else if(ti->sixel_maxy){
    ncplane_printf(n, "%smax sixel size: %dx%d colorregs: %u\n",
                   indent, ti->sixel_maxy, ti->sixel_maxx, ti->color_registers);
  }else if(ti->color_registers){
    ncplane_printf(n, "%ssixel colorregs: %u\n", indent, ti->color_registers);
  }else{
    ncplane_printf(n, "%srgba pixel graphics supported\n", indent);
  }
}

static void
tinfo_debug_style(struct ncplane* n, const char* name, int style){
  ncplane_set_styles(n, style);
  ncplane_putstr(n, name);
  ncplane_set_styles(n, NCSTYLE_NONE);
}

static inline wchar_t
capyn(const tinfo* ti, const char* cap){
  return capbool(ti, cap);
}

static void
tinfo_debug_caps(struct ncplane* n, const tinfo* ti, const char* indent){
  ncplane_printf(n, "%srgb%lc ccc%lc af%lc ab%lc appsync%lc u7%lc cup%lc vpa%lc hpa%lc sgr%lc sgr0%lc op%lc fgop%lc bgop%lc\n",
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
  ncplane_printf(n, "%sbackground of 0x%06lx is %sconsidered transparent\n", indent, ti->bg_collides_default & 0xfffffful,
                   (ti->bg_collides_default & 0x01000000) ? "" : "not ");
}

static void
tinfo_debug_styles(struct ncplane* n, const char* indent){
  ncplane_putstr(n, indent);
  tinfo_debug_style(n, "bold", NCSTYLE_BOLD);
  ncplane_putchar(n, ' ');
  tinfo_debug_style(n, "ital", NCSTYLE_ITALIC);
  ncplane_putchar(n, ' ');
  tinfo_debug_style(n, "struck", NCSTYLE_STRUCK);
  ncplane_putchar(n, ' ');
  tinfo_debug_style(n, "ucurl", NCSTYLE_UNDERCURL);
  ncplane_putchar(n, ' ');
  tinfo_debug_style(n, "uline", NCSTYLE_UNDERLINE);
  ncplane_putchar(n, '\n');
}

// we should probably change this up to use regular good ol'
// notcurses_render() without the alternate screen, no?
int main(void){
  notcurses_options nopts = {
    .flags = NCOPTION_NO_ALTERNATE_SCREEN | NCOPTION_PRESERVE_CURSOR,
  };
  struct notcurses* nc = notcurses_init(&nopts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  const char indent[] = " ";
  struct ncplane* stdn = notcurses_stdplane(nc);
  // FIXME want cursor wherever it was
  // FIXME want scrolling on standard plane
  tinfo_debug_caps(stdn, &nc->tcache, indent);
  tinfo_debug_styles(stdn, indent);
  tinfo_debug_bitmaps(stdn, &nc->tcache, indent);
  unicodedumper(nc, stdn, &nc->tcache, indent);
  if(notcurses_render(nc)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  // FIXME want cursor wherever it ought be
  return notcurses_stop(nc) ? EXIT_FAILURE : EXIT_SUCCESS;
}
