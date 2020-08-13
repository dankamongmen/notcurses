#include "demo.h"
#include <unictype.h>

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
allglyphs(struct notcurses* nc, struct ncplane* column, int legendy){
  // some of these cause major problems with Kitty, if not others, due to
  // heavy duty beating on freetype FIXME reenable when reasonable
  const int valid_planes[] = {
    0, 1,/* 2, 3, 14,*/
    /*15, 16,*/ -1
  };
  struct ncplane* std = notcurses_stdplane(nc);
  const int dimx = ncplane_dim_x(column);
  ncplane_set_base(column, " ", 0, 0);
  for(const int* plane = valid_planes ; *plane >= 0 ; ++plane){
    for(long int c = 0 ; c < 0x10000l ; ++c){
      const char32_t wc = *plane * 0x10000l + c;
      wchar_t w[2] = { wc, L'\0', };
      // surrogates are meaningful only for UTF-16
      if(wc >= 0xd800 && wc <= 0xdfff){
        continue;
      }
      if(uc_bidi_category(wc)){
        continue;
      }
      if(wcwidth(w[0]) >= 1){
        int x;
        if(ncplane_putwegc(column, w, NULL) < 0){
          return -1;
        }
        if(ncplane_printf_aligned(std, legendy, NCALIGN_CENTER, "0x%06x", wc) < 0){
          return -1;
        }
        ncplane_cursor_yx(column, NULL, &x);
        if(x >= dimx){
          DEMO_RENDER(nc);
          ncplane_set_fg_rgb(column,
                             random() % 192 + 64,
                             random() % 192 + 64,
                             random() % 192 + 64);
        }
      }
    }
  }
  DEMO_RENDER(nc);
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
  channel_set_rgb(&tl, 0, 0, 0);
  channel_set_rgb(&tr, 0, 0xff, 0);
  channel_set_rgb(&bl, 0, 0, 0xff);
  channel_set_rgb(&br, 0, 0xff, 0xff);
  if(ncplane_highgradient(n, tl, tr, bl, br, dimy - 1, dimx - 1) < 0){
    return -1;
  }
  ncplane_set_fg(n, 0xf0f0a0);
  ncplane_set_bg(n, 0);
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
  struct ncplane* column = ncplane_aligned(n, height, width, planey,
                                           NCALIGN_CENTER, NULL);
  if(column == NULL){
    return -1;
  }
  ncplane_set_scrolling(column, true);
  int r = allglyphs(nc, column, planey - 2);
  ncplane_destroy(column);
  return r;
}
