#include "demo.h"

// FIXME turn this into one large plane and move the plane, ratrher than
// manually redrawing each time
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

static struct ncplane*
make_slider(struct notcurses* nc, int dimy){
  const int REPS = 4;
  int y = dimy - sizeof(leg) / sizeof(*leg);
  const int len = strlen(leg[0]);
  struct ncplane* n = ncplane_new(nc, sizeof(leg) / sizeof(*leg), len * REPS, y, 0, NULL);
  uint64_t channels = 0;
  channels_set_fg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
  channels_set_bg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
  ncplane_set_base(n, " ", 0, channels);
  ncplane_set_scrolling(n, true);
  int r = 0x5f;
  int g = 0xaf;
  int b = 0x84;
  ncplane_set_bg_alpha(n, CELL_ALPHA_BLEND);
  for(int x = 0 ; x < REPS ; ++x){
    for(size_t l = 0 ; l < sizeof(leg) / sizeof(*leg) ; ++l){
      ncplane_set_fg_rgb_clipped(n, r + 0x8 * l, g + 0x8 * l, b + 0x8 * l);
      if(ncplane_set_bg_rgb(n, (l + 1) * 0x2, 0x20, (l + 1) * 0x2)){
        ncplane_destroy(n);
        return NULL;
      }
      if(ncplane_putstr_yx(n, l, x * len, leg[l]) != len){
        ncplane_destroy(n);
        return NULL;
      }
    }
    int t = r;
    r = g;
    g = b;
    b = t;
  }
  return n;
}

static int
perframecb(struct notcurses* nc, struct ncvisual* ncv __attribute__ ((unused)),
           const struct timespec* tspec, void* vnewplane){
  static int frameno = 0;
  int y, x;
  struct ncplane* n = vnewplane;
  assert(n);
  ncplane_yx(n, &y, &x);
  ncplane_move_yx(n, y, x - 1);
  ++frameno;
  DEMO_RENDER(nc);
  clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, tspec, NULL);
  return 0;
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
  struct ncvisual_options vopts = {
    .n = n,
  };
  struct ncvisual* ncv = ncvisual_from_file(nc, &vopts, path, &err);
  free(path);
  if(ncv == NULL){
    return -1;
  }
  struct ncplane* newpanel = make_slider(nc, dimy);
  if(newpanel == NULL){
    ncvisual_destroy(ncv);
    ncplane_destroy(n);
    return -1;
  }
  int ret = ncvisual_stream(nc, ncv, &err, 0.5 * delaymultiplier, perframecb, newpanel);
  ncvisual_destroy(ncv);
  ncplane_destroy(n);
  ncplane_destroy(newpanel);
  return ret;
}
