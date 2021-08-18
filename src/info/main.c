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
tinfo_debug_cap(struct ncplane* n, const char* name, bool yn){
  if(!yn){
    ncplane_set_styles(n, NCSTYLE_ITALIC);
  }
  ncplane_putstr(n, name);
  ncplane_set_styles(n, NCSTYLE_BOLD);
  ncplane_putwc(n, capboolbool(notcurses_canutf8(ncplane_notcurses(n)), yn));
  ncplane_set_styles(n, NCSTYLE_NONE);
  ncplane_putchar(n, ' ');
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

static int
braille_viz(struct ncplane* n, wchar_t l, const wchar_t* egcs, wchar_t r,
            const char* indent, const wchar_t* bounds, wchar_t r8, wchar_t l8,
            const wchar_t* trailer){
  ncplane_printf(n, "%s%lc", indent, l);
  for(int i = 0 ; i < 64 ; ++i){
    if(ncplane_putwc(n, egcs[i]) <= 0){
      ncplane_putchar(n, ' ');
    }
  }
  ncplane_putwc(n, r);
  ncplane_putwc(n, bounds[0]);
  if(ncplane_putwc(n, r8) <= 0){
    ncplane_putchar(n, ' ');
  }
  if(ncplane_putwc(n, l8) <= 0){
    ncplane_putchar(n, ' ');
  }
  ncplane_putwc(n, bounds[1]);
  if(trailer){
    wviz(n, trailer);
  }
  if(ncplane_dim_x(n) > 80){
    ncplane_putchar(n, '\n');
  }
  return 0;
}

static void
finish_line(struct ncplane* n){
  int x;
  ncplane_cursor_yx(n, NULL, &x);
  while(x++ < 80){
    ncplane_putchar(n, ' ');
  }
  if(ncplane_dim_x(n) > 80){
    ncplane_putchar(n, '\n');
  }
}

static int
emoji_viz(struct ncplane* n){
  static const char emoji[] = "👾🏴🤘🚬🌍🌎🌏🥆💣🗡🔫⚗️⚛️☢️☣️🌿🎱🏧"
                              "💉💊"
                              "\U0001f574\ufe0f" // man in suit levitating
                              "📡🤻🦑🇮🇱🇦🇶"
                              "\U0001f469\u200d\U0001f52c" // woman scientist
                              "\U0001faa4" // mouse trap
                              "\U0001f6b1" // non-potable water
                              "\u270a\U0001f3ff" // type-6 raised fist
                              "\U0001f52c" // microscope
                              "\U0001f9ec" // dna double helix
                              "\U0001f3f4\u200d\u2620\ufe0f" // pirate flag
                              "\U0001f93d\U0001f3fc\u200d\u2640\ufe0f" // type-3 woman playing water polo
                              ;
  ncplane_set_bg_rgb(n, 0);
  int bytes;
  for(const char* e = emoji ; *e ; e += bytes){
    if(ncplane_putegc(n, e, &bytes) < 0){
      if(ncplane_putchar(n, ' ') < 0){
        break;
      }
    }
  }
  int x;
  ncplane_cursor_yx(n, NULL, &x);
  while(x++ < 80){
    ncplane_putchar(n, ' ');
  }
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
  for(const wchar_t* p = post ; *p ; ++p){
    if(ncplane_putwc(n, *p) <= 0){
      ncplane_putchar(n, ' ');
    }
  }
  return 0;
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
  wvizn(n, wd, 2);
  wvizn(n, we, 1);
  ncplane_putchar(n, ' ');
  ncplane_putchar(n, ' ');
  wvizn(n, wf, 2);
  wvizn(n, w10, 1);
  ncplane_putchar(n, ' ');
  wvizn(n, w11, 2);
  wvizn(n, w12, 1);
  ncplane_putchar(n, ' ');
  wviz(n, w13);
  wviz(n, w14);
}

static void
vertviz(struct ncplane* n, wchar_t l, wchar_t li, wchar_t ri, wchar_t r,
        const wchar_t* trail){
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
  wviz(n, trail);
  if(ncplane_dim_x(n) > 80){
    ncplane_putchar(n, '\n');
  }
}

static int
unicodedumper(struct ncplane* n, const char* indent){
  if(notcurses_canutf8(ncplane_notcurses_const(n))){
    // all NCHALFBLOCKS are contained within NCQUADBLOCKS
    ncplane_printf(n, "%s%ls⎧", indent, NCQUADBLOCKS);
    sex_viz(n, NCSEXBLOCKS, L'⎫', L"♠♥🯰🯱🯲🯳🯴🯵🯶🯷🯸🯹\u2157\u2158\u2159\u215a\u215b");
    vertviz(n, L'⎧', NCEIGHTHSR[0], NCEIGHTHSL[0], L'⎫', L"┌╥─╥─╥┐🭩⎛⎞");
    ncplane_printf(n, "%s╲╿╱ ◨◧ ◪◩ ◖◗ ⫷⫸ ⎩", indent);
    sex_viz(n, &NCSEXBLOCKS[32], L'⎭', L"♦♣\u00bc\u00bd\u00be\u2150\u2151\u2152\u2153\u2154\u2155\u2156\u215c\u215d\u215e\u215f\u2189");
    vertviz(n, L'⎪', NCEIGHTHSR[1], NCEIGHTHSL[1], L'⎪', L"├╜╓╫╖╙┤🭫⎜⎟");
    ncplane_printf(n, "%s╾╳╼ ", indent);
    triviz(n, NCWHITESQUARESW, NCWHITECIRCLESW, NCDIAGONALSW, &NCDIAGONALSW[4],
           NCCIRCULARARCSW, NCWHITETRIANGLESW, NCSHADETRIANGLESW, NCBLACKTRIANGLESW,
           NCBOXLIGHTW, &NCBOXLIGHTW[4], NCBOXHEAVYW, &NCBOXHEAVYW[4], NCBOXROUNDW,
           &NCBOXROUNDW[4], NCBOXDOUBLEW, &NCBOXDOUBLEW[4], NCBOXOUTERW, &NCBOXOUTERW[4],
           L"⩘▵△▹▷▿▽◃◁", NCARROWW);
    vertviz(n, L'⎪', NCEIGHTHSR[2], NCEIGHTHSL[2], L'⎪', L"├─╨╫╨─┤┇⎜⎟");
    ncplane_printf(n, "%s╱╽╲ ", indent);
    triviz(n, &NCWHITESQUARESW[2], &NCWHITECIRCLESW[2], &NCDIAGONALSW[2], &NCDIAGONALSW[6],
           &NCCIRCULARARCSW[2], &NCWHITETRIANGLESW[2], &NCSHADETRIANGLESW[2], &NCBLACKTRIANGLESW[2],
           &NCBOXLIGHTW[2], &NCBOXLIGHTW[5], &NCBOXHEAVYW[2], &NCBOXHEAVYW[5], &NCBOXROUNDW[2],
           &NCBOXROUNDW[5], &NCBOXDOUBLEW[2], &NCBOXDOUBLEW[5], &NCBOXOUTERW[2], &NCBOXOUTERW[5],
           L"⩗▴⏶⯅▲▸⏵⯈▶", L"▾⏷⯆▼◂⏴⯇◀");
    vertviz(n, L'⎪', NCEIGHTHSR[3], NCEIGHTHSL[3], L'⎪', L"╞═╤╬╤═╡┋⎜⎟");
    braille_viz(n, L'⎡', NCBRAILLEEGCS, L'⎤', indent, L"⎨⎬", NCEIGHTHSR[4], NCEIGHTHSL[4],
                L"╞╕╘╬╛╒╡┊⎜⎟");
    braille_viz(n, L'⎢', &NCBRAILLEEGCS[64], L'⎥', indent, L"⎪⎪", NCEIGHTHSR[5], NCEIGHTHSL[5],
                L"└┴─╨─┴┘╏⎝⎠");
    braille_viz(n, L'⎢', &NCBRAILLEEGCS[128], L'⎥', indent, L"⎪⎪", NCEIGHTHSR[6], NCEIGHTHSL[6],
                L"╭──╮⟬⟭╔╗≶≷");
    braille_viz(n, L'⎣', &NCBRAILLEEGCS[192], L'⎦', indent, L"⎪⎪", NCEIGHTHSR[7], NCEIGHTHSL[7],
                L"│╭╮│╔═╝║⊆⊇");
    legacy_viz(n, indent, L"▔🭶🭷🭸🭹🭺🭻▁", NCANGLESBR, NCANGLESBL);
    wviz(n, NCDIGITSSUBW);
    wviz(n, L" ⎛");
    wviz(n, NCEIGHTHSB);
    wviz(n, L" ⎞⎪🭨🭪⎪╰╯││║╔═╝⊴⊵");
    if(ncplane_dim_x(n) > 80){
      ncplane_putchar(n, '\n');
    }
    legacy_viz(n, indent, L"▏🭰🭱🭲🭳🭴🭵▕", NCANGLESTR, NCANGLESTL);
    wviz(n, NCDIGITSSUPERW);
    wviz(n, L" ⎝");
    wviz(n, NCEIGHTHST);
    wviz(n, L" ⎠⎩🭪🭨⎭⧒⧑╰╯╚╝❨❩⟃⟄");
    if(ncplane_dim_x(n) > 80){
      ncplane_putchar(n, '\n');
    }
    emoji_viz(n);
    int y, x;
    ncplane_cursor_yx(n, &y, &x);
    uint64_t ur = NCCHANNELS_INITIALIZER(0xff, 0xff, 0xff, 0x1B, 0xd8, 0x8E);
    uint64_t lr = NCCHANNELS_INITIALIZER(0xff, 0xff, 0xff, 0xdB, 0x18, 0x8E);
    uint64_t ul = NCCHANNELS_INITIALIZER(0xff, 0xff, 0xff, 0x19, 0x19, 0x70);
    uint64_t ll = NCCHANNELS_INITIALIZER(0xff, 0xff, 0xff, 0x19, 0x19, 0x70);
    ncplane_cursor_move_yx(n, y - 15, 0);
    ncplane_stain(n, y - 1, 79, ul, ur, ll, lr);
    ncplane_set_styles(n, NCSTYLE_BOLD | NCSTYLE_ITALIC);
    ncplane_cursor_move_yx(n, y - 12, 54);
    wviz(n, L"🯁🯂🯃https://notcurses.com");
    ncplane_set_styles(n, NCSTYLE_NONE);
  }
  return 0;
}

static int
display_logo(struct ncplane* n, const char* path){
  int cpixy, cpixx;
  ncplane_pixelgeom(n, NULL, NULL, &cpixy, &cpixx, NULL, NULL);
  struct ncvisual* ncv = ncvisual_from_file(path);
  if(ncv == NULL){
    return -1;
  }
  if(ncvisual_resize(ncv, 3 * cpixy, 24 * cpixx)){
    ncvisual_destroy(ncv);
    return -1;
  }
  int y;
  ncplane_cursor_yx(n, &y, NULL);
  struct ncvisual_options vopts = {
    .n = n,
    .y = y - 3,
    .x = 54,
    .blitter = NCBLIT_PIXEL,
    .flags = NCVISUAL_OPTION_CHILDPLANE | NCVISUAL_OPTION_NODEGRADE,
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
  ncplane_printf(n, "%sdefbg 0x%06lx %sconsidered transparent", indent,
                 ti->bg_collides_default & 0xfffffful,
                 (ti->bg_collides_default & 0x01000000) ? "" : "not ");
  finish_line(n);
  if(!ti->pixel_draw && !ti->pixel_draw_late){
    ncplane_printf(n, "%sno bitmap graphics detected", indent);
  }else{ // we do have support; draw one
    if(ti->color_registers){
      if(ti->sixel_maxy){
        ncplane_printf(n, "%smax sixel size: %dx%d colorregs: %u",
                      indent, ti->sixel_maxy, ti->sixel_maxx, ti->color_registers);
      }else{
        ncplane_printf(n, "%ssixel colorregs: %u", indent, ti->color_registers);
      }
    }else if(ti->pixel_draw_late){
      ncplane_printf(n, "%sframebuffer graphics supported", indent);
    }else if(ti->sixel_maxy_pristine){
      ncplane_printf(n, "%srgba pixel graphics support", indent);
    }else{
      ncplane_printf(n, "%srgba pixel animation support", indent);
    }
  }
  finish_line(n);
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
  tinfo_debug_cap(n, "af", get_escape(ti, ESCAPE_SETAF));
  tinfo_debug_cap(n, "ab", get_escape(ti, ESCAPE_SETAB));
  tinfo_debug_cap(n, "sum", get_escape(ti, ESCAPE_BSUM));
  tinfo_debug_cap(n, "cup", get_escape(ti, ESCAPE_CUP));
  tinfo_debug_cap(n, "vpa", get_escape(ti, ESCAPE_VPA));
  tinfo_debug_cap(n, "hpa", get_escape(ti, ESCAPE_HPA));
  tinfo_debug_cap(n, "sgr0", get_escape(ti, ESCAPE_SGR0));
  tinfo_debug_cap(n, "op", get_escape(ti, ESCAPE_OP));
  tinfo_debug_cap(n, "fgop", get_escape(ti, ESCAPE_FGOP));
  tinfo_debug_cap(n, "bgop", get_escape(ti, ESCAPE_BGOP));
  tinfo_debug_cap(n, "bce", ti->bce);
  finish_line(n);
}

static void
tinfo_debug_styles(const notcurses* nc, struct ncplane* n, const char* indent){
  const tinfo* ti = &nc->tcache;
  ncplane_putstr(n, indent);
  tinfo_debug_style(n, "bold", NCSTYLE_BOLD, ' ');
  tinfo_debug_style(n, "ital", NCSTYLE_ITALIC, ' ');
  tinfo_debug_style(n, "struck", NCSTYLE_STRUCK, ' ');
  tinfo_debug_style(n, "ucurl", NCSTYLE_UNDERCURL, ' ');
  tinfo_debug_style(n, "uline", NCSTYLE_UNDERLINE, ' ');
  tinfo_debug_cap(n, "u7", get_escape(ti, ESCAPE_U7));
  tinfo_debug_cap(n, "ccc", ti->caps.can_change_colors);
  tinfo_debug_cap(n, "rgb", ti->caps.rgb);
  finish_line(n);
  ncplane_putstr(n, indent);
  tinfo_debug_cap(n, "utf8", ti->caps.utf8);
  tinfo_debug_cap(n, "quad", ti->caps.quadrants);
  tinfo_debug_cap(n, "sex", ti->caps.sextants);
  tinfo_debug_cap(n, "braille", ti->caps.braille);
  tinfo_debug_cap(n, "images", notcurses_canopen_images(nc));
  tinfo_debug_cap(n, "video", notcurses_canopen_videos(nc));
  tinfo_debug_cap(n, "indn", get_escape(ti, ESCAPE_INDN));
  finish_line(n);
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
  int dimx;
  struct ncplane* stdn = notcurses_stddim_yx(nc, NULL, &dimx);
  if(dimx < 80){
    notcurses_stop(nc);
    fprintf(stderr, "This program requires at least 80 columns.\n");
    return EXIT_FAILURE;
  }
  ncplane_set_scrolling(stdn, true);
  tinfo_debug_caps(stdn, &nc->tcache, indent);
  tinfo_debug_styles(nc, stdn, indent);
  tinfo_debug_bitmaps(stdn, &nc->tcache, indent);
  unicodedumper(stdn, indent);
  char* path = prefix_data("notcurses.png");
  if(path){
    display_logo(stdn, path); // let it fail
    free(path);
  }
  if(notcurses_render(nc)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  return notcurses_stop(nc) ? EXIT_FAILURE : EXIT_SUCCESS;
}
