#include <pthread.h>
#include "demo.h"

// the only dynamic information which needs be shared between the two threads
// is the last polyfill origin, the total filled count, and whose turn it is.

struct marsh {
  int id;                 // unique for each thread, mono-increasing
  int* turn;              // whose turn it is to prep the ncvisual, shared
  int* rturn;             // whose turn it is to render, shared
  int* polyy;             // last shared polyfill origin (y)
  int* polyx;             // last shared polyfill origin (x)
  int maxy;               // pixels, y dimension
  int maxx;               // pixels, x dimension
  uint32_t* polypixel;    // last shared polyfill pixel (rgba)
  int* filled;            // shared, how many have we filled?
  unsigned* done;         // shared, are we done?
  struct ncvisual* ncv;   // our copy of the ncv
  struct ncplane* label;  // single, shared between threads
  struct notcurses* nc;
  struct ncvisual_options vopts; // each has their own copy
  struct timespec tspec;  // delay param, copy per thread
};

// guard turn
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// guard rturn
static pthread_mutex_t rlock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t rcond = PTHREAD_COND_INITIALIZER;

static int
display(struct marsh* m, int filledcopy, long threshold_painted){
  ncplane_printf_aligned(m->label, 0, NCALIGN_CENTER, "Yield: %3.1f%%", ((double)filledcopy * 100) / threshold_painted);
  ncplane_reparent(m->vopts.n, m->label);
  ncplane_move_below(m->vopts.n, m->label);
  if(ncvisual_blit(m->nc, m->ncv, &m->vopts) == NULL){
    return -1;
  }
  if(demo_render(m->nc)){
    return -1;
  }
  *m->rturn = !*m->rturn;
  ncplane_reparent(m->vopts.n, m->vopts.n);
  pthread_mutex_unlock(&rlock);
  pthread_cond_signal(&rcond);
  pthread_mutex_lock(&lock);
  while(*m->turn != m->id && !*m->done){
    pthread_cond_wait(&cond, &lock);
  }
  return 0;
}

static int
yielder(struct marsh* m){
#define MAXITER (1024 / 2)
  int iters = 0;
  // less than this, and we exit almost immediately. more than this, and we
  // run closer to twenty seconds. 11/50 it is, then. pixels are different.
  const long threshold_painted = m->maxy * m->maxx * 11 / 50;
  if(m->id == 1){
    pthread_mutex_lock(&rlock);
    if(display(m, 0, threshold_painted)){
      *m->done = 1;
      pthread_mutex_unlock(&rlock);
      return -1;
    }
  }else{
    pthread_mutex_lock(&lock);
    while(*m->turn != m->id && !*m->done){
      pthread_cond_wait(&cond, &lock);
    }
  }
  while(!*m->done && *m->filled < threshold_painted && iters < MAXITER){
    int pfilled = 0;
    // the first time the first thread runs, it does not pick up the previous
    // polyfill origin (as there was none). all other runs, we do. fill in our
    // own copy with the other thread's move.
    if(iters || m->id){
      ncvisual_polyfill_yx(m->ncv, *m->polyy, *m->polyx, *m->polypixel);
    }
    int filledcopy;
    do{
      ++iters;
      int x = rand() % m->maxx;
      int y = rand() % m->maxy;
      ncvisual_at_yx(m->ncv, y, x, m->polypixel);
      if(ncpixel_a(*m->polypixel) != 0xff){ // don't do areas we've already done
        continue;
      }
      if(ncpixel_g(*m->polypixel) < 0x80){ // only do land, which is whiter than blue
        continue;
      }
      ncpixel_set_a(m->polypixel, 0xfe);
      ncpixel_set_rgb8(m->polypixel, (rand() % 128) + 128, 0, ncpixel_b(*m->polypixel) / 4);
      pfilled = ncvisual_polyfill_yx(m->ncv, y, x, *m->polypixel);
      if(pfilled < 0){
        break;
      }else if(pfilled){
        *m->polyy = y;
        *m->polyx = x;
      }
    }while(pfilled == 0);
    if(pfilled < 0){
      break;
    }
    *m->turn = !*m->turn;
    *m->filled += pfilled;
    if(*m->filled > threshold_painted){
      *m->filled = threshold_painted; // don't allow printing of 100.1% etc
    }
    filledcopy = *m->filled;
    pthread_mutex_unlock(&lock);
    pthread_cond_signal(&cond);

    pthread_mutex_lock(&rlock);
    while(*m->rturn != m->id && !*m->done){
      pthread_cond_wait(&rcond, &rlock);
    }
    if(display(m, filledcopy, threshold_painted)){
      pthread_mutex_unlock(&rlock);
      pthread_mutex_lock(&lock);
      break;
    }
  }
  *m->done = 1;
  pthread_mutex_unlock(&lock);
  pthread_cond_signal(&cond);
  pthread_cond_signal(&rcond);
  return 0;
#undef MAXITER
}

static void*
yielder_thread(void* vmarsh){
  struct marsh* m = vmarsh;
  if(yielder(m)){
    return NULL;
  }
  return NULL; // FIXME indicate failure/success
}

int yield_demo(struct notcurses* nc, uint64_t startns){
  (void)startns;
  if(!notcurses_canopen_images(nc)){
    return 0;
  }
  unsigned dimy, dimx;
  struct ncplane* std = notcurses_stddim_yx(nc, &dimy, &dimx);
  // in sixel-based implementation, if we redraw each cycle, the underlying
  // material will be redrawn, taking time. erasing won't eliminate the
  // flicker, but it does minimize it.
  ncplane_erase(std);
  char* pic = find_data("worldmap.png");
  struct ncvisual* v1 = ncvisual_from_file(pic);
  struct ncvisual* v2 = ncvisual_from_file(pic);
  free(pic);
  if(v1 == NULL || v2 == NULL){
    ncvisual_destroy(v1);
    ncvisual_destroy(v2);
    return -1;
  }
  // can we do bitmaps?
  const bool bitmaps = notcurses_canpixel(nc);
  struct ncplane_options nopts = {
    .y = 1,
    // this chops one line off the bottom that we could use in the case
    // of kitty graphics (xterm can't draw on the bottom row without
    // scrolling). this doesn't hit clamping thresholds since we're
    // starting on row 1. what we really need is a valid story for trimming
    // sprixels which cross the bottom row, see #2195.
    .rows = dimy - 1 - bitmaps, // FIXME
    .cols = dimx,
    .name = "wmap",
  };
  struct ncplane_options labopts = {
    .y = 3,
    .x = NCALIGN_CENTER,
    .rows = 1,
    .cols = 13,
    .name = "pcnt",
    .flags = NCPLANE_OPTION_HORALIGNED,
  };
  struct ncplane* label = ncplane_create(std, &labopts);
  if(label == NULL){
    ncvisual_destroy(v1);
    ncvisual_destroy(v2);
    return -1;
  }
  int turn = 0;
  int rturn = 1; // second thread draws the unmodified map first
  unsigned done = 0;
  int filled = 0;
  uint32_t polypixel = 0;
  int polyx = 0;
  int polyy = 0;
  struct marsh m1 = {
    .id = 0,
    .label = label,
    .nc = nc,
    .ncv = v1,
    .done = &done,
    .polyy = &polyy,
    .polyx = &polyx,
    .polypixel = &polypixel,
    .turn = &turn,
    .filled = &filled,
    .rturn = &rturn,
    .vopts = {
      .scaling = NCSCALE_STRETCH,
      .blitter = NCBLIT_PIXEL,
    },
  };
  m1.vopts.n = ncpile_create(nc, &nopts);
  if(m1.vopts.n == NULL){
    ncvisual_destroy(v1);
    ncvisual_destroy(v2);
    ncplane_destroy(label);
    return -1;
  }
  struct marsh m2 = {
    .id = 1,
    .label = label,
    .nc = nc,
    .ncv = v2,
    .done = &done,
    .polyy = &polyy,
    .polyx = &polyx,
    .polypixel = &polypixel,
    .turn = &turn,
    .filled = &filled,
    .rturn = &rturn,
    .vopts = {
      .scaling = NCSCALE_STRETCH,
      .blitter = NCBLIT_PIXEL,
    },
  };
  m2.vopts.n = ncpile_create(nc, &nopts);
  if(m2.vopts.n == NULL){
    ncvisual_destroy(v1);
    ncvisual_destroy(v2);
    ncplane_destroy(label);
    ncplane_destroy(m1.vopts.n);
    return -1;
  }
  if(bitmaps){
    timespec_div(&demodelay, 10, &m1.tspec);
    timespec_div(&demodelay, 10, &m2.tspec);
  }
  uint64_t basechan = 0;
  ncchannels_set_bg_alpha(&basechan, NCALPHA_TRANSPARENT);
  ncchannels_set_fg_alpha(&basechan, NCALPHA_TRANSPARENT);
  ncplane_set_base(label, "", 0, basechan);
  ncplane_set_bg_alpha(label, NCALPHA_TRANSPARENT);
  ncplane_set_fg_rgb8(label, 0xff, 0xff, 0xff);
  ncplane_set_styles(label, NCSTYLE_BOLD);
  ncplane_printf_aligned(label, 0, NCALIGN_CENTER, "Yield: %03.1f%%", 0.0);
  ncvgeom geom;
  if(ncvisual_geom(nc, v1, &m1.vopts, &geom)){
    ncplane_destroy(label);
    ncvisual_destroy(v1);
    ncvisual_destroy(v2);
    ncplane_destroy(m2.vopts.n);
    ncplane_destroy(m1.vopts.n);
    return -1;
  }
  // we resize the visuals ahead of time, so that we're not rescaling them
  // every time we call ncvisual_blit().
  ncvisual_resize_noninterpolative(m1.ncv, geom.rpixy, geom.rpixx);
  ncvisual_resize_noninterpolative(m2.ncv, geom.rpixy, geom.rpixx);
  m1.maxy = m2.maxy = geom.rpixy;
  m1.maxx = m2.maxx = geom.rpixx;

  pthread_t t1, t2;
  int ret = 0;
  // FIXME error checks
  pthread_create(&t1, NULL, yielder_thread, &m1);
  pthread_create(&t2, NULL, yielder_thread, &m2);
  ret = pthread_join(t1, NULL) | pthread_join(t2, NULL);

  ncplane_destroy(label);
  ncplane_reparent(m1.vopts.n, notcurses_stdplane(nc));
  ncplane_reparent(m2.vopts.n, notcurses_stdplane(nc));
  ncplane_destroy(m1.vopts.n);
  ncplane_destroy(m2.vopts.n);
  ncvisual_destroy(v1);
  ncvisual_destroy(v2);
  ncplane_erase(std);
  return ret;
}
