#include "demo.h"
#include <pthread.h>
#include <stdatomic.h>

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
make_slider(struct notcurses* nc, int dimx){
  // 862 frames in the video
  const int len = strlen(leg[0]);
  const int REPS = 862 / len + dimx / len;
  struct ncplane_options nopts = {
    .y = 1,
    .x = 0,
    .rows = sizeof(leg) / sizeof(*leg),
    .cols = len * REPS,
    .name = "scrl",
  };
  struct ncplane* n = ncplane_create(notcurses_stdplane(nc), &nopts);
  uint64_t channels = 0;
  ncchannels_set_fg_alpha(&channels, NCALPHA_TRANSPARENT);
  ncchannels_set_bg_alpha(&channels, NCALPHA_TRANSPARENT);
  ncplane_set_base(n, " ", 0, channels);
  ncplane_set_scrolling(n, true);
  int r = 0x5f;
  int g = 0xaf;
  int b = 0x84;
  ncplane_set_bg_alpha(n, NCALPHA_TRANSPARENT);
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

static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t render_lock = PTHREAD_MUTEX_INITIALIZER;

// initialized per run
struct marsh {
  struct notcurses* nc;
  struct ncvisual* ncv;     // video stream, one copy per thread
  struct ncplane* slider;   // text plane at top, sliding to the left
  float dm;                 // delay multiplier
  int next_frame;
  int* frame_to_render; // protected by renderlock
  struct ncplane** lplane;   // plane to destroy
};

// make a plane on a new pile suitable for rendering a frame of the video
static int
make_plane(struct notcurses* nc, struct ncplane** t){
  unsigned dimy, dimx;
  notcurses_stddim_yx(nc, &dimy, &dimx);
  // FIXME want a resizecb
  struct ncplane_options opts = {
    .rows = dimy - 1,
    .cols = dimx,
    .name = "bmap",
  };
  *t = ncpile_create(nc, &opts);
  if(!*t){
    return -1;
  }
  return 0;
}

// returns the index of the next frame, which can immediately begin to be
// rendered onto the thread's plane.
static int
get_next_frame(struct marsh* m, struct ncvisual_options* vopts){
  int ret = 0;
  // one does the odds, and one the evens. load two unless we're the even,
  // and it's the first frame.
  if(m->next_frame){
    if(ncvisual_decode(m->ncv)){
      ret = -1;
    }
  }
  if(ret == 0){
    if(ncvisual_decode(m->ncv)){
      ret = -1;
    }else if(ncvisual_blit(m->nc, m->ncv, vopts) == NULL){
      ret = -1;
    }
  }
  if(ret == 0){
    ret = m->next_frame;
    m->next_frame += 2;
  }
  return ret;
}

static void*
xray_thread(void *vmarsh){
  struct marsh* m = vmarsh;
  int frame = -1;
  struct ncvisual_options vopts = {
    .x = NCALIGN_CENTER,
    .y = NCALIGN_CENTER,
    .scaling = NCSCALE_SCALE_HIRES,
    .blitter = NCBLIT_PIXEL,
    .flags = NCVISUAL_OPTION_VERALIGNED | NCVISUAL_OPTION_HORALIGNED
              | NCVISUAL_OPTION_ADDALPHA,
  };
  int ret;
  do{
    if(make_plane(m->nc, &vopts.n)){
      return NULL;
    }
    if((frame = get_next_frame(m, &vopts)) < 0){
      ncplane_destroy(vopts.n);
      // FIXME need to cancel other one; it won't be able to progress
      return NULL;
    }
    ret = -1;
    // only one thread can render the standard pile at a time
    pthread_mutex_lock(&render_lock);
    while(*m->frame_to_render != frame){
      pthread_cond_wait(&cond, &render_lock);
    }
    int x = ncplane_x(m->slider);
    if(ncplane_move_yx(m->slider, 1, x - 1) == 0){
      ncplane_reparent(vopts.n, notcurses_stdplane(m->nc));
      ncplane_move_top(vopts.n);
      ncplane_destroy(*m->lplane);
      ret = demo_render(m->nc);
    }
    *m->frame_to_render = frame + 1;
    *m->lplane = vopts.n;
    pthread_mutex_unlock(&render_lock);
    pthread_cond_signal(&cond);
    vopts.n = NULL;
  }while(ret == 0);
  return NULL;
}

int xray_demo(struct notcurses* nc){
  if(!notcurses_canopen_videos(nc)){
    return 0;
  }
  unsigned dimx, dimy;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  ncplane_erase(notcurses_stdplane(nc));
  char* path = find_data("notcursesIII.mov");
  struct ncvisual* ncv1 = ncvisual_from_file(path);
  struct ncvisual* ncv2 = ncvisual_from_file(path);
  free(path);
  if(ncv1 == NULL || ncv2 == NULL){
    return -1;
  }
  struct ncplane* slider = make_slider(nc, dimx);
  if(slider == NULL){
    ncvisual_destroy(ncv1);
    ncvisual_destroy(ncv2);
    return -1;
  }
  uint64_t stdc = 0;
  ncchannels_set_bg_rgb(&stdc, 0);
  ncplane_set_base(notcurses_stdplane(nc), "", 0, stdc);
  // returns non-zero if the selected blitter isn't available
  pthread_t tid1, tid2;
  int last_frame = 0;
  struct ncplane* kplane = NULL; // to kill
  struct marsh m1 = {
    .slider = slider,
    .nc = nc,
    .next_frame = 0,
    .frame_to_render = &last_frame,
    .dm = notcurses_check_pixel_support(nc) ? 0 : 0.5 * delaymultiplier,
    .ncv = ncv1,
    .lplane = &kplane,
  };
  struct marsh m2 = {
    .slider = slider,
    .nc = nc,
    .next_frame = 1,
    .frame_to_render = &last_frame,
    .dm = notcurses_check_pixel_support(nc) ? 0 : 0.5 * delaymultiplier,
    .ncv = ncv2,
    .lplane = &kplane,
  };
  int ret = -1;
  if(pthread_create(&tid1, NULL, xray_thread, &m1)){
    goto err;
  }
  if(pthread_create(&tid2, NULL, xray_thread, &m2)){
    pthread_join(tid1, NULL);
    goto err;
  }
  ret = pthread_join(tid1, NULL) | pthread_join(tid2, NULL);

err:
  ncvisual_destroy(ncv1);
  ncvisual_destroy(ncv2);
  ncplane_destroy(slider);
  if(kplane){
    ncplane_destroy(kplane);
  }
  return ret;
}
