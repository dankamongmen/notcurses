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
braille_viz(ncplane* n, wchar_t l, const wchar_t* egcs, wchar_t r,
            const char* indent, wchar_t suit,
            const wchar_t* bounds, wchar_t r8, wchar_t l8){
  ncplane_printf(n, "%s%lc", indent, l);
  for(int i = 0 ; i < 64 ; ++i){
    if(ncplane_putwc(n, egcs[i]) <= 0){
      ncplane_putchar(n, ' ');
    }
  }
  ncplane_putwc(n, r);
  ncplane_set_bg_rgb(n, 0x0);
  ncplane_putwc(n, suit);
  ncplane_putwc(n, bounds[0]);
  if(ncplane_putwc(n, r8) <= 0){
    ncplane_putchar(n, ' ');
  }
  if(ncplane_putwc(n, l8) <= 0){
    ncplane_putchar(n, ' ');
  }
  ncplane_putwc(n, bounds[1]);
  ncplane_putchar(n, '\n');
  return 0;
}

// symbols for legacy computing
static int
legacy_viz(struct ncplane* n, const char* indent, const wchar_t* eighths,
           const wchar_t* anglesr, const wchar_t* anglesl){
  ncplane_printf(n, "%s ", indent);
  for(const wchar_t* e = eighths ; *e ; ++e){
    if(ncplane_putwc(n, *e) <= 0){
      ncplane_putchar(n, ' ');
    }
  }
  ncplane_putchar(n, ' ');
  for(const wchar_t* r = anglesr ; *r ; ++r){
    if(ncplane_putwc(n, *r) <= 0){
      ncplane_putchar(n, ' ');
    }
    if(ncplane_putwc(n, anglesl[r - anglesr]) <= 0){
      ncplane_putchar(n, ' ');
    }
    ncplane_putchar(n, ' ');
  }
  return 0;
}

static int
sex_viz(struct ncplane* n, const wchar_t* sex, wchar_t r, const wchar_t* post){
  for(int i = 0 ; i < 31 ; ++i){
    if(ncplane_putwc(n, sex[i]) <= 0){
      ncplane_putchar(n, ' ');
    }
  }
  if(ncplane_putwc(n, r) <= 0){
    ncplane_putchar(n, ' ');
  }
  ncplane_putchar(n, ' ');
  for(const wchar_t* p = post ; *p ; ++p){
    if(ncplane_putwc(n, *p) <= 0){
      ncplane_putchar(n, ' ');
    }
  }
  ncplane_putchar(n, ' ');
  return 0;
}

static void
wviz(struct ncplane* n, const wchar_t* wp){
  for(const wchar_t* w = wp ; *w ; ++w){
    if(ncplane_putwc(n, *w) <= 0){
      ncplane_putchar(n, ' ');
    }
  }
}

static void
wvizn(struct ncplane* n, const wchar_t* wp, int nnn){
  for(int nn = 0 ; nn < nnn ; ++nn){
    if(ncplane_putwc(n, wp[nn]) <= 0){
      ncplane_putchar(n, ' ');
    }
  }
}

static void
triviz(struct ncplane* n, const wchar_t* w1, const wchar_t* w2, const wchar_t* w3,
       const wchar_t* w4, const wchar_t* w5, const wchar_t* w6,
       const wchar_t* w7, const wchar_t* w8, const wchar_t* w9,
       const wchar_t* wa, const wchar_t* wb, const wchar_t* wc,
       const wchar_t* wd, const wchar_t* we, const wchar_t* wf,
       const wchar_t* w10, const wchar_t* w11, const wchar_t* w12,
       const wchar_t* w13, const wchar_t* w14){
  wvizn(n, w1, 2);
  ncplane_putchar(n, ' ');
  wvizn(n, w2, 2);
  ncplane_putchar(n, ' ');
  wvizn(n, w3, 2);
  ncplane_putchar(n, ' ');
  wvizn(n, w4, 2);
  wvizn(n, w5, 2);
  ncplane_putchar(n, ' ');
  wvizn(n, w6, 2);
  ncplane_putchar(n, ' ');
  wvizn(n, w7, 2);
  ncplane_putchar(n, ' ');
  wvizn(n, w8, 2);
  ncplane_putchar(n, ' ');
  wvizn(n, w9, 2);
  wvizn(n, wa, 1);
  ncplane_putchar(n, ' ');
  ncplane_putchar(n, ' ');
  wvizn(n, wb, 2);
  wvizn(n, wc, 1);
  ncplane_putchar(n, ' ');
  ncplane_putchar(n, ' ');
  wvizn(n, wd, 2);
  wvizn(n, we, 1);
  ncplane_putchar(n, ' ');
  ncplane_putchar(n, ' ');
  wvizn(n, wf, 2);
  wvizn(n, w10, 1);
  ncplane_putchar(n, ' ');
  ncplane_putchar(n, ' ');
  wvizn(n, w11, 2);
  wvizn(n, w12, 1);
  ncplane_putchar(n, ' ');
  wviz(n, w13);
  wviz(n, w14);
}

static void
vertviz(struct ncplane* n, wchar_t l, wchar_t li, wchar_t ri, wchar_t r){
  if(ncplane_putwc(n, l) <= 0){
    ncplane_putchar(n, ' ');
  }
  if(ncplane_putwc(n, li) <= 0){
    ncplane_putchar(n, ' ');
  }
  if(ncplane_putwc(n, ri) <= 0){
    ncplane_putchar(n, ' ');
  }
  if(ncplane_putwc(n, r) <= 0){
    ncplane_putchar(n, ' ');
  }
  ncplane_putchar(n, '\n');
}

static int
unicodedumper(struct ncplane* n, tinfo* ti, const char* indent){
  if(ti->caps.utf8){
    // all NCHALFBLOCKS are contained within NCQUADBLOCKS
    ncplane_printf(n, "%s%ls ⎧", indent, NCQUADBLOCKS);
    sex_viz(n, NCSEXBLOCKS, L'⎫', L"🯰🯱🯲🯳🯴🯵🯶🯷🯸🯹\u2157\u2158\u2159\u215a\u215b");
    vertviz(n, L'⎧', NCEIGHTHSR[0], NCEIGHTHSL[0], L'⎫');
    ncplane_printf(n, "%s╲╿╱ ◨◧ ◪◩ ◖◗     ⎩", indent);
    sex_viz(n, &NCSEXBLOCKS[32], L'⎭', L"\u00bc\u00bd\u00be\u2150\u2151\u2152\u2153\u2154\u2155\u2156\u215c\u215d\u215e\u215f\u2189");
    vertviz(n, L'⎪', NCEIGHTHSR[1], NCEIGHTHSL[1], L'⎪');
    ncplane_printf(n, "%s╾╳╼ ", indent);
    triviz(n, NCWHITESQUARESW, NCWHITECIRCLESW, NCDIAGONALSW, &NCDIAGONALSW[4],
           NCCIRCULARARCSW, NCWHITETRIANGLESW, NCSHADETRIANGLESW, NCBLACKTRIANGLESW,
           NCBOXLIGHTW, &NCBOXLIGHTW[4], NCBOXHEAVYW, &NCBOXHEAVYW[4], NCBOXROUNDW,
           &NCBOXROUNDW[4], NCBOXDOUBLEW, &NCBOXDOUBLEW[4], NCBOXOUTERW, &NCBOXOUTERW[4],
           L"▵△▹▷▿▽◃◁", NCARROWW);
    vertviz(n, L'⎪', NCEIGHTHSR[2], NCEIGHTHSL[2], L'⎪');
    ncplane_printf(n, "%s╱╽╲ ", indent);
    triviz(n, &NCWHITESQUARESW[2], &NCWHITECIRCLESW[2], &NCDIAGONALSW[2], &NCDIAGONALSW[6],
           &NCCIRCULARARCSW[2], &NCWHITETRIANGLESW[2], &NCSHADETRIANGLESW[2], &NCBLACKTRIANGLESW[2],
           &NCBOXLIGHTW[2], &NCBOXLIGHTW[5], &NCBOXHEAVYW[2], &NCBOXHEAVYW[5], &NCBOXROUNDW[2],
           &NCBOXROUNDW[5], &NCBOXDOUBLEW[2], &NCBOXDOUBLEW[5], &NCBOXOUTERW[2], &NCBOXOUTERW[5],
           L"▴⏶⯅▲▸⏵⯈▶", L"▾⏷⯆▼◂⏴⯇◀");
    vertviz(n, L'⎪', NCEIGHTHSR[3], NCEIGHTHSL[3], L'⎪');
    braille_viz(n, L'⎡', NCBRAILLEEGCS, L'⎤', indent, L'♠', L"⎨⎬", NCEIGHTHSR[4], NCEIGHTHSL[4]);
    braille_viz(n, L'⎢', &NCBRAILLEEGCS[64], L'⎥', indent, L'♥', L"⎪⎪", NCEIGHTHSR[5], NCEIGHTHSL[5]);
    braille_viz(n, L'⎢', &NCBRAILLEEGCS[128], L'⎥', indent, L'♦', L"⎪⎪", NCEIGHTHSR[6], NCEIGHTHSL[6]);
    braille_viz(n, L'⎣', &NCBRAILLEEGCS[192], L'⎦', indent, L'♣', L"⎩⎭", NCEIGHTHSR[7], NCEIGHTHSL[7]);
    legacy_viz(n, indent, L"▔🭶🭷🭸🭹🭺🭻▁", NCANGLESBR, NCANGLESBL);
    wviz(n, L"🭨🭪  ");
    wviz(n, NCDIGITSSUBW);
    ncplane_printf(n, "  ⎛");
    wviz(n, NCEIGHTHSB);
    ncplane_printf(n, " ⎞");
    ncplane_putchar(n, '\n');
    legacy_viz(n, indent, L"▏🭰🭱🭲🭳🭴🭵▕", NCANGLESTR, NCANGLESTL);
    wviz(n, L"🭪🭨  ");
    wviz(n, NCDIGITSSUPERW);
    ncplane_printf(n, "  ⎝");
    wviz(n, NCEIGHTHST);
    ncplane_printf(n, " ⎠");
    ncplane_putchar(n, '\n');
    int y, x;
    ncplane_cursor_yx(n, &y, &x);
    /*
    ncplane_printf_aligned(n, y - 9, NCALIGN_RIGHT, "⎡⎛⎞⎤");
    ncplane_printf_aligned(n, y - 8, NCALIGN_RIGHT, "⎢⎜⎟⎥");
    ncplane_printf_aligned(n, y - 7, NCALIGN_RIGHT, "⎢⎜⎟⎥");
    ncplane_printf_aligned(n, y - 6, NCALIGN_RIGHT, "⎢⎜⎟⎥");
    ncplane_printf_aligned(n, y - 5, NCALIGN_RIGHT, "⎢⎜⎟⎥");
    ncplane_printf_aligned(n, y - 4, NCALIGN_RIGHT, "⎢⎜⎟⎥");
    ncplane_printf_aligned(n, y - 3, NCALIGN_RIGHT, "⎢⎜⎟⎥");
    ncplane_printf_aligned(n, y - 2, NCALIGN_RIGHT, "⎢⎜⎟⎥");
    ncplane_printf_aligned(n, y - 1, NCALIGN_RIGHT, "⎣⎝⎠⎦");
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
    ncplane_cursor_move_yx(n, y - 11, 47);
    wviz(n, L"🯁🯂🯃https://notcurses.com");
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
    .y = y - 3,
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
  ncplane_printf(n, "%sdefbg 0x%06lx %sconsidered transparent\n", indent,
                 ti->bg_collides_default & 0xfffffful,
                 (ti->bg_collides_default & 0x01000000) ? "" : "not ");
  ncplane_set_fg_default(n);
  ncplane_set_fg_rgb(n, 0x5efa80);
  if(!ti->pixel_draw){
    ncplane_printf(n, "%sno bitmap graphics detected\n", indent);
  }else{ // we do have support; draw one
    if(ti->color_registers){
      if(ti->sixel_maxy){
        ncplane_printf(n, "%smax sixel size: %dx%d colorregs: %u\n",
                      indent, ti->sixel_maxy, ti->sixel_maxx, ti->color_registers);
      }else{
        ncplane_printf(n, "%ssixel colorregs: %u\n", indent, ti->color_registers);
      }
    }else if(ti->linux_fb_fd >= 0){
      ncplane_printf(n, "%sframebuffer graphics supported\n", indent);
    }else if(ti->pixel_move == NULL){
      ncplane_printf(n, "%siTerm2 graphics support\n", indent);
    }else if(ti->sixel_maxy_pristine){
      ncplane_printf(n, "%srgba pixel graphics support\n", indent);
    }else{
      ncplane_printf(n, "%srgba pixel animation support\n", indent);
    }
  }
  /*
  ncplane_putstr(n, "\U0001F918");
  ncplane_putstr(n, "\U0001F918\u200d\U0001F3FB");
  ncplane_putstr(n, "\U0001F918\u200d\U0001F3FC");
  ncplane_putstr(n, "\U0001F918\u200d\U0001F3FD");
  ncplane_putstr(n, "\U0001F918\u200d\U0001F3FE");
  ncplane_putstr(n, "\U0001F918\u200d\U0001F3FF");
  */
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
  tinfo_debug_cap(n, "video", notcurses_canopen_videos(nc), '\n');
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
  char* path = prefix_data("notcurses.png");
  if(path){
    display_logo(&nc->tcache, stdn, path);
    free(path);
  }
  if(notcurses_render(nc)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  return notcurses_stop(nc) ? EXIT_FAILURE : EXIT_SUCCESS;
}
