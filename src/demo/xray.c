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
  ncchannels_set_fg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
  ncchannels_set_bg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
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

static atomic_bool cancelled;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t render_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// initialized per run
static struct marsh {
  struct notcurses* nc;
  struct ncvisual* ncv;     // video stream
  // this state is guarded by the lock; always signal after an update.
  int next_frame;           // next frame to render. first thread to get the
                            // lock will be doing even frames, and the other
                            // thread will be doing odd.
  int last_frame_rendered;  // a thread cannot render the standard pile using
                            // its plane N until last_frame_rendered >= N - 1.
  int last_frame_written;   // a thread cannot rasterize and write its frame N
                            // until last_frame_written >= N - 1.
  struct ncplane* slider;   // text plane at top, sliding to the left
  float dm;                 // delay multiplier
} marsh;

// returns the index of the next frame, which can immediately begin to be
// rendered onto the thread's plane. last_frame ought be -1 the first time
// a thread calls this function.
static int
get_next_frame(struct ncvisual* ncv, struct ncvisual_options* vopts,
               int last_frame){
  int ret;
  pthread_mutex_lock(&lock);
  if(last_frame < 0){
    ret = marsh.next_frame++;
  }else{
    while(marsh.next_frame != last_frame + 2){
      pthread_cond_wait(&cond, &lock);
    }
    ++marsh.next_frame;
    ret = last_frame + 2;
    if(ncvisual_decode(ncv)){
      ret = -1;
    }else if(ncvisual_render(marsh.nc, ncv, vopts) == NULL){
      ret = -1;
    }
  }
  pthread_mutex_unlock(&lock);
  if(ret == last_frame + 2 || ret == -1){
    pthread_cond_signal(&cond);
  }
  return ret;
}

static void*
xray_thread(void *vplane){
  int frame = -1;
  struct ncvisual_options vopts = {
    .x = NCALIGN_CENTER,
    .y = NCALIGN_CENTER,
    .scaling = NCSCALE_STRETCH,
    .n = vplane,
    .blitter = NCBLIT_PIXEL,
    .flags = NCVISUAL_OPTION_VERALIGNED | NCVISUAL_OPTION_HORALIGNED
              | NCVISUAL_OPTION_ADDALPHA,
  };
  while(!cancelled){
    frame = get_next_frame(marsh.ncv, &vopts, frame);
    if(frame < 0){
      cancelled = true;
      return NULL;
    }

    // only one thread can render the standard pile at a time
    pthread_mutex_lock(&render_lock);
    while(marsh.last_frame_rendered + 1 != frame){
      pthread_cond_wait(&cond, &render_lock);
    }
    pthread_mutex_unlock(&render_lock);
    int x = ncplane_x(marsh.slider);
    if(ncplane_move_yx(marsh.slider, -1, x - 1)){
      cancelled = true;
      return NULL;
    }
    // FIXME swap our plane into stdplane, and swap old one out
    if(ncpile_render(vopts.n)){
      cancelled = true;
      return NULL;
    }

    // and only one thread can write at a time
    pthread_mutex_lock(&render_lock);
    marsh.last_frame_rendered = frame;
    pthread_cond_signal(&cond);
    while(marsh.last_frame_written + 1 != frame){
      pthread_cond_wait(&cond, &render_lock);
    }
    pthread_mutex_unlock(&render_lock);

    if(ncpile_rasterize(vopts.n)){
      cancelled = true;
      return NULL;
    }

    pthread_mutex_lock(&render_lock);
    marsh.last_frame_written = frame;
    pthread_mutex_unlock(&render_lock);
    pthread_cond_signal(&cond);
  }
  return NULL;
}

static int
perframecb(struct ncvisual* ncv, struct ncvisual_options* vopts,
           const struct timespec* tspec, void* vnewplane){
  (void)ncv;
  // only need these two steps done once, but we can't do them in
  // main() due to the plane being created in ncvisual_stream() =[
  ncplane_set_resizecb(vopts->n, ncplane_resize_maximize);
  ncplane_move_above(vopts->n, vnewplane);

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

// make two planes, both the size of the standard plane less one row at the
// bottom. the first is in the standard pile, but the second is in its own.
static int
make_planes(struct notcurses* nc, struct ncplane** t1, struct ncplane** t2){
  int dimy, dimx;
  struct ncplane* stdn = notcurses_stddim_yx(nc, &dimy, &dimx);
  // FIXME want a resizecb
  struct ncplane_options opts = {
    .rows = dimy - 1,
    .cols = dimx,
  };
  *t1 = ncplane_create(stdn, &opts);
  *t2 = ncpile_create(nc, &opts);
  if(!*t1 || !*t2){
    ncplane_destroy(*t1);
    ncplane_destroy(*t2);
    return -1;
  }
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
  struct ncplane* slider = make_slider(nc, dimy, dimx);
  if(slider == NULL){
    ncvisual_destroy(ncv);
    return -1;
  }
  uint64_t stdc = 0;
  ncchannels_set_fg_alpha(&stdc, CELL_ALPHA_TRANSPARENT);
  ncchannels_set_bg_alpha(&stdc, CELL_ALPHA_TRANSPARENT);
  ncplane_set_base(notcurses_stdplane(nc), "", 0, stdc);
  // returns non-zero if the selected blitter isn't available
  if(notcurses_check_pixel_support(nc) < 1){
    marsh.dm = 0.5 * delaymultiplier;
  }
  pthread_t tid1, tid2;
  struct ncplane* t1;
  struct ncplane* t2;
  if(make_planes(nc, &t1, &t2)){
    ncvisual_destroy(ncv);
    ncplane_destroy(slider);
    return -1;
  }
  marsh.slider = slider;
  marsh.nc = nc;
  marsh.ncv = ncv;
  marsh.next_frame = 0;
  marsh.last_frame_rendered = -1;
  marsh.last_frame_written = -1;
  cancelled = false;
  if(pthread_create(&tid1, NULL, xray_thread, t1)){
    ncvisual_destroy(ncv);
    ncplane_destroy(slider);
    ncplane_destroy(t1);
    ncplane_destroy(t2);
    return -1;
  }
  if(pthread_create(&tid2, NULL, xray_thread, t2)){
    cancelled = 1;
    pthread_join(tid1, NULL);
    ncvisual_destroy(ncv);
    ncplane_destroy(slider);
    ncplane_destroy(t1);
    ncplane_destroy(t2);
    return -1;
  }
  int ret = pthread_join(tid1, NULL) | pthread_join(tid2, NULL);
  ncvisual_destroy(ncv);
  ncplane_destroy(slider);
  ncplane_destroy(t1);
  ncplane_destroy(t2);
  return ret;
}
