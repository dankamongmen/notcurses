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
perframecb(struct notcurses* nc, struct ncvisual* ncv __attribute__ ((unused)),
           void* vnewplane){
  static int startr = 0x80;
  static int startg = 0xff;
  static int startb = 0x80;
  static int frameno = 0;
  int dimx, dimy, y;
  struct ncplane* n = *(struct ncplane**)vnewplane;
  if(n == NULL){
    struct ncplane* nstd = notcurses_stdplane(nc);
    ncplane_dim_yx(nstd, &dimy, &dimx);
    //y = dimy - sizeof(leg) / sizeof(*leg) - 1;
    y = 0;
    n = ncplane_new(nc, sizeof(leg) / sizeof(*leg), dimx, y, 0, NULL);
    if(n == NULL){
      return -1;
    }
    *(struct ncplane**)vnewplane = n;
  }
  ncplane_dim_yx(n, &dimy, &dimx);
  cell c = CELL_SIMPLE_INITIALIZER(' ');
  cell_set_fg_alpha(&c, CELL_ALPHA_TRANSPARENT);
  cell_set_bg_alpha(&c, CELL_ALPHA_TRANSPARENT);
  ncplane_set_base(n, &c);
  ncplane_set_fg_alpha(n, CELL_ALPHA_BLEND);
  ncplane_set_bg_alpha(n, CELL_ALPHA_BLEND);
  // fg/bg rgbs are set within loop
  int x = dimx - frameno;
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
      for(size_t l = 0 ; l < sizeof(leg) / sizeof(*leg) ; ++l, ++y){
        if(ncplane_set_fg_rgb(n, r - 0xc * l, g - 0xc * l, b - 0xc * l)){
          return -1;
        }
        if(ncplane_set_bg_rgb(n, (l + 1) * 0x2, 0x20, (l + 1) * 0x2)){
          return -1;
        }
        if(ncplane_printf_yx(n, l, x, "%*.*s", len, len, leg[l] + stroff) != (int)len){
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
  demo_render(nc);
  return 0;
}

int xray_demo(struct notcurses* nc){
  int dimx, dimy;
  struct ncplane* nstd = notcurses_stdplane(nc);
  ncplane_dim_yx(nstd, &dimy, &dimx);
  struct ncplane* n = ncplane_new(nc, dimy, dimx, 0, 0, NULL);
  if(n == NULL){
    return -1;
  }
  char* path = find_data("notcursesI.avi");
  int averr;
  struct ncvisual* ncv = ncplane_visual_open(n, path, &averr);
  if(ncv == NULL){
    return -1;
  }
  if(ncvisual_decode(ncv, &averr) == NULL){
    return -1;
  }
  struct ncplane* newpanel = NULL;
  ncvisual_stream(nc, ncv, &averr, perframecb, &newpanel);
  ncvisual_destroy(ncv);
  ncplane_destroy(n);
  ncplane_destroy(newpanel);
  return 0;
}
