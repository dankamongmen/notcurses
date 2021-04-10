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
make_slider(struct notcurses* nc, int dimy, int dimx){
  // 487 frames in the video
  const int len = strlen(leg[0]);
  const int REPS = 487 / len + dimx / len;
  int y = dimy - sizeof(leg) / sizeof(*leg);
  struct ncplane_options nopts = {
    .y = y,
    .x = 0,
    .rows = sizeof(leg) / sizeof(*leg),
    .cols = len * REPS,
    .name = "scrl",
  };
  struct ncplane* n = ncplane_create(notcurses_stdplane(nc), &nopts);
  uint64_t channels = 0;
  channels_set_fg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
  channels_set_bg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
  ncplane_set_base(n, " ", 0, channels);
  ncplane_set_scrolling(n, true);
  int r = 0x5f;
  int g = 0xaf;
  int b = 0x84;
  ncplane_set_bg_alpha(n, CELL_ALPHA_TRANSPARENT);
  for(int x = 0 ; x < REPS ; ++x){
    for(size_t l = 0 ; l < sizeof(leg) / sizeof(*leg) ; ++l){
      ncplane_set_fg_rgb8_clipped(n, r + 0x8 * l, g + 0x8 * l, b + 0x8 * l);
      if(ncplane_set_bg_rgb8(n, (l + 1) * 0x2, 0x20, (l + 1) * 0x2)){
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
perframecb(struct ncvisual* ncv, struct ncvisual_options* vopts,
           const struct timespec* tspec, void* vnewplane){
  (void)ncv;
  // only need these two steps done once, but we can't do them in
  // main() due to the plane being created in ncvisual_stream() =[
  ncplane_set_resizecb(vopts->n, ncplane_resize_maximize);
  ncplane_move_above(vnewplane, vopts->n);

  struct notcurses* nc = ncplane_notcurses(vopts->n);
  static int frameno = 0;
  int x;
  struct ncplane* n = vnewplane;
  assert(n);
  ncplane_yx(n, NULL, &x);
  ncplane_move_yx(n, 1, x - 1);
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
  ncplane_erase(notcurses_stdplane(nc));
  char* path = find_data("notcursesIII.mkv");
  struct ncvisual* ncv = ncvisual_from_file(path);
  free(path);
  if(ncv == NULL){
    return -1;
  }
  struct ncplane* newpanel = make_slider(nc, dimy, dimx);
  if(newpanel == NULL){
    ncvisual_destroy(ncv);
    return -1;
  }
  struct ncvisual_options vopts = {
    .y = NCALIGN_CENTER,
    .x = NCALIGN_CENTER,
    .scaling = NCSCALE_SCALE_HIRES,
    .blitter = NCBLIT_PIXEL,
    .flags = NCVISUAL_OPTION_NODEGRADE // to test for NCBLIT_PIXEL
              | NCVISUAL_OPTION_VERALIGNED | NCVISUAL_OPTION_HORALIGNED,
  };
  float dm = 0;
  // returns 0 if the selected blitter isn't available
  if(ncvisual_blitter_geom(nc, ncv, &vopts, NULL, NULL, NULL, NULL, NULL)){
    vopts.flags &= ~NCVISUAL_OPTION_NODEGRADE;
    dm = 0.5 * delaymultiplier;
  }
  int ret = ncvisual_stream(nc, ncv, dm, perframecb, &vopts, newpanel);
  ncvisual_destroy(ncv);
  ncplane_destroy(newpanel);
  return ret;
}
