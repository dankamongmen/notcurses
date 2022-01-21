#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <notcurses/notcurses.h>
#include "lib/internal.h" // internal headers

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
  unsigned wchars;
  for(const wchar_t* w = wp ; *w ; w += wchars){
    if(ncplane_putwc_utf32(n, w, &wchars) <= 0){
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
braille_viz(struct ncplane* n, wint_t l, const wchar_t* egcs, wchar_t r,
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
  unsigned x;
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
  static const char emoji[] = "\U0001f47e" // alien monster
                              "\U0001f3f4" // waving black flag
                              "\U0001f918" // sign of the horns
                              "\U0001f6ac" // cigarette, delicious
                              "\U0001f30d" // globe europe/africa
                              "\U0001f30e" // globe americas
                              "\U0001f30f" // globe asia/australia
                              "\U0001F946" // rifle
                              "\U0001f4a3" // bomb
                              "\U0001f5e1" // dagger
                              "\U0001F52B" // pistol
                              "\u2697\ufe0f" // alembic
                              "\u269b\ufe0f" // atom
                              "\u2622\ufe0f" // radiation sign
                              "\u2623\ufe0f" // biohazard
                              "\U0001F33F" // herb
                              "\U0001F3B1" // billiards
                              "\U0001F3E7" // automated teller machine
                              "\U0001F489" // syringe
                              "\U0001F48A" // pill
                              "\U0001f574\ufe0f" // man in suit levitating
                              "\U0001F4E1" // satellite antenna
                              "\U0001F93B" // modern pentathlon
                              "\U0001F991" // squid
                              "\U0001f1e6\U0001f1f6" // regional indicators AQ (antarctica)
                              "\U0001f469\u200d\U0001f52c" // woman scientist
                              "\U0001faa4" // mouse trap
                              "\U0001f6b1" // non-potable water
                              "\u270a\U0001f3ff" // type-6 raised fist
                              "\U0001f52c" // microscope
                              "\U0001f9ec" // dna double helix
                              "\U0001f3f4\u200d\u2620\ufe0f" // pirate flag
                              "\U0001f93d\U0001f3fc\u200d\u2640\ufe0f" // type-3 woman playing water polo
                              ;
  ncplane_set_bg_default(n);
  size_t bytes;
  for(const char* e = emoji ; *e ; e += bytes){
    if(ncplane_putegc(n, e, &bytes) < 0){
      if(ncplane_putchar(n, ' ') < 0){
        break;
      }
    }
  }
  unsigned x;
  ncplane_cursor_yx(n, NULL, &x);
  while(x++ < 80){
    ncplane_putchar(n, ' ');
  }
  if(ncplane_dim_x(n) > 80){
    ncplane_putchar(n, '\n');
  }
  return 0;
}

// symbols for legacy computing
static int
legacy_viz(struct ncplane* n, const char* indent, const wchar_t* eighths,
           const wchar_t* anglesr, const wchar_t* anglesl){
  ncplane_printf(n, "%s ", indent);
  unsigned wchars;
  for(const wchar_t* e = eighths ; *e ; e += wchars){
    if(ncplane_putwc_utf32(n, e, &wchars) <= 0){
      ncplane_putchar(n, ' ');
    }
  }
  ncplane_putchar(n, ' ');
  const wchar_t* r = anglesr;
  const wchar_t* l = anglesl;
  while(*r){
    if(ncplane_putwc_utf32(n, r, &wchars) <= 0){
      ncplane_putchar(n, ' ');
    }
    r += wchars;
    if(ncplane_putwc_utf32(n, l, &wchars) <= 0){
      ncplane_putchar(n, ' ');
    }
    l += wchars;
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
  unsigned wchars = 0;
  for(const wchar_t* p = post ; *p ; p += wchars){
    if(ncplane_putwc_utf32(n, p, &wchars) <= 0){
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
       const wchar_t* w13, const wchar_t* w14, const wchar_t* w15){
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
  wvizn(n, wb, 2);
  wvizn(n, wc, 1);
  ncplane_putchar(n, ' ');
  wvizn(n, wd, 2);
  wvizn(n, we, 1);
  ncplane_putchar(n, ' ');
  wvizn(n, wf, 2);
  wvizn(n, w10, 1);
  ncplane_putchar(n, ' ');
  wvizn(n, w11, 2);
  wvizn(n, w12, 1);
  wvizn(n, w13, 3); // chess
  wviz(n, w14);
  wviz(n, w15);
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
unicodedumper(struct notcurses* nc, struct ncplane* n, const char* indent){
  if(notcurses_canutf8(ncplane_notcurses_const(n))){
    // all NCHALFBLOCKS are contained within NCQUADBLOCKS
    ncplane_printf(n, "%s%ls⎧", indent, NCQUADBLOCKS);
    // 🯰🯱🯲🯳🯴🯵🯶🯷🯸🯹 (on Windows, these will be encoded as UTF-16 surrogate
    // pairs due to a 16-bit wchar_t.
    sex_viz(n, NCSEXBLOCKS, L'⎫', L"♠♥" NCSEGDIGITS L"\u2157\u2158\u2159\u215a\u215b");
    vertviz(n, L'⎧', NCEIGHTHSR[0], NCEIGHTHSL[0], L'⎫', L"┌╥─╥─╥┐🭩⎛⎞");
    ncplane_printf(n, "%s╲╿╱ ◨◧ ◪◩ ◖◗ ⫷⫸ ⎩", indent);
    sex_viz(n, &NCSEXBLOCKS[32], L'⎭', L"♦♣\u00bc\u00bd\u00be\u2150\u2151\u2152\u2153\u2154\u2155\u2156\u215c\u215d\u215e\u215f\u2189");
    vertviz(n, L'⎪', NCEIGHTHSR[1], NCEIGHTHSL[1], L'⎪', L"├╜╓╫╖╙┤🭫⎜⎟");
    ncplane_printf(n, "%s╾╳╼ ", indent);
    triviz(n, NCWHITESQUARESW, NCWHITECIRCLESW, NCDIAGONALSW, &NCDIAGONALSW[4],
           NCCIRCULARARCSW, NCWHITETRIANGLESW, NCSHADETRIANGLESW, NCBLACKTRIANGLESW,
           NCBOXLIGHTW, &NCBOXLIGHTW[4], NCBOXHEAVYW, &NCBOXHEAVYW[4], NCBOXROUNDW,
           &NCBOXROUNDW[4], NCBOXDOUBLEW, &NCBOXDOUBLEW[4], NCBOXOUTERW, &NCBOXOUTERW[4],
           NCCHESSBLACK, L"⩘▵△▹▷▿▽◃◁", NCARROWW);
    vertviz(n, L'⎪', NCEIGHTHSR[2], NCEIGHTHSL[2], L'⎪', L"├─╨╫╨─┤┇⎜⎟");
    ncplane_printf(n, "%s╱╽╲ ", indent);
    triviz(n, &NCWHITESQUARESW[2], &NCWHITECIRCLESW[2], &NCDIAGONALSW[2], &NCDIAGONALSW[6],
           &NCCIRCULARARCSW[2], &NCWHITETRIANGLESW[2], &NCSHADETRIANGLESW[2], &NCBLACKTRIANGLESW[2],
           &NCBOXLIGHTW[2], &NCBOXLIGHTW[5], &NCBOXHEAVYW[2], &NCBOXHEAVYW[5], &NCBOXROUNDW[2],
           &NCBOXROUNDW[5], &NCBOXDOUBLEW[2], &NCBOXDOUBLEW[5], &NCBOXOUTERW[2], &NCBOXOUTERW[5],
           &NCCHESSBLACK[3], L"⩗▴⏶⯅▲▸⏵⯈▶", L"▾⏷⯆▼◂⏴⯇◀");
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
    // 🭫⎞⎪🭨🭪⎪╰╯││║╔═╝⊴⊵
    wviz(n, L"\U0001FB6B\u239e⎪🭨🭪⎪╰╯││║╔═╝⊴⊵");
    if(ncplane_dim_x(n) > 80){
      ncplane_putchar(n, '\n');
    }
    legacy_viz(n, indent, L"▏🭰🭱🭲🭳🭴🭵▕", NCANGLESTR, NCANGLESTL);
    wviz(n, NCDIGITSSUPERW);
    wviz(n, L" ⎝");
    wviz(n, NCEIGHTHST);
    // 🭩⎠⎩🭪🭨⎭⧒⧑╰╯╚╝❨❩⟃⟄
    wviz(n, L"\U0001FB69\u23a0⎩🭪🭨⎭⧒⧑╰╯╚╝❨❩⟃⟄");
    if(ncplane_dim_x(n) > 80){
      ncplane_putchar(n, '\n');
    }
    emoji_viz(n);
    unsigned y, x;
    ncplane_cursor_yx(n, &y, &x);
    ncpalette* npal = ncpalette_new(nc);
    unsigned r, g, b;
    // if we weren't able to detect the palette, we'll just have a black
    // background throughout the unicode section. otherwise, our gradient
    // will be based off the existing palette.
    ncpalette_get_rgb8(npal, 4, &r, &g, &b);
    uint64_t ur = NCCHANNELS_INITIALIZER(0xff, 0xff, 0xff, r, g, b);
    ncpalette_get_rgb8(npal, 5, &r, &g, &b);
    uint64_t lr = NCCHANNELS_INITIALIZER(0xff, 0xff, 0xff, r, g, b);
    ncpalette_get_rgb8(npal, 6, &r, &g, &b);
    uint64_t ul = NCCHANNELS_INITIALIZER(0xff, 0xff, 0xff, r, g, b);
    ncpalette_get_rgb8(npal, 7, &r, &g, &b);
    uint64_t ll = NCCHANNELS_INITIALIZER(0xff, 0xff, 0xff, r, g, b);
    ncpalette_free(npal);
    ncplane_stain(n, y - 16, 0, 15, 80, ul, ur, ll, lr);
    ncplane_set_styles(n, NCSTYLE_BOLD | NCSTYLE_ITALIC);
    ncplane_cursor_move_yx(n, y - 12, 55);
    wviz(n, L"🯁🯂🯃https://notcurses.com");
    ncplane_set_styles(n, NCSTYLE_NONE);
  }
  return 0;
}

static int
display_logo(struct ncplane* n, const char* path){
  unsigned cpixy, cpixx;
  ncplane_pixel_geom(n, NULL, NULL, &cpixy, &cpixx, NULL, NULL);
  struct ncvisual* ncv = ncvisual_from_file(path);
  if(ncv == NULL){
    return -1;
  }
  if(ncvisual_resize(ncv, 3 * cpixy, 24 * cpixx)){
    ncvisual_destroy(ncv);
    return -1;
  }
  unsigned y;
  ncplane_cursor_yx(n, &y, NULL);
  struct ncvisual_options vopts = {
    .n = n,
    .y = y - 3,
    .x = 55,
    .blitter = NCBLIT_PIXEL,
    .flags = NCVISUAL_OPTION_CHILDPLANE | NCVISUAL_OPTION_NODEGRADE,
  };
  struct ncplane* bitm = ncvisual_blit(ncplane_notcurses(n), ncv, &vopts);
  if(bitm == NULL){
    ncvisual_destroy(ncv);
    return -1;
  }
  ncvisual_destroy(ncv);
  return 0;
}

static void
tinfo_debug_bitmaps(struct ncplane* n, const tinfo* ti, const char* indent){
  uint32_t fg = 0;
  int r = notcurses_default_foreground(ncplane_notcurses(n), &fg);
  if(r){
    ncplane_printf(n, "%sno known default fg ", indent);
  }else{
    ncplane_printf(n, "%sdefault fg 0x%06x ", indent, fg);
  }
  uint32_t bg = 0;
  r = notcurses_default_background(ncplane_notcurses(n), &bg);
  if(r){
    ncplane_printf(n, "no known default bg");
  }else{
    ncplane_printf(n, "default bg 0x%06x", bg);
  }
  tinfo_debug_cap(n, " pmouse", ti->pixelmice);
  finish_line(n);
  ncpixelimpl_e blit = notcurses_check_pixel_support(ncplane_notcurses(n));
  switch(blit){
    case NCPIXEL_NONE:
      ncplane_printf(n, "%sno bitmap graphics detected", indent);
      break;
    case NCPIXEL_SIXEL:
      if(ti->sixel_maxy){
        ncplane_printf(n, "%smax sixel size: %dx%d colorregs: %u",
                      indent, ti->sixel_maxy, ti->sixel_maxx, ti->color_registers);
      }else{
        ncplane_printf(n, "%ssixel colorregs: %u", indent, ti->color_registers);
      }
      break;
    case NCPIXEL_LINUXFB:
      ncplane_printf(n, "%sframebuffer graphics supported", indent);
      break;
    case NCPIXEL_ITERM2:
      ncplane_printf(n, "%siTerm2 graphics supported", indent);
      break;
    case NCPIXEL_KITTY_STATIC:
      ncplane_printf(n, "%srgba pixel graphics support", indent);
      break;
    case NCPIXEL_KITTY_ANIMATED:
      ncplane_printf(n, "%s1st gen rgba pixel animation support", indent);
      break;
    case NCPIXEL_KITTY_SELFREF:
      ncplane_printf(n, "%s2nd gen rgba pixel animation support", indent);
      break;
  }
  finish_line(n);
}

static void
tinfo_debug_caps(struct ncplane* n, const tinfo* ti, const char* indent){
  ncplane_printf(n, "%s", indent);
  tinfo_debug_cap(n, "af", get_escape(ti, ESCAPE_SETAF));
  tinfo_debug_cap(n, "ab", get_escape(ti, ESCAPE_SETAB));
  tinfo_debug_cap(n, "sum", get_escape(ti, ESCAPE_BSUM));
  tinfo_debug_cap(n, "vpa", get_escape(ti, ESCAPE_VPA));
  tinfo_debug_cap(n, "hpa", get_escape(ti, ESCAPE_HPA));
  tinfo_debug_cap(n, "sgr0", get_escape(ti, ESCAPE_SGR0));
  tinfo_debug_cap(n, "op", get_escape(ti, ESCAPE_OP));
  tinfo_debug_cap(n, "fgop", get_escape(ti, ESCAPE_FGOP));
  tinfo_debug_cap(n, "bgop", get_escape(ti, ESCAPE_BGOP));
  tinfo_debug_cap(n, "bce", ti->bce);
  tinfo_debug_cap(n, "rect", get_escape(ti, ESCAPE_DECERA));
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
  tinfo_debug_cap(n, "el", get_escape(ti, ESCAPE_EL));
  finish_line(n);
  ncplane_putstr(n, indent);
  tinfo_debug_cap(n, "utf8", notcurses_canutf8(nc));
  tinfo_debug_cap(n, "2x1", notcurses_canhalfblock(nc));
  tinfo_debug_cap(n, "2x2", notcurses_canquadrant(nc));
  tinfo_debug_cap(n, "3x2", notcurses_cansextant(nc));
  tinfo_debug_cap(n, "4x2", notcurses_canbraille(nc));
  tinfo_debug_cap(n, "img", notcurses_canopen_images(nc));
  tinfo_debug_cap(n, "vid", notcurses_canopen_videos(nc));
  tinfo_debug_cap(n, "indn", get_escape(ti, ESCAPE_INDN));
  tinfo_debug_cap(n, "gpm", ti->gpmfd >= 0);
  tinfo_debug_cap(n, "kbd", ti->kittykbdsupport);
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
             | NCOPTION_NO_CLEAR_BITMAPS
             | NCOPTION_DRAIN_INPUT,
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
  // so that we know whether we're talking to gpm
  notcurses_mice_enable(nc, NCMICE_ALL_EVENTS);
  const char indent[] = "";
  unsigned dimx;
  struct ncplane* stdn = notcurses_stddim_yx(nc, NULL, &dimx);
  if(dimx < 80){
    ncplane_set_fg_rgb(stdn, 0xff5349);
    ncplane_set_styles(stdn, NCSTYLE_BOLD);
    ncplane_putstr(stdn, "This program requires at least 80 columns.\n");
    notcurses_render(nc);
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  ncplane_set_fg_alpha(stdn, NCALPHA_HIGHCONTRAST);
  ncplane_set_fg_rgb(stdn, 0xffffff);
  ncplane_set_scrolling(stdn, true);
  tinfo_debug_caps(stdn, &nc->tcache, indent);
  tinfo_debug_styles(nc, stdn, indent);
  tinfo_debug_bitmaps(stdn, &nc->tcache, indent);
  unicodedumper(nc, stdn, indent);
  char* path = notcurses_data_path(NULL, "notcurses.png");
  if(path){
    if(notcurses_canpixel(nc)){
      display_logo(stdn, path); // let it fail
    }
    free(path);
  }
  if(notcurses_render(nc)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  return notcurses_stop(nc) ? EXIT_FAILURE : EXIT_SUCCESS;
}
