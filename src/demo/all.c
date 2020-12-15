#include "demo.h"
#include <unictype.h>

// ¡PROBLEMATIC!
static int
problematic_unicode(char32_t wc){
  // surrogates are meaningful only for UTF-16
  if(wc >= 0xd800 && wc <= 0xdfff){
    return 1;
  }
  // these are broken in several terminals ㉈ ㉉ ㉊ ㉋ ㉌ ㉍ ㉎ ㉏
  // https://github.com/dankamongmen/notcurses/issues/881
  if(wc >= 0x3248 && wc <= 0x324f){
    return 1;
  }
  return 0;
}

// planes as of unicode 13.0:
//  0: BMP
//  1: SMP
//  2: SIP
//  3: TIP
// 14: SSPP
// 15: SPUAP
// 16: SPUBP
// works on a scrolling plane
static int
allglyphs(struct notcurses* nc, struct ncplane* column, int legendy,
          struct ncprogbar* left, struct ncprogbar* right){
  // some of these cause major problems with Kitty, if not others, due to
  // heavy duty beating on freetype FIXME reenable when reasonable
  const int valid_planes[] = {
    0, 1,/* 2, 3, 14,*/
    /*15, 16,*/ -1
  };
  // 64k characters (not all used) per valid plane
  const double totalglyphs = 0x10000ul * (sizeof(valid_planes) / sizeof(*valid_planes) - 1);
  double glyphsdone = 0;
  struct ncplane* std = notcurses_stdplane(nc);
  const int dimx = ncplane_dim_x(column);
  ncplane_set_base(column, " ", 0, 0);
  for(const int* plane = valid_planes ; *plane >= 0 ; ++plane){
    for(long int c = 0 ; c < 0x10000l ; ++c){
      const char32_t wc = *plane * 0x10000l + c;
      wchar_t w[2] = { wc, L'\0', };
      ++glyphsdone;
      if(problematic_unicode(wc)){
        continue;
      }
      if(wcwidth(w[0]) >= 1){
        int x;
        if(ncplane_putwegc(column, w, NULL) < 0){
          return -1;
        }
        ncplane_cursor_yx(column, NULL, &x);
        if(x >= dimx){
          ncplane_set_styles(std, NCSTYLE_BOLD | NCSTYLE_UNDERLINE | NCSTYLE_ITALIC);
          if(ncplane_printf_aligned(std, legendy, NCALIGN_CENTER, "0x%06x", wc) < 0){
            return -1;
          }
          ncplane_set_styles(std, NCSTYLE_NONE);
          ncprogbar_set_progress(left, glyphsdone / totalglyphs);
          ncprogbar_set_progress(right, glyphsdone / totalglyphs);
          DEMO_RENDER(nc);
          ncplane_set_fg_rgb8(column,
                             random() % 192 + 64,
                             random() % 192 + 64,
                             random() % 192 + 64);
//struct timespec ts = { .tv_sec = 0, .tv_nsec = 100000000, };
//nanosleep(&ts, NULL);
        }
      }
    }
  }
  DEMO_RENDER(nc);
  return 0;
}

static int
make_pbars(struct ncplane* column, struct ncprogbar** left, struct ncprogbar** right){
  int dimy, dimx, coly, colx, colposy, colposx;
  struct notcurses* nc = ncplane_notcurses(column);
  notcurses_stddim_yx(nc, &dimy, &dimx);
  ncplane_dim_yx(column, &coly, &colx);
  ncplane_yx(column, &colposy, &colposx);
  ncplane_options opts = {
    .x = colposx / 4 * -3,
    .rows = coly,
    .cols = (dimx - colx) / 4,
  };
  struct ncplane* leftp = ncplane_create(column, &opts);
  if(leftp == NULL){
    return -1;
  }
  ncplane_set_base(leftp, " ", 0, CHANNELS_RGB_INITIALIZER(0xdd, 0xdd, 0xdd, 0x1b, 0x1b, 0x1b));
  *left = ncprogbar_create(leftp, NULL);
  if(*left == NULL){
    return -1;
  }
  opts.x = colx + colposx / 4;
  struct ncplane* rightp = ncplane_create(column, &opts);
  if(rightp == NULL){
    return -1;
  }
  ncplane_set_base(rightp, " ", 0, CHANNELS_RGB_INITIALIZER(0xdd, 0xdd, 0xdd, 0x1b, 0x1b, 0x1b));
  *right = ncprogbar_create(rightp, NULL);
  if(*right == NULL){
    ncprogbar_destroy(*left);
    return -1;
  }
  return 0;
}

// render all the glyphs worth rendering
int allglyphs_demo(struct notcurses* nc){
  if(!notcurses_canutf8(nc)){
    return 0;
  }
  int dimy, dimx;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  ncplane_erase(n);
  ncplane_home(n);
  uint32_t tl = 0, tr = 0, bl = 0, br = 0;
  channel_set_rgb8(&tl, 0, 0, 0);
  channel_set_rgb8(&tr, 0, 0xff, 0);
  channel_set_rgb8(&bl, 0, 0, 0xff);
  channel_set_rgb8(&br, 0, 0xff, 0xff);
  if(ncplane_highgradient(n, tl, tr, bl, br, dimy - 1, dimx - 1) < 0){
    return -1;
  }
  ncplane_set_fg_rgb(n, 0xf0f0a0);
  ncplane_set_bg_rgb(n, 0);
  int width = 40;
  if(width > dimx - 8){
    if((width = dimx - 8) <= 0){
      return -1;
    }
  }
  int height = 40;
  if(height >= dimy - 4){
    if((height = dimy - 5) <= 0){
      return -1;
    }
  }
  const int planey = (dimy - height) / 2 + 1;
  ncplane_options nopts = {
    .y = planey,
    .x = NCALIGN_CENTER,
    .rows = height,
    .cols = width,
    .flags = NCPLANE_OPTION_HORALIGNED,
  };
  struct ncplane* column = ncplane_create(n, &nopts);
  if(column == NULL){
    return -1;
  }
  struct ncprogbar *pbarleft, *pbarright;
  if(make_pbars(column, &pbarleft, &pbarright)){
    ncplane_destroy(column);
    return -1;
  }
  ncplane_set_scrolling(column, true);
  int r = allglyphs(nc, column, planey - 2, pbarleft, pbarright);
  ncplane_destroy(column);
  // reflash the gradient to eliminate the counter, setting stage for next demo
  ncplane_cursor_move_yx(n, 1, 0);
  if(ncplane_highgradient(n, tl, tr, bl, br, dimy - 1, dimx - 1) < 0){
    return -1;
  }
  ncprogbar_destroy(pbarleft);
  ncprogbar_destroy(pbarright);
  return r;
}
