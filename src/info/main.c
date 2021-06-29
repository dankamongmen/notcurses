#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <notcurses/notcurses.h>
#include "internal.h" // internal headers

static inline wchar_t
capboolbool(unsigned utf8, bool cap){
  if(utf8){
    return cap ? L'✓' : L'✖';
  }else{
    return cap ? '+' : '-';
  }
}

static void
tinfo_debug_cap(struct ncplane* n, const char* name, bool yn,
                uint32_t colory, uint32_t colorn, char ch){
  if(yn){
    ncplane_set_fg_rgb(n, colory);
  }else{
    ncplane_set_fg_rgb(n, colorn);
  }
  ncplane_putstr(n, name);
  ncplane_putwc(n, capboolbool(notcurses_canutf8(ncplane_notcurses(n)), yn));
  ncplane_putchar(n, ch);
}

static int
braille_viz(ncplane* n, const char* l, const wchar_t* egcs, const char* r, const char* indent){
  ncplane_printf(n, "%s%s", indent, l);
  ncplane_printf(n, "%.192ls", egcs);
  ncplane_printf(n, "%s ", r);
  return 0;
}

static int
unicodedumper(struct ncplane* n, tinfo* ti, const char* indent){
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
    braille_viz(n, "⎡", NCBRAILLEEGCS, "⎤", indent);
    ncplane_printf(n, "⎨▐▌⎬ 🯸🯹\n");
    braille_viz(n, "⎢", NCBRAILLEEGCS + 64, "⎥", indent);
    ncplane_printf(n, "⎪🮈▋⎪\n");
    braille_viz(n, "⎢",  NCBRAILLEEGCS + 128, "⎥", indent);
    ncplane_printf(n, "⎪🮇▊⎪\n");
    braille_viz(n, "⎣",NCBRAILLEEGCS + 192, "⎦", indent);
    ncplane_printf(n, "⎪▕▉⎪\n");
    ncplane_printf(n, "%s⎛%ls⎞ ▔🭶🭷🭸🭹🭺🭻▁ 🭁 🭂 🭃 🭄 🭅 🭆 🭑 🭐 🭏 🭎 🭍 🭌 🭆🭑 🭄🭏 🭅🭐 🭃🭎 🭂🭍 🭁🭌 🭨🭪  ⎩ █⎭\n",
                   indent, NCEIGHTHSBOTTOM);
    ncplane_printf(n, "%s⎝%ls⎠ ▏🭰🭱🭲🭳🭴🭵▕ 🭒 🭓 🭔 🭕 🭖 🭧 🭜 🭟 🭠 🭡 🭞 🭝 🭧🭜 🭕🭠 🭖🭡 🭔🭟 🭓🭞 🭒🭝 🭪🭨 \n",
                   indent, NCEIGHTSTOP);
    int y, x;
    ncplane_cursor_yx(n, &y, &x);
    // the symbols for legacy computing
    ncplane_cursor_move_yx(n, y - 2, 12);
    uint32_t ul = CHANNEL_RGB_INITIALIZER(0x30, 0x30, 0x30);
    uint32_t lr = CHANNEL_RGB_INITIALIZER(0x80, 0x80, 0x80);
    ncplane_stain(n, y - 1, 65, ul, lr, ul, lr);
    // the vertical eighths
    ncplane_cursor_move_yx(n, y - 2, 2);
    ul = CHANNEL_RGB_INITIALIZER(0x60, 0x7d, 0x3b);
    lr = CHANNEL_RGB_INITIALIZER(0x02, 0x8a, 0x0f);
    ncplane_stain(n, y, 10, ul, lr, lr, ul);
    // the horizontal eighths
    ncplane_cursor_move_yx(n, y - 10, 69);
    ncplane_stain(n, y - 2, 70, lr, ul, ul, lr);
    // the braille
    ncplane_cursor_move_yx(n, y - 6, 2);
    ul = CHANNEL_RGB_INITIALIZER(0x7f, 0x25, 0x24);
    lr = CHANNEL_RGB_INITIALIZER(0x24, 0x25, 0x7f);
    ncplane_stain(n, y - 3, 65, ul, lr, ul, lr);
    // the sextants
    uint32_t ll = CHANNEL_RGB_INITIALIZER(0x40, 0x0, 0x0);
    ncplane_cursor_move_yx(n, y - 10, 28);
    ll = CHANNEL_RGB_INITIALIZER(0x7B, 0x68, 0xEE);
    ul = CHANNEL_RGB_INITIALIZER(0x19, 0x19, 0x70);
    ncplane_stain(n, y - 9, 58, ll, ul, ll, ul);
    // the quadrants and halfblocks
    ncplane_cursor_move_yx(n, y - 10, 9);
    ncplane_stain(n, y - 10, 22, ll, ul, ll, ul);
    ncplane_cursor_move_yx(n, y - 10, 2);
    ncplane_stain(n, y - 10, 5, ll, ul, ll, ul);
  }
  return 0;
}

static void
tinfo_debug_bitmaps(struct ncplane* n, const tinfo* ti, const char* indent){
  ncplane_set_fg_rgb8(n, 0xc4, 0x5a, 0xec);
  ncplane_printf(n, "%sbackground of 0x%06lx is %sconsidered transparent\n", indent, ti->bg_collides_default & 0xfffffful,
                   (ti->bg_collides_default & 0x01000000) ? "" : "not ");
  ncplane_set_fg_default(n);
  ncplane_set_fg_rgb(n, 0x5efa80);
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
  ncplane_set_fg_default(n);
}

static void
tinfo_debug_style(struct ncplane* n, const char* name, int style,
                  uint32_t colory, uint32_t colorn, char ch){
  unsigned support = notcurses_supported_styles(ncplane_notcurses(n)) & style;
  if(support){
    ncplane_set_fg_rgb(n, colory);
  }else{
    ncplane_set_fg_rgb(n, colorn);
  }
  ncplane_set_styles(n, style);
  ncplane_putstr(n, name);
  ncplane_putwc(n, capboolbool(notcurses_canutf8(ncplane_notcurses(n)), support));
  ncplane_set_styles(n, NCSTYLE_NONE);
  ncplane_putchar(n, ch);
}

static void
tinfo_debug_caps(struct ncplane* n, const tinfo* ti, const char* indent){
  uint32_t colory = 0xcc99ff;
  uint32_t colorn = 0xff99ff;
  ncplane_printf(n, "\n%s", indent);
  tinfo_debug_cap(n, "rgb", ti->caps.rgb, colory, colorn, ' ');
  tinfo_debug_cap(n, "ccc", ti->caps.can_change_colors, colory, colorn, ' ');
  tinfo_debug_cap(n, "af", get_escape(ti, ESCAPE_SETAF), colory, colorn, ' ');
  tinfo_debug_cap(n, "ab", get_escape(ti, ESCAPE_SETAB), colory, colorn, ' ');
  tinfo_debug_cap(n, "appsync", get_escape(ti, ESCAPE_BSU), colory, colorn, ' ');
  tinfo_debug_cap(n, "u7", get_escape(ti, ESCAPE_DSRCPR), colory, colorn, ' ');
  tinfo_debug_cap(n, "cup", get_escape(ti, ESCAPE_CUP), colory, colorn, ' ');
  tinfo_debug_cap(n, "vpa", get_escape(ti, ESCAPE_VPA), colory, colorn, ' ');
  tinfo_debug_cap(n, "hpa", get_escape(ti, ESCAPE_HPA), colory, colorn, ' ');
  tinfo_debug_cap(n, "sgr0", get_escape(ti, ESCAPE_SGR0), colory, colorn, ' ');
  tinfo_debug_cap(n, "op", get_escape(ti, ESCAPE_OP), colory, colorn, ' ');
  tinfo_debug_cap(n, "fgop", get_escape(ti, ESCAPE_FGOP), colory, colorn, ' ');
  tinfo_debug_cap(n, "bgop", get_escape(ti, ESCAPE_BGOP), colory, colorn, '\n');
}

static void
tinfo_debug_styles(const notcurses* nc, struct ncplane* n, const char* indent){
  const tinfo* ti = &nc->tcache;
  uint32_t colory = 0xc8a2c8;
  uint32_t colorn = 0xffa2c8;
  ncplane_putstr(n, indent);
  tinfo_debug_style(n, "blink", NCSTYLE_BLINK, colory, colorn, ' ');
  tinfo_debug_style(n, "bold", NCSTYLE_BOLD, colory, colorn, ' ');
  tinfo_debug_style(n, "ital", NCSTYLE_ITALIC, colory, colorn, ' ');
  tinfo_debug_style(n, "struck", NCSTYLE_STRUCK, colory, colorn, ' ');
  tinfo_debug_style(n, "ucurl", NCSTYLE_UNDERCURL, colory, colorn, ' ');
  tinfo_debug_style(n, "uline", NCSTYLE_UNDERLINE, colory, colorn, '\n');
  ncplane_set_fg_default(n);
  colory = 0x9172ec;
  colorn = 0xff72ec;
  ncplane_putstr(n, indent);
  tinfo_debug_cap(n, "utf8", ti->caps.utf8, colory, colorn, ' ');
  tinfo_debug_cap(n, "quad", ti->caps.quadrants, colory, colorn, ' ');
  tinfo_debug_cap(n, "sex", ti->caps.sextants, colory, colorn, ' ');
  tinfo_debug_cap(n, "braille", ti->caps.braille, colory, colorn, ' ');
  tinfo_debug_cap(n, "images", notcurses_canopen_images(nc), colory, colorn, ' ');
  tinfo_debug_cap(n, "videos", notcurses_canopen_videos(nc), colory, colorn, '\n');
  ncplane_set_fg_default(n);
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
  ncplane_set_scrolling(stdn, true);
  tinfo_debug_caps(stdn, &nc->tcache, indent);
  tinfo_debug_styles(nc, stdn, indent);
  tinfo_debug_bitmaps(stdn, &nc->tcache, indent);
  unicodedumper(stdn, &nc->tcache, indent);
  if(notcurses_render(nc)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  // FIXME want cursor wherever it ought be
  return notcurses_stop(nc) ? EXIT_FAILURE : EXIT_SUCCESS;
}
