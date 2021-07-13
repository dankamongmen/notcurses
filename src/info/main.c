#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <notcurses/notcurses.h>
#include "internal.h" // internal headers

static inline wchar_t
capboolbool(unsigned utf8, bool cap){
  // FIXME these fancy glyphs aren't reliable enough yet (they're not
  // usually present on the linux console, for instance) for such an
  // essential, must-work-everywhere tool.
  /*if(utf8){
    return cap ? L'✓' : L'✖';
  }else{
    return cap ? '+' : '-';
  }*/
  (void)utf8;
  return cap ? '+' : '-';
}

static void
tinfo_debug_cap(struct ncplane* n, const char* name, bool yn, char ch){
  if(!yn){
    ncplane_set_styles(n, NCSTYLE_ITALIC);
  }
  ncplane_putstr(n, name);
  ncplane_set_styles(n, NCSTYLE_BOLD);
  ncplane_putwc(n, capboolbool(notcurses_canutf8(ncplane_notcurses(n)), yn));
  ncplane_set_styles(n, NCSTYLE_NONE);
  ncplane_putchar(n, ch);
}

static void
tinfo_debug_style(struct ncplane* n, const char* name, int style, char ch){
  unsigned support = notcurses_supported_styles(ncplane_notcurses(n)) & style;
  if(!support){
    ncplane_set_styles(n, NCSTYLE_ITALIC);
  }
  ncplane_set_styles(n, style);
  ncplane_putstr(n, name);
  ncplane_set_styles(n, NCSTYLE_BOLD);
  ncplane_putwc(n, capboolbool(notcurses_canutf8(ncplane_notcurses(n)), support));
  ncplane_set_styles(n, NCSTYLE_NONE);
  ncplane_putchar(n, ch);
}

static int
braille_viz(ncplane* n, const char* l, const wchar_t* egcs, const char* r, const char* indent){
  ncplane_printf(n, "%s%s", indent, l);
  for(int i = 0 ; i < 64 ; ++i){
    if(ncplane_putwc(n, egcs[i]) < 0){
      ncplane_putwc(n, L' ');
    }
  }
  ncplane_printf(n, "%s ", r);
  return 0;
}

static int
unicodedumper(struct ncplane* n, tinfo* ti, const char* indent){
  if(ti->caps.utf8){
    // all NCHALFBLOCKS are contained within NCQUADBLOCKS
    ncplane_printf(n, "%s%ls ⎧%.122ls⎫ 🯰🯱🯲🯳🯴🯵🯶🯷🯸🯹\u2157\u2158\u2159\u215a\u215b ⎧%lc%lc⎫",
                   indent, NCQUADBLOCKS, NCSEXBLOCKS,
                   NCEIGHTHSR[0], NCEIGHTHSL[0]);
    ncplane_putchar(n, '\n');
    ncplane_printf(n, "%s╲╿╱ ◨◧ ◪◩ ◖◗     ⎩%ls⎭ \u00bc\u00bd\u00be\u2150\u2151\u2152\u2153\u2154\u2155\u2156\u215c\u215d\u215e\u215f\u2189 ⎪%lc%lc⎪",
                   indent, NCSEXBLOCKS + 32,
                   NCEIGHTHSR[1], NCEIGHTHSL[1]);
    ncplane_putchar(n, '\n');
    ncplane_printf(n, "%s╾╳╼ %.6ls %.6ls %.8ls%.8ls %.6ls %.6ls %.8ls %.6ls %.6ls%.3ls  %.6ls%.3ls  %.6ls%.3ls  %.6ls%.3ls  %.8ls%.4ls ▵△▹▷▿▽◃◁%.32ls⎪%lc%lc⎪",
                   indent,
                   NCWHITESQUARESW,
                   NCWHITECIRCLESW,
                   NCDIAGONALSW,
                   NCDIAGONALSW + 4,
                   NCCIRCULARARCSW,
                   NCWHITETRIANGLESW,
                   NCSHADETRIANGLESW,
                   NCBLACKTRIANGLESW,
                   NCBOXLIGHTW, NCBOXLIGHTW + 4,
                   NCBOXHEAVYW, NCBOXHEAVYW + 4,
                   NCBOXROUNDW, NCBOXROUNDW + 4,
                   NCBOXDOUBLEW, NCBOXDOUBLEW + 4,
                   NCBOXOUTERW, NCBOXOUTERW + 4,
                   NCARROWW,
                   NCEIGHTHSR[2], NCEIGHTHSL[2]);
    ncplane_putchar(n, '\n');
    ncplane_printf(n, "%s╱╽╲ %.6ls %.6ls %.8ls%.8ls %.6ls %.6ls %.8ls %.6ls %.6ls%.3ls  %.6ls%.3ls  %.6ls%.3ls  %.6ls%.3ls  %.8ls%.4ls ▴⏶⯅▲▸⏵⯈▶▾⏷⯆▼◂⏴⯇◀⎪%lc%lc⎪",
                   indent,
                   NCWHITESQUARESW + 2,
                   NCWHITECIRCLESW + 2,
                   NCDIAGONALSW + 2,
                   NCDIAGONALSW + 6,
                   NCCIRCULARARCSW + 2,
                   NCWHITETRIANGLESW + 2,
                   NCSHADETRIANGLESW + 2,
                   NCBLACKTRIANGLESW + 2,
                   NCBOXLIGHTW + 2, NCBOXLIGHTW + 5,
                   NCBOXHEAVYW + 2, NCBOXHEAVYW + 5,
                   NCBOXROUNDW + 2, NCBOXROUNDW + 5,
                   NCBOXDOUBLEW + 2, NCBOXDOUBLEW + 5,
                   NCBOXOUTERW + 2, NCBOXOUTERW + 5,
                   NCEIGHTHSR[3], NCEIGHTHSL[3]);
    ncplane_putchar(n, '\n');
    braille_viz(n, "⎡", NCBRAILLEEGCS, "⎤", indent);
    ncplane_printf(n, "⎨%lc%lc⎬", NCEIGHTHSR[4], NCEIGHTHSL[4]);
    ncplane_putchar(n, '\n');
    braille_viz(n, "⎢", NCBRAILLEEGCS + 64, "⎥", indent);
    ncplane_printf(n, "⎪%lc%lc⎪", NCEIGHTHSR[5], NCEIGHTHSL[5]);
    ncplane_putchar(n, '\n');
    braille_viz(n, "⎢",  NCBRAILLEEGCS + 128, "⎥", indent);
    ncplane_printf(n, "⎪%lc%lc⎪", NCEIGHTHSR[6], NCEIGHTHSL[6]);
    ncplane_putchar(n, '\n');
    braille_viz(n, "⎣",NCBRAILLEEGCS + 192, "⎦", indent);
    ncplane_printf(n, "⎩%lc%lc⎭", NCEIGHTHSR[7], NCEIGHTHSL[7]);
    ncplane_putchar(n, '\n');

    ncplane_printf(n, "%s ▔🭶🭷🭸🭹🭺🭻▁ %lc%lc %lc%lc %lc%lc %lc%lc %lc%lc %lc%lc %lc%lc %lc%lc %lc%lc %lc%lc %lc%lc 🭨🭪  %.30ls  ⎛%ls ⎞",
                   indent,
                   NCANGLESBR[0], NCANGLESBL[0],
                   NCANGLESBR[1], NCANGLESBL[1],
                   NCANGLESBR[2], NCANGLESBL[2],
                   NCANGLESBR[3], NCANGLESBL[3],
                   NCANGLESBR[4], NCANGLESBL[4],
                   NCANGLESBR[5], NCANGLESBL[5],
                   NCANGLESBR[6], NCANGLESBL[6],
                   NCANGLESBR[7], NCANGLESBL[7],
                   NCANGLESBR[8], NCANGLESBL[8],
                   NCANGLESBR[9], NCANGLESBL[9],
                   NCANGLESBR[10], NCANGLESBL[10],
                   NCDIGITSSUBW, NCEIGHTHSB);
    ncplane_putchar(n, '\n');
    ncplane_printf(n, "%s ▏🭰🭱🭲🭳🭴🭵▕ %lc%lc %lc%lc %lc%lc %lc%lc %lc%lc %lc%lc %lc%lc %lc%lc %lc%lc %lc%lc %lc%lc 🭪🭨  %.30ls  ⎝%ls ⎠",
                   indent,
                   NCANGLESTR[0], NCANGLESTL[0],
                   NCANGLESTR[1], NCANGLESTL[1],
                   NCANGLESTR[2], NCANGLESTL[2],
                   NCANGLESTR[3], NCANGLESTL[3],
                   NCANGLESTR[4], NCANGLESTL[4],
                   NCANGLESTR[5], NCANGLESTL[5],
                   NCANGLESTR[6], NCANGLESTL[6],
                   NCANGLESTR[7], NCANGLESTL[7],
                   NCANGLESTR[8], NCANGLESTL[8],
                   NCANGLESTR[9], NCANGLESTL[9],
                   NCANGLESTR[10], NCANGLESTL[10],
                   NCDIGITSSUPERW, NCEIGHTHST);
    ncplane_putchar(n, '\n');
    int y, x;
    ncplane_cursor_yx(n, &y, &x);
    /*
    ncplane_printf_aligned(n, y - 9, NCALIGN_RIGHT, "⎧⎡⎛⎞⎤⎫");
    ncplane_printf_aligned(n, y - 8, NCALIGN_RIGHT, "⎪⎢⎜⎟⎥⎪");
    ncplane_printf_aligned(n, y - 7, NCALIGN_RIGHT, "⎪⎢⎜⎟⎥⎪");
    ncplane_printf_aligned(n, y - 6, NCALIGN_RIGHT, "⎪⎢⎜⎟⎥⎪");
    ncplane_printf_aligned(n, y - 5, NCALIGN_RIGHT, "⎨⎢⎜⎟⎥⎬");
    ncplane_printf_aligned(n, y - 4, NCALIGN_RIGHT, "⎪⎢⎜⎟⎥⎪");
    ncplane_printf_aligned(n, y - 3, NCALIGN_RIGHT, "⎪⎢⎜⎟⎥⎪");
    ncplane_printf_aligned(n, y - 2, NCALIGN_RIGHT, "⎪⎢⎜⎟⎥⎪");
    ncplane_printf_aligned(n, y - 1, NCALIGN_RIGHT, "⎩⎣⎝⎠⎦⎭");
    */
    // the symbols for legacy computing
    ncplane_cursor_move_yx(n, y - 2, 0);
    uint32_t ul = NCCHANNEL_INITIALIZER(0x30, 0x30, 0x30);
    uint32_t lr = NCCHANNEL_INITIALIZER(0x80, 0x80, 0x80);
    ncplane_stain(n, y - 1, 70, ul, lr, ul, lr);
    // the braille
    ncplane_cursor_move_yx(n, y - 6, 0);
    ul = NCCHANNEL_INITIALIZER(0x2f, 0x25, 0x24);
    lr = NCCHANNEL_INITIALIZER(0x74, 0x25, 0x2f);
    ncplane_stain(n, y - 3, 65, ul, lr, ul, lr);
    // the sextants
    ncplane_cursor_move_yx(n, y - 10, 27);
    lr = NCCHANNEL_INITIALIZER(0x7B, 0x68, 0xEE);
    ul = NCCHANNEL_INITIALIZER(0x19, 0x19, 0x70);
    ncplane_stain(n, y - 9, 57, lr, ul, lr, ul);
    // the boxes + quadrants
    ncplane_cursor_move_yx(n, y - 10, 0);
    ncplane_stain(n, y - 7, 70, lr, ul, lr, ul);
    // the horizontal eighths
    ul = NCCHANNEL_INITIALIZER(0x60, 0x7d, 0x3b);
    lr = NCCHANNEL_INITIALIZER(0x02, 0x8a, 0x0f);
    ncplane_cursor_move_yx(n, y - 10, 67);
    ncplane_stain(n, y - 3, 70, lr, ul, lr, ul);
    // the capabilities
    ul = NCCHANNEL_INITIALIZER(0x1B, 0xb8, 0x8E);
    lr = NCCHANNEL_INITIALIZER(0x19, 0x19, 0x70);
    ncplane_cursor_move_yx(n, y - 15, 0);
    ncplane_stain(n, y - 11, 70, lr, ul, lr, ul);

    ncplane_set_fg_rgb(n, 0x00c0c0);
    ncplane_set_styles(n, NCSTYLE_BOLD);
    ncplane_putstr_yx(n, y - 11, 47, "🯁🯂🯃https://notcurses.com");
    ncplane_set_fg_default(n);
    ncplane_set_styles(n, NCSTYLE_NONE);
  }
  return 0;
}

static int
display_logo(const tinfo* ti, struct ncplane* n, const char* path){
  struct ncvisual* ncv = ncvisual_from_file(path);
  if(ncv == NULL){
    return -1;
  }
  // FIXME ought be exactly 4:1
  if(ncvisual_resize(ncv, 3 * ti->cellpixy, 24 * ti->cellpixx)){
    ncvisual_destroy(ncv);
    return -1;
  }
  int y;
  ncplane_cursor_yx(n, &y, NULL);
  struct ncvisual_options vopts = {
    .n = n,
    .y = y - 4,
    .x = 48,
    .blitter = NCBLIT_PIXEL,
    .flags = NCVISUAL_OPTION_CHILDPLANE,
  };
  struct ncplane* bitm = ncvisual_render(ncplane_notcurses(n), ncv, &vopts);
  if(bitm == NULL){
    ncvisual_destroy(ncv);
    return -1;
  }
  ncvisual_destroy(ncv);
  return 0;
}

static void
tinfo_debug_bitmaps(struct ncplane* n, const tinfo* ti, const char* indent){
  ncplane_set_fg_rgb8(n, 0xc4, 0x5a, 0xec);
  ncplane_printf(n, "%sbgcolor 0x%06lx %sconsidered transparent\n", indent, ti->bg_collides_default & 0xfffffful,
                   (ti->bg_collides_default & 0x01000000) ? "" : "not ");
  ncplane_set_fg_default(n);
  ncplane_set_fg_rgb(n, 0x5efa80);
  if(!ti->pixel_draw){
    ncplane_printf(n, "%sdidn't detect bitmap graphics support\n", indent);
  }else{ // we do have support; draw one
    if(ti->color_registers){
      if(ti->sixel_maxy){
        ncplane_printf(n, "%smax sixel size: %dx%d colorregs: %u\n",
                      indent, ti->sixel_maxy, ti->sixel_maxx, ti->color_registers);
      }else{
        ncplane_printf(n, "%ssixel colorregs: %u\n", indent, ti->color_registers);
      }
    }else if(ti->sixel_maxy_pristine){
      ncplane_printf(n, "%srgba pixel graphics supported\n", indent);
    }else{
      ncplane_printf(n, "%srgba pixel animation supported\n", indent);
    }
    char* path = prefix_data("notcurses.png");
    if(path){
      display_logo(ti, n, path);
      free(path);
    }
  }
}

static void
tinfo_debug_caps(struct ncplane* n, const tinfo* ti, const char* indent){
  ncplane_printf(n, "%s", indent);
  tinfo_debug_cap(n, "rgb", ti->caps.rgb, ' ');
  tinfo_debug_cap(n, "ccc", ti->caps.can_change_colors, ' ');
  tinfo_debug_cap(n, "af", get_escape(ti, ESCAPE_SETAF), ' ');
  tinfo_debug_cap(n, "ab", get_escape(ti, ESCAPE_SETAB), ' ');
  tinfo_debug_cap(n, "sum", get_escape(ti, ESCAPE_BSUM), ' ');
  tinfo_debug_cap(n, "u7", get_escape(ti, ESCAPE_DSRCPR), ' ');
  tinfo_debug_cap(n, "cup", get_escape(ti, ESCAPE_CUP), ' ');
  tinfo_debug_cap(n, "vpa", get_escape(ti, ESCAPE_VPA), ' ');
  tinfo_debug_cap(n, "hpa", get_escape(ti, ESCAPE_HPA), ' ');
  tinfo_debug_cap(n, "sgr0", get_escape(ti, ESCAPE_SGR0), ' ');
  tinfo_debug_cap(n, "op", get_escape(ti, ESCAPE_OP), ' ');
  tinfo_debug_cap(n, "fgop", get_escape(ti, ESCAPE_FGOP), ' ');
  tinfo_debug_cap(n, "bgop", get_escape(ti, ESCAPE_BGOP), '\n');
}

static void
tinfo_debug_styles(const notcurses* nc, struct ncplane* n, const char* indent){
  const tinfo* ti = &nc->tcache;
  ncplane_putstr(n, indent);
  tinfo_debug_style(n, "blink", NCSTYLE_BLINK, ' ');
  tinfo_debug_style(n, "bold", NCSTYLE_BOLD, ' ');
  tinfo_debug_style(n, "ital", NCSTYLE_ITALIC, ' ');
  tinfo_debug_style(n, "struck", NCSTYLE_STRUCK, ' ');
  tinfo_debug_style(n, "ucurl", NCSTYLE_UNDERCURL, ' ');
  tinfo_debug_style(n, "uline", NCSTYLE_UNDERLINE, '\n');
  ncplane_set_fg_default(n);
  ncplane_putstr(n, indent);
  tinfo_debug_cap(n, "utf8", ti->caps.utf8, ' ');
  tinfo_debug_cap(n, "quad", ti->caps.quadrants, ' ');
  tinfo_debug_cap(n, "sex", ti->caps.sextants, ' ');
  tinfo_debug_cap(n, "braille", ti->caps.braille, ' ');
  tinfo_debug_cap(n, "images", notcurses_canopen_images(nc), ' ');
  tinfo_debug_cap(n, "videos", notcurses_canopen_videos(nc), '\n');
  ncplane_set_fg_default(n);
}

static int
usage(const char* base, FILE* fp, int ret){
  fprintf(fp, "usage: %s [ -v ]\n", base);
  exit(ret);
}

// we should probably change this up to use regular good ol'
// notcurses_render() without the alternate screen, no?
int main(int argc, const char** argv){
  notcurses_options nopts = {
    .flags = NCOPTION_NO_ALTERNATE_SCREEN
             | NCOPTION_PRESERVE_CURSOR
             | NCOPTION_NO_CLEAR_BITMAPS,
    .loglevel = NCLOGLEVEL_WARNING,
  };
  if(argc > 2){
    usage(*argv, stderr, EXIT_FAILURE);
  }
  if(argc == 2){
    if(strcmp(argv[1], "-v")){
      usage(*argv, stderr, EXIT_FAILURE);
    }
    nopts.loglevel = NCLOGLEVEL_TRACE;
  }
  struct notcurses* nc = notcurses_init(&nopts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  const char indent[] = "";
  struct ncplane* stdn = notcurses_stdplane(nc);
  ncplane_set_scrolling(stdn, true);
  tinfo_debug_caps(stdn, &nc->tcache, indent);
  tinfo_debug_styles(nc, stdn, indent);
  tinfo_debug_bitmaps(stdn, &nc->tcache, indent);
  unicodedumper(stdn, &nc->tcache, indent);
  if(notcurses_render(nc)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  return notcurses_stop(nc) ? EXIT_FAILURE : EXIT_SUCCESS;
}
