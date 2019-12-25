#include "demo.h"

static const char* leg[] = {
"                               88              88            88           88                          88             88               88                        ",
"                               \"\"              88            88           88                          88             \"\"               \"\"                 ,d     ",
"                                               88            88           88                          88                                                 88     ",
"   ,adPPYYba,     8b,dPPYba,   88   ,adPPYba,  88   ,d8      88,dPPYba,   88  ,adPPYYba,   ,adPPYba,  88   ,d8       88   ,adPPYba,   88  8b,dPPYba,  MM88MMM   ",
"   \"\"     `Y8     88P'   `\"8a  88  a8\"     \"\"  88 ,a8\"       88P'    \"8a  88  \"\"     `Y8  a8\"     \"\"  88 ,a8\"        88  a8\"     \"8a  88  88P'   `\"8a   88      ",
"   ,adPPPPP88     88       88  88  8b          8888[         88       d8  88  ,adPPPPP88  8b          8888[          88  8b       d8  88  88       88   88      ",
"   88,    ,88     88       88  88  \"8a,   ,aa  88`\"Yba,      88b,   ,a8\"  88  88,    ,88  \"8a,   ,aa  88`\"Yba,       88  \"8a,   ,a8\"  88  88       88   88,     ",
"   `\"8bbdP\"Y8     88       88  88   `\"Ybbd8\"'  88   `Y8a     8Y\"Ybbd8\"'   88  `\"8bbdP\"Y8   `\"Ybbd8\"'  88   `Y8a      88   `\"YbbdP\"'   88  88       88   \"Y888   ",
"                                                                                                                    ,88                                         ",
"                                                                                                                  888P                                          ",
};

static struct ncplane* killme; // FIXME

static int
perframecb(struct notcurses* nc, struct ncvisual* ncv __attribute__ ((unused))){
  static struct ncplane* n = NULL;
  static int startr = 0x80;
  static int startg = 0xff;
  static int startb = 0x80;
  static int frameno = 0;
  int dimx, dimy, y;
  if(n == NULL){
    struct ncplane* nstd = notcurses_stdplane(nc);
    ncplane_dim_yx(nstd, &dimy, &dimx);
    y = dimy - sizeof(leg) / sizeof(*leg);
    // FIXME how will this plane be destroyed?
    n = notcurses_newplane(nc, sizeof(leg) / sizeof(*leg), dimx, y, 0, NULL);
    if(n == NULL){
      return -1;
    }
    killme = n;
  }
  ncplane_dim_yx(n, &dimy, &dimx);
  y = 0;
  cell c = CELL_SIMPLE_INITIALIZER(' ');
  cell_set_fg_alpha(&c, CELL_ALPHA_TRANSPARENT);
  cell_set_bg_alpha(&c, CELL_ALPHA_TRANSPARENT);
  ncplane_set_default(n, &c);
  ncplane_set_fg_alpha(n, CELL_ALPHA_BLEND);
  ncplane_set_bg_alpha(n, CELL_ALPHA_BLEND);
  // fg/bg rgbs are set within loop
  int x = dimx - frameno;
  for(size_t l = 0 ; l < sizeof(leg) / sizeof(*leg) ; ++l, ++y){
    int r = startr;
    int g = startg - (l * 0x8);
    int b = startb;
    ncplane_set_bg_rgb(n, l * 0x4, 0x20, l * 0x4);
    int xoff = x;
    while(xoff + (int)strlen(leg[l]) <= 0){
      xoff += strlen(leg[l]);
    }
    do{
      ncplane_set_fg_rgb(n, r, g, b);
      int len = dimx - xoff;
      if(xoff < 0){
        len = strlen(leg[l]) + xoff;
      }else if(xoff == 0){
        int t = startr;
        startr = startg;
        startg = startb;
        startb = t;
      }
      if(len > (int)strlen(leg[l])){
        len = strlen(leg[l]);
      }
      if(len > dimx){
        len = dimx;
      }
      int stroff = 0;
      if(xoff < 0){
        stroff = -xoff;
        xoff = 0;
      }
      ncplane_printf_yx(n, y, xoff, "%*.*s", len, len, leg[l] + stroff);
      xoff += len;
      int t = r;
      r = g;
      g = b;
      b = t;
    }while(xoff < dimx);
  }
  ++frameno;
  demo_render(nc);
  // FIXME we'll need some delay here
  return 0;
}

int xray_demo(struct notcurses* nc){
  int dimx, dimy;
  struct ncplane* nstd = notcurses_stdplane(nc);
  ncplane_dim_yx(nstd, &dimy, &dimx);
  struct ncplane* n = notcurses_newplane(nc, dimy, dimx, 0, 0, NULL);
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
  ncvisual_stream(nc, ncv, &averr, perframecb);
  ncvisual_destroy(ncv);
  ncplane_destroy(n);
  ncplane_destroy(killme);
  return 0;
}
