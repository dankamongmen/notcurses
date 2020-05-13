#include "demo.h"

static const char* leg[] = {
"                              88              88            88           88                          88             88               88                        ",
"                              \"\"              88            88           88                          88             \"\"               \"\"                 ,d     ",
"                                              88            88           88                          88                                                 88     ",
"  ,adPPYYba,     8b,dPPYba,   88   ,adPPYba,  88   ,d8      88,dPPYba,   88  ,adPPYYba,   ,adPPYba,  88   ,d8       88   ,adPPYba,   88  8b,dPPYba,  MM88MMM   ",
"  \"\"     `Y8     88P'   `\"8a  88  a8\"     \"\"  88 ,a8\"       88P'    \"8a  88  \"\"     `Y8  a8\"     \"\"  88 ,a8\"        88  a8\"     \"8a  88  88P'   `\"8a   88      ",
"  ,adPPPPP88     88       88  88  8b          8888[         88       d8  88  ,adPPPPP88  8b          8888[          88  8b       d8  88  88       88   88      ",
"  88,    ,88     88       88  88  \"8a,   ,aa  88`\"Yba,      88b,   ,a8\"  88  88,    ,88  \"8a,   ,aa  88`\"Yba,       88  \"8a,   ,a8\"  88  88       88   88,     ",
"  `\"8bbdP\"Y8     88       88  88   `\"Ybbd8\"'  88   `Y8a     8Y\"Ybbd8\"'   88  `\"8bbdP\"Y8   `\"Ybbd8\"'  88   `Y8a      88   `\"YbbdP\"'   88  88       88   \"Y888   ",
"                                                                                                                   ,88                                         ",
"                                                                                                                 888P                                          ",
};

static int
watch_for_keystroke(struct notcurses* nc){
  wchar_t w;
  if((w = demo_getc_nblock(nc, NULL)) != (wchar_t)-1){
    if(w == 'q'){
      return 1;
    }
  }
  return demo_render(nc);
}

static int
perframecb(struct notcurses* nc, struct ncvisual* ncv __attribute__ ((unused)),
           void* vnewplane){
  static int startr = 0x5f;
  static int startg = 0xaf;
  static int startb = 0x84;
  static int frameno = 0;
  int dimx, dimy;
  struct ncplane* n = *(struct ncplane**)vnewplane;
  if(n == NULL){
    notcurses_term_dim_yx(nc, &dimy, &dimx);
    int y = dimy - sizeof(leg) / sizeof(*leg);
    n = ncplane_new(nc, sizeof(leg) / sizeof(*leg), dimx, y, 0, NULL);
    if(n == NULL){
      return -1;
    }
    *(struct ncplane**)vnewplane = n;
    uint64_t channels = 0;
    channels_set_fg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
    channels_set_bg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
    ncplane_set_base(n, " ", 0, channels);
    ncplane_set_bg_alpha(n, CELL_ALPHA_BLEND);
    ncplane_set_scrolling(n, true);
  }
  ncplane_dim_yx(n, &dimy, &dimx);
  // fg/bg rgbs are set within loop
  int x = dimx - (frameno * 2);
  int r = startr;
  int g = startg;
  int b = startb;
  const size_t llen = strlen(leg[0]);
  do{
    if(x + (int)llen <= 0){
      x += llen;
    }else{
      int len = dimx - x;
      if(x < 0){
        len = llen + x;
      }
      if(len > (int)llen){
        len = llen;
      }
      if(len > dimx){
        len = dimx;
      }
      int stroff = 0;
      if(x < 0){
        stroff = -x;
        x = 0;
      }
      ncplane_set_bg_alpha(n, CELL_ALPHA_BLEND);
      for(size_t l = 0 ; l < sizeof(leg) / sizeof(*leg) ; ++l){
        ncplane_set_fg_rgb_clipped(n, r + 0x8 * l, g + 0x8 * l, b + 0x8 * l);
        if(ncplane_set_bg_rgb(n, (l + 1) * 0x2, 0x20, (l + 1) * 0x2)){
          return -1;
        }
        if(ncplane_printf_yx(n, l, x, "%*.*s", len, len, leg[l] + stroff) != len){
          return -1;
        }
      }
      x += len;
    }
    int t = r;
    r = g;
    g = b;
    b = t;
  }while(x < dimx);
  ++frameno;
  return watch_for_keystroke(nc);
}

int xray_demo(struct notcurses* nc){
  if(!notcurses_canopen_videos(nc)){
    return 0;
  }
  int dimx, dimy;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  struct ncplane* n = ncplane_new(nc, dimy, dimx, 0, 0, NULL);
  if(n == NULL){
    return -1;
  }
  char* path = find_data("notcursesI.avi");
  nc_err_e err;
  struct ncvisual* ncv = ncplane_visual_open(n, path, &err);
  free(path);
  if(ncv == NULL){
    return -1;
  }
  struct ncplane* newpanel = NULL;
  int ret = ncvisual_stream(nc, ncv, &err, 0.5 * delaymultiplier, perframecb, &newpanel);
  ncvisual_destroy(ncv);
  ncplane_destroy(n);
  ncplane_destroy(newpanel);
  return ret;
}
