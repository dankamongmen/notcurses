#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include "demo.h"

#define INITIAL_TABLET_COUNT 4

static pthread_mutex_t renderlock = PTHREAD_MUTEX_INITIALIZER;

// FIXME ought just be an unordered_map
typedef struct tabletctx {
  pthread_t tid;
  struct ncreel* pr;
  struct nctablet* t;
  int lines;
  unsigned rgb;
  unsigned id;
  struct tabletctx* next;
  pthread_mutex_t lock;
} tabletctx;

static void
kill_tablet(tabletctx** tctx){
  tabletctx* t = *tctx;
  if(t){
    if(pthread_cancel(t->tid)){
      fprintf(stderr, "Warning: error sending pthread_cancel (%s)\n", strerror(errno));
    }
    if(pthread_join(t->tid, NULL)){
      fprintf(stderr, "Warning: error joining pthread (%s)\n", strerror(errno));
    }
    ncreel_del(t->pr, t->t);
    *tctx = t->next;
    pthread_mutex_destroy(&t->lock);
    free(t);
  }
}

static int
kill_active_tablet(struct ncreel* pr, tabletctx** tctx){
  struct nctablet* focused = ncreel_focused(pr);
  tabletctx* t;
  while( (t = *tctx) ){
    if(t->t == focused){
      *tctx = t->next; // pull it out of the list
      t->next = NULL; // finish splicing it out
      break;
    }
    tctx = &t->next;
  }
  if(t == NULL){
    return -1; // wasn't present in our list, wacky
  }
  kill_tablet(&t);
  return 0;
}

static int
tabletdraw(struct ncplane* w, int maxy, tabletctx* tctx, unsigned rgb){
  char cchbuf[2];
  nccell c = NCCELL_TRIVIAL_INITIALIZER;
  int y;
  int maxx = ncplane_dim_x(w) - 1;
  if(maxy > tctx->lines){
    maxy = tctx->lines;
  }
  for(y = 0 ; y < maxy ; ++y, rgb += 16){
    snprintf(cchbuf, sizeof(cchbuf) / sizeof(*cchbuf), "%x", y % 16);
    nccell_load(w, &c, cchbuf);
    if(nccell_set_fg_rgb8(&c, (rgb >> 16u) % 0xffu, (rgb >> 8u) % 0xffu, rgb % 0xffu)){
      return -1;
    }
    int x;
    for(x = 0 ; x <= maxx ; ++x){
      if(ncplane_putc_yx(w, y, x, &c) <= 0){
        return -1;
      }
    }
    nccell_release(w, &c);
  }
  return y;
}

static int
drawcb(struct nctablet* t, bool drawfromtop){
  struct ncplane* p = nctablet_plane(t);
  tabletctx* tctx = nctablet_userptr(t);
  if(tctx == NULL){
    return -1;
  }
  pthread_mutex_lock(&tctx->lock);
  unsigned rgb = tctx->rgb;
  int ll;
  int maxy = ncplane_dim_y(p);
  ll = tabletdraw(p, maxy, tctx, rgb);
  ncplane_set_fg_rgb8(p, 242, 242, 242);
  if(ll){
    const int summaryy = drawfromtop ? 0 : ll - 1;
    ncplane_on_styles(p, NCSTYLE_BOLD);
    if(ncplane_printf_yx(p, summaryy, 0, "[#%u %d lines] ",
                         tctx->id, tctx->lines) < 0){
      pthread_mutex_unlock(&tctx->lock);
      return -1;
    }
    ncplane_off_styles(p, NCSTYLE_BOLD);
  }
//fprintf(stderr, "  \\--> callback for %d, %d lines (%d/%d -> %d/%d) dir: %s wrote: %d\n", tctx->id, tctx->lines, begy, begx, maxy, maxx, cliptop ? "up" : "down", ll);
  pthread_mutex_unlock(&tctx->lock);
  return ll;
}

// Each tablet has an associated thread which will periodically send update
// events for its tablet.
static void*
tablet_thread(void* vtabletctx){
  static int MINSECONDS = 0;
  tabletctx* tctx = vtabletctx;
  while(true){
    struct timespec ts;
    ts.tv_sec = rand() % 2 + MINSECONDS;
    ts.tv_nsec = rand() % 1000000000;
    nanosleep(&ts, NULL);
    int action = rand() % 5;
    pthread_mutex_lock(&tctx->lock);
    if(action < 2){
      tctx->lines -= (action + 1);
    }else if(action > 2){
      tctx->lines += (action - 2);
    }
    if(tctx->lines < 2){
      tctx->lines = 2;
    }
    pthread_mutex_unlock(&tctx->lock);
    pthread_mutex_lock(&renderlock);
    if(nctablet_plane(tctx->t)){
      ncreel_redraw(tctx->pr);
      struct ncplane* tplane = nctablet_plane(tctx->t);
      if(tplane){
        demo_render(ncplane_notcurses(tplane));
      }
    }
    pthread_mutex_unlock(&renderlock);
  }
  return tctx;
}

static tabletctx*
new_tabletctx(struct ncreel* pr, unsigned *id){
  tabletctx* tctx = malloc(sizeof(*tctx));
  if(tctx == NULL){
    return NULL;
  }
  pthread_mutex_init(&tctx->lock, NULL);
  tctx->pr = pr;
  tctx->lines = rand() % 10 + 2; // FIXME a nice gaussian would be swell
  tctx->rgb = rand() % (1u << 24u);
  tctx->id = ++*id;
  if((tctx->t = ncreel_add(pr, NULL, NULL, drawcb, tctx)) == NULL){
    pthread_mutex_destroy(&tctx->lock);
    free(tctx);
    return NULL;
  }
  if(pthread_create(&tctx->tid, NULL, tablet_thread, tctx)){
    pthread_mutex_destroy(&tctx->lock);
    free(tctx);
    return NULL;
  }
  return tctx;
}

static uint32_t
handle_input(struct notcurses* nc, const struct timespec* deadline,
             ncinput* ni){
  int64_t deadlinens = timespec_to_ns(deadline);
  struct timespec pollspec, cur;
  clock_gettime(CLOCK_MONOTONIC, &cur);
  int64_t curns = timespec_to_ns(&cur);
  if(curns > deadlinens){
    return 0;
  }
  ns_to_timespec(deadlinens - curns, &pollspec);
  uint32_t r = demo_getc(nc, &pollspec, ni);
  return r;
}

static int
ncreel_demo_core(struct notcurses* nc, uint64_t startns){
  tabletctx* tctxs = NULL;
  bool aborted = false;
  int x = 8, y = 4;
  unsigned dimy, dimx;
  struct ncplane* std = notcurses_stddim_yx(nc, &dimy, &dimx);
  struct ncplane_options nopts = {
    .y = y,
    .x = x,
    .rows = dimy - 12,
    .cols = dimx - 16,
  };
  struct ncplane* n = ncplane_create(std, &nopts);
  if(n == NULL){
    return -1;
  }
  ncreel_options popts = {
    .bordermask = 0,
    .borderchan = 0,
    .tabletchan = 0,
    .focusedchan = 0,
    .flags = NCREEL_OPTION_INFINITESCROLL | NCREEL_OPTION_CIRCULAR,
  };
  ncchannels_set_fg_rgb8(&popts.focusedchan, 58, 150, 221);
  ncchannels_set_bg_rgb8(&popts.focusedchan, 97, 214, 214);
  ncchannels_set_fg_rgb8(&popts.tabletchan, 19, 161, 14);
  ncchannels_set_bg_rgb8(&popts.borderchan, 0, 0, 0);
  ncchannels_set_fg_rgb8(&popts.borderchan, 136, 23, 152);
  ncchannels_set_bg_rgb8(&popts.borderchan, 0, 0, 0);
  uint64_t bgchannels = 0;
  if(ncchannels_set_fg_alpha(&bgchannels, NCALPHA_TRANSPARENT)){
    ncplane_destroy(n);
    return -1;
  }
  if(ncchannels_set_bg_alpha(&bgchannels, NCALPHA_TRANSPARENT)){
    ncplane_destroy(n);
    return -1;
  }
  ncplane_set_base(n, "", 0, bgchannels);
  struct ncreel* nr = ncreel_create(n, &popts);
  if(nr == NULL){
    ncplane_destroy(n);
    return -1;
  }
  // Press a for a new nc above the current, c for a new one below the
  // current, and b for a new block at arbitrary placement.
  struct ncplane_options legendops = {
    .rows = 4,
    .cols = dimx - 2,
    .x = 0,
    .y = 0,
  };
  struct ncplane* lplane = ncplane_create(std, &legendops);
  if(lplane == NULL){
    ncreel_destroy(nr);
    return -1;
  }
  ncplane_on_styles(lplane, NCSTYLE_BOLD | NCSTYLE_ITALIC);
  ncplane_set_fg_rgb8(lplane, 58, 150, 221);
  uint64_t channels = 0;
  ncchannels_set_fg_alpha(&channels, NCALPHA_TRANSPARENT);
  ncchannels_set_bg_alpha(&channels, NCALPHA_TRANSPARENT);
  ncplane_set_base(lplane, "", 0, channels);
  ncplane_set_bg_default(lplane);
  ncplane_printf_yx(lplane, 1, 2, "a, b, c create tablets, DEL deletes.");
  ncplane_off_styles(lplane, NCSTYLE_BOLD | NCSTYLE_ITALIC);
  struct timespec deadline;
  ns_to_timespec((timespec_to_ns(&demodelay) * 5) + startns, &deadline);
  unsigned id = 0;
  struct tabletctx* newtablet;
  // Make an initial number of tablets suitable for the screen's height
  while(id < dimy / 8u){
    newtablet = new_tabletctx(nr, &id);
    if(newtablet == NULL){
      ncreel_destroy(nr);
      ncplane_destroy(lplane);
      return -1;
    }
    newtablet->next = tctxs;
    tctxs = newtablet;
  }
  do{
    ncplane_set_styles(lplane, NCSTYLE_NONE);
    ncplane_set_fg_rgb8(lplane, 197, 15, 31);
    int count = ncreel_tabletcount(nr);
    ncplane_on_styles(lplane, NCSTYLE_BOLD);
    ncplane_printf_yx(lplane, 2, 2, "%d tablet%s", count, count == 1 ? "" : "s");
    ncplane_off_styles(lplane, NCSTYLE_BOLD);
    ncplane_set_fg_rgb8(lplane, 0, 55, 218);
    uint32_t rw;
    ncinput ni;
    pthread_mutex_lock(&renderlock);
    ncreel_redraw(nr);
    int renderret;
    renderret = demo_render(nc);
    pthread_mutex_unlock(&renderlock);
    if(renderret){
      while(tctxs){
        kill_tablet(&tctxs);
      }
      ncreel_destroy(nr);
      ncplane_destroy(lplane);
      return renderret;
    }
    if((rw = handle_input(nc, &deadline, &ni)) == (uint32_t)-1){
      break;
    }
    // FIXME clrtoeol();
    newtablet = NULL;
    switch(rw){
      case 'a': newtablet = new_tabletctx(nr, &id); break;
      case 'b': newtablet = new_tabletctx(nr, &id); break;
      case 'c': newtablet = new_tabletctx(nr, &id); break;
      case 'k': ncreel_prev(nr); break;
      case 'j': ncreel_next(nr); break;
      case 'q': aborted = true; break;
      case NCKEY_UP: ncreel_prev(nr); break;
      case NCKEY_DOWN: ncreel_next(nr); break;
      case NCKEY_LEFT:
        ncplane_yx(ncreel_plane(nr), &y, &x);
        ncplane_move_yx(ncreel_plane(nr), y, x - 1);
        break;
      case NCKEY_RIGHT:
        ncplane_yx(ncreel_plane(nr), &y, &x);
        ncplane_move_yx(ncreel_plane(nr), y, x + 1);
        break;
      case NCKEY_DEL: kill_active_tablet(nr, &tctxs); break;
      case NCKEY_RESIZE: notcurses_render(nc); break;
      default: ncplane_printf_yx(lplane, 3, 2, "Unknown keycode (0x%lx)\n", (unsigned long)rw); break;
    }
    if(newtablet){
      newtablet->next = tctxs;
      tctxs = newtablet;
    }
    struct timespec cur;
    clock_gettime(CLOCK_MONOTONIC, &cur);
    if(timespec_subtract_ns(&cur, &deadline) >= 0){
      break;
    }
    dimy = ncplane_dim_y(n);
  }while(!aborted);
  while(tctxs){
    kill_tablet(&tctxs);
  }
  ncreel_destroy(nr);
  ncplane_destroy(lplane);
  return aborted ? 1 : 0;
}

int reel_demo(struct notcurses* nc, uint64_t startns){
  ncplane_greyscale(notcurses_stdplane(nc));
  int ret = ncreel_demo_core(nc, startns);
  return ret;
}
