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

static int
perframecb(struct notcurses* nc, struct ncvisual* ncv __attribute__ ((unused))){
  static int frameno = 0;
  int dimx, dimy;
  struct ncplane* n = notcurses_stdplane(nc);
  ncplane_dim_yx(n, &dimy, &dimx);
  ncplane_putsimple_yx(n, 0, 0, 'a');
  int y = dimy - sizeof(leg) / sizeof(*leg) - 3;
  int x = dimx - frameno;
  for(size_t l = 0 ; l < sizeof(leg) / sizeof(*leg) ; ++l, ++y){
    ncplane_set_bg_rgb(n, l * 0x4, 0x20, l * 0x4);
    ncplane_set_fg_rgb(n, 0x80, 0xff - (l * 0x10), 0x80);
    int xoff = x;
    while(xoff + (int)strlen(leg[l]) <= 0){
      xoff += strlen(leg[l]);
    }
    do{
      int len = dimx - xoff;
      if(xoff < 0){
        len = strlen(leg[l]) + xoff;
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
if(y == 22)fprintf(stderr, "printing %d at %d (%d)\n", len, xoff, dimx);
      ncplane_printf_yx(n, y, xoff, "%*.*s", len, len, leg[l] + stroff);
      xoff += len;
    }while(xoff < dimx);
  }
  ++frameno;
  notcurses_render(nc);
  return 0;
}

int xray_demo(struct notcurses* nc){
  struct ncplane* n = notcurses_stdplane(nc);
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
  return 0;
}
