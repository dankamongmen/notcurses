#include "demo.h"
#include <pthread.h>
#include <stdatomic.h>

// this can take a long time, especially in large terminals; we cap execution
// at fifteen seconds (the true intended runtime), and drop frames when behind.
#define MAX_SECONDS 15

// issue #2390 adds ncvisual_frame_count(). until then...FIXME
#define VIDEO_FRAMES 862

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
  const int len = strlen(leg[0]);
  // 862 frames in the video
  const int REPS = 862 / len + dimx / len + 2;
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

struct marsh {
  struct notcurses* nc;
  struct ncvisual* ncv;     // video stream, one copy per thread
  struct ncplane* slider;   // text plane at top, sliding to the left
  float dm;                 // delay multiplier
  int next_frame;
  uint64_t startns;         // when we started (CLOCK_MONOTONIC) for frame dropping
  int* frame_to_render;     // shared; protected by renderlock
  int* dropped;             // shared; protected by renderlock
  struct ncplane** lplane;  // shared; plane to destroy, renderlocked
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
// rendered onto the thread's plane. if we're behind the clock, we don't
// bother blitting it, and drop the plane to signal as much.
static int
get_next_frame(struct marsh* m, struct ncvisual_options* vopts){
  // one does the odds, and one the evens. load two unless we're the even,
  // and it's the first frame.
  if(m->next_frame){
    if(ncvisual_decode(m->ncv)){
      return -1;
    }
  }
  if(ncvisual_decode(m->ncv)){
    return -1;
  }
  uint64_t ns = clock_getns(CLOCK_MONOTONIC);
  int ret = m->next_frame;
  uint64_t deadline = m->startns + (m->next_frame + 1) * MAX_SECONDS * (NANOSECS_IN_SEC / VIDEO_FRAMES);
  // if we've missed the deadline, drop the frame 99% of the time (you've still
  // got to draw now and again, or else there's just the initial frame hanging
  // there for ~900 frames of crap).
  if(ns > deadline && (rand() % 100)){
    ncplane_destroy(vopts->n);
    vopts->n = NULL;
  }else if(ncvisual_blit(m->nc, m->ncv, vopts) == NULL){
    return -1;
  }
  m->next_frame += 2;
  return ret;
}

static void*
xray_thread(void *vmarsh){
  struct marsh* m = vmarsh;
  struct ncplane* stdn = notcurses_stdplane(m->nc);
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
    // if we're behind where we want to be time-wise, this will return the
    // expected frame id, but vopts.n will be NULL (and the plane we created
    // will have been destroyed). that frame has been dropped.
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
      // FIXME otherwise, increment a visible drop count?
      if(vopts.n){
        ncplane_reparent(vopts.n, notcurses_stdplane(m->nc));
        ncplane_move_top(vopts.n);
        ncplane_destroy(*m->lplane);
        *m->lplane = vopts.n;
        ncplane_set_fg_rgb8_clipped(stdn, 96 + *m->dropped / 2, 0, 0x80);
        ncplane_printf_aligned(stdn, 1 + ncplane_dim_y(m->slider),
                               NCALIGN_RIGHT, "%d dropped frame%s",
                               *m->dropped, *m->dropped == 0 ? "s 🤘" :
                               *m->dropped == 1 ? " 🤔 " :
                               *m->dropped < 10 ? "s 😕" :
                               *m->dropped < 100 ? "s 😞" :
                               *m->dropped < 250 ? "s 😟" :
                               *m->dropped < 450 ? "s 😠" :
                               *m->dropped < 700 ? "s 😡" : "s 🤬");
        ret = demo_render(m->nc);
      }else{
        // FIXME i'd like to at least render the updated drop count and the
        // moved slider, but even that's too slow with stupid shitty sixel,
        // due to redrawing far too much of it see #2380
        ++*m->dropped;
        ret = 0;
      }
    }
    *m->frame_to_render = frame + 1;
    pthread_mutex_unlock(&render_lock);
    pthread_cond_signal(&cond);
    vopts.n = NULL;
  }while(ret == 0);
  return NULL;
}

int xray_demo(struct notcurses* nc, uint64_t startns){
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
  ncplane_set_bg_rgb(notcurses_stdplane(nc), 0);
  // returns non-zero if the selected blitter isn't available
  pthread_t tid1, tid2;
  int last_frame = 0;
  int dropped = 0;
  struct ncplane* kplane = NULL; // to kill
  struct marsh m1 = {
    .slider = slider,
    .nc = nc,
    .next_frame = 0,
    .frame_to_render = &last_frame,
    .dm = notcurses_check_pixel_support(nc) ? 0 : 0.5 * delaymultiplier,
    .ncv = ncv1,
    .startns = startns,
    .lplane = &kplane,
    .dropped = &dropped,
  };
  struct marsh m2 = {
    .slider = slider,
    .nc = nc,
    .next_frame = 1,
    .frame_to_render = &last_frame,
    .dm = notcurses_check_pixel_support(nc) ? 0 : 0.5 * delaymultiplier,
    .ncv = ncv2,
    .startns = startns,
    .lplane = &kplane,
    .dropped = &dropped,
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
