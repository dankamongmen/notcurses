#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <sys/poll.h>
#include <notcurses.h>
#include "demo.h"

#define INITIAL_TABLET_COUNT 4

// FIXME ought just be an unordered_map
typedef struct tabletctx {
  pthread_t tid;
  struct panelreel* pr;
  struct tablet* t;
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
    panelreel_del(t->pr, t->t);
    *tctx = t->next;
    pthread_mutex_destroy(&t->lock);
    free(t);
  }
}

static int
kill_active_tablet(struct panelreel* pr, tabletctx** tctx){
  struct tablet* focused = panelreel_focused(pr);
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

// We need write in reverse order (since only the bottom will be seen, if we're
// partially off-screen), but also leave unused space at the end (since
// wresize() only keeps the top and left on a shrink).
static int
tabletup(struct ncplane* w, int begx, int begy, int maxx, int maxy,
         tabletctx* tctx, int rgb){
  char cchbuf[2];
  cell c = CELL_TRIVIAL_INITIALIZER;
  int y, idx;
  idx = tctx->lines;
  if(maxy - begy > tctx->lines){
    maxy -= (maxy - begy - tctx->lines);
  }
/*fprintf(stderr, "-OFFSET BY %d (%d->%d)\n", maxy - begy - tctx->lines,
        maxy, maxy - (maxy - begy - tctx->lines));*/
  for(y = maxy ; y >= begy ; --y, rgb += 16){
    snprintf(cchbuf, sizeof(cchbuf) / sizeof(*cchbuf), "%x", idx % 16);
    cell_load(w, &c, cchbuf);
    if(cell_set_fg_rgb(&c, (rgb >> 16u) % 0xffu, (rgb >> 8u) % 0xffu, rgb % 0xffu)){
      return -1;
    }
    int x;
    for(x = begx ; x <= maxx ; ++x){
      if(ncplane_putc_yx(w, y, x, &c) <= 0){
        return -1;
      }
    }
    cell_release(w, &c);
    if(--idx == 0){
      break;
    }
  }
// fprintf(stderr, "tabletup done%s at %d (%d->%d)\n", idx == 0 ? " early" : "", y, begy, maxy);
  return tctx->lines - idx;
}

static int
tabletdown(struct ncplane* w, int begx, int begy, int maxx, int maxy,
           tabletctx* tctx, unsigned rgb){
  char cchbuf[2];
  cell c = CELL_TRIVIAL_INITIALIZER;
  int y;
  for(y = begy ; y <= maxy ; ++y, rgb += 16){
    if(y - begy >= tctx->lines){
      break;
    }
    snprintf(cchbuf, sizeof(cchbuf) / sizeof(*cchbuf), "%x", y % 16);
    cell_load(w, &c, cchbuf);
    if(cell_set_fg_rgb(&c, (rgb >> 16u) % 0xffu, (rgb >> 8u) % 0xffu, rgb % 0xffu)){
      return -1;
    }
    int x;
    for(x = begx ; x <= maxx ; ++x){
      if(ncplane_putc_yx(w, y, x, &c) <= 0){
        return -1;
      }
    }
    cell_release(w, &c);
  }
  return y - begy;
}

static int
tabletdraw(struct tablet* t, int begx, int begy, int maxx, int maxy, bool cliptop){
  struct ncplane* p = tablet_ncplane(t);
  tabletctx* tctx = tablet_userptr(t);
  pthread_mutex_lock(&tctx->lock);
  unsigned rgb = tctx->rgb;
  int ll;
  if(cliptop){
    ll = tabletup(p, begx, begy, maxx, maxy, tctx, rgb);
  }else{
    ll = tabletdown(p, begx, begy, maxx, maxy, tctx, rgb);
  }
  ncplane_set_fg_rgb(p, 242, 242, 242);
  if(ll){
    int summaryy = begy;
    if(cliptop){
      if(ll == maxy - begy + 1){
        summaryy = ll - 1;
      }else{
        summaryy = ll;
      }
    }
    ncplane_styles_on(p, CELL_STYLE_BOLD);
    if(ncplane_printf_yx(p, summaryy, begx, "[#%u %d line%s %u/%u] ",
                         tctx->id, tctx->lines, tctx->lines == 1 ? "" : "s",
                         begy, maxy) < 0){
      pthread_mutex_unlock(&tctx->lock);
      return -1;
    }
    ncplane_styles_off(p, CELL_STYLE_BOLD);
  }
/*fprintf(stderr, "  \\--> callback for %d, %d lines (%d/%d -> %d/%d) dir: %s wrote: %d ret: %d\n", tctx->id,
    tctx->lines, begy, begx, maxy, maxx,
    cliptop ? "up" : "down", ll, err);*/
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
    ts.tv_sec = random() % 3 + MINSECONDS;
    ts.tv_nsec = random() % 1000000000;
    nanosleep(&ts, NULL);
    int action = random() % 5;
    pthread_mutex_lock(&tctx->lock);
    if(action < 2){
      if((tctx->lines -= (action + 1)) < 1){
        tctx->lines = 1;
      }
      panelreel_touch(tctx->pr, tctx->t);
    }else if(action > 2){
      if((tctx->lines += (action - 2)) < 1){
        tctx->lines = 1;
      }
      panelreel_touch(tctx->pr, tctx->t);
    }
    pthread_mutex_unlock(&tctx->lock);
  }
  return tctx;
}

static tabletctx*
new_tabletctx(struct panelreel* pr, unsigned *id){
  tabletctx* tctx = malloc(sizeof(*tctx));
  if(tctx == NULL){
    return NULL;
  }
  pthread_mutex_init(&tctx->lock, NULL);
  tctx->pr = pr;
  tctx->lines = random() % 10 + 1; // FIXME a nice gaussian would be swell
  tctx->rgb = random() % (1u << 24u);
  tctx->id = ++*id;
  if((tctx->t = panelreel_add(pr, NULL, NULL, tabletdraw, tctx)) == NULL){
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

static wchar_t
handle_input(struct notcurses* nc, struct panelreel* pr, int efd,
             const struct timespec* deadline){
  struct pollfd fds[2] = {
    { .fd = STDIN_FILENO, .events = POLLIN, .revents = 0, },
    { .fd = efd,          .events = POLLIN, .revents = 0, },
  };
  sigset_t sset;
  sigemptyset(&sset);
  wchar_t key = -1;
  int pret;
  demo_render(nc);
  do{
    struct timespec pollspec, cur;
    clock_gettime(CLOCK_MONOTONIC, &cur);
    timespec_subtract(&pollspec, deadline, &cur);
    pret = ppoll(fds, sizeof(fds) / sizeof(*fds), &pollspec, &sset);
    if(pret == 0){
      return 0;
    }else if(pret < 0){
      fprintf(stderr, "Error polling on stdin/eventfd (%s)\n", strerror(errno));
      return (wchar_t)-1;
    }else{
      if(fds[0].revents & POLLIN){
        key = demo_getc_blocking(NULL);
        if(key < 0){
          return -1;
        }
      }
      if(fds[1].revents & POLLIN){
        uint64_t val;
        if(read(efd, &val, sizeof(val)) != sizeof(val)){
          fprintf(stderr, "Error reading from eventfd %d (%s)\n", efd, strerror(errno)); }else if(key < 0){
          panelreel_redraw(pr);
          demo_render(nc);
        }
      }
    }
  }while(key < 0);
  return key;
}

static struct panelreel*
panelreel_demo_core(struct notcurses* nc, int efd, tabletctx** tctxs){
  bool done = false;
  int x = 8, y = 4;
  panelreel_options popts = {
    .infinitescroll = true,
    .circular = true,
    .min_supported_cols = 8,
    .min_supported_rows = 5,
    .bordermask = 0,
    .borderchan = 0,
    .tabletchan = 0,
    .focusedchan = 0,
    .toff = y,
    .loff = x,
    .roff = x,
    .boff = y,
    .bgchannel = 0,
  };
  channels_set_fg_rgb(&popts.focusedchan, 58, 150, 221);
  channels_set_bg_rgb(&popts.focusedchan, 97, 214, 214);
  channels_set_fg_rgb(&popts.tabletchan, 19, 161, 14);
  channels_set_fg_rgb(&popts.borderchan, 136, 23, 152);
  channels_set_bg_rgb(&popts.borderchan, 0, 0, 0);
  if(channels_set_fg_alpha(&popts.bgchannel, CELL_ALPHA_TRANSPARENT)){
    return NULL;
  }
  if(channels_set_bg_alpha(&popts.bgchannel, CELL_ALPHA_TRANSPARENT)){
    return NULL;
  }
  struct ncplane* w = notcurses_stdplane(nc);
  struct panelreel* pr = panelreel_create(w, &popts, efd);
  if(pr == NULL){
    fprintf(stderr, "Error creating panelreel\n");
    return NULL;
  }
  // Press a for a new panel above the current, c for a new one below the
  // current, and b for a new block at arbitrary placement.
  ncplane_styles_on(w, CELL_STYLE_BOLD | CELL_STYLE_ITALIC);
  ncplane_set_fg_rgb(w, 58, 150, 221);
  ncplane_set_bg_default(w);
  ncplane_printf_yx(w, 1, 1, "a, b, c create tablets, DEL deletes.");
  ncplane_styles_off(w, CELL_STYLE_BOLD | CELL_STYLE_ITALIC);
  // FIXME clrtoeol();
  struct timespec deadline;
  clock_gettime(CLOCK_MONOTONIC, &deadline);
  ns_to_timespec((timespec_to_ns(&demodelay) * 5) + timespec_to_ns(&deadline),
                 &deadline);
  unsigned id = 0;
  struct tabletctx* newtablet;
  int dimy = ncplane_dim_y(w);
  // Make an initial number of tablets suitable for the screen's height
  while(id < dimy / 8u){
    newtablet = new_tabletctx(pr, &id);
    if(newtablet == NULL){
      return NULL;
    }
    newtablet->next = *tctxs;
    *tctxs = newtablet;
  }
  do{
    ncplane_styles_set(w, 0);
    ncplane_set_fg_rgb(w, 197, 15, 31);
    int count = panelreel_tabletcount(pr);
    ncplane_styles_on(w, CELL_STYLE_BOLD);
    ncplane_printf_yx(w, 2, 2, "%d tablet%s", count, count == 1 ? "" : "s");
    ncplane_styles_off(w, CELL_STYLE_BOLD);
    // FIXME wclrtoeol(w);
    ncplane_set_fg_rgb(w, 0, 55, 218);
    wchar_t rw;
    if((rw = handle_input(nc, pr, efd, &deadline)) <= 0){
      done = true;
      break;
    }
    // FIXME clrtoeol();
    newtablet = NULL;
    switch(rw){
      case 'p': sleep(60); exit(EXIT_FAILURE); break;
      case 'a': newtablet = new_tabletctx(pr, &id); break;
      case 'b': newtablet = new_tabletctx(pr, &id); break;
      case 'c': newtablet = new_tabletctx(pr, &id); break;
      case 'h': --x; if(panelreel_move(pr, x, y)){ ++x; } break;
      case 'l': ++x; if(panelreel_move(pr, x, y)){ --x; } break;
      case 'k': panelreel_prev(pr); break;
      case 'j': panelreel_next(pr); break;
      case 'q': done = true; break;
      case NCKEY_LEFT: --x; if(panelreel_move(pr, x, y)){ ++x; } break;
      case NCKEY_RIGHT: ++x; if(panelreel_move(pr, x, y)){ --x; } break;
      case NCKEY_UP: panelreel_prev(pr); break;
      case NCKEY_DOWN: panelreel_next(pr); break;
      case NCKEY_DEL: kill_active_tablet(pr, tctxs); break;
      default:
        ncplane_printf_yx(w, 3, 2, "Unknown keycode (0x%x)\n", rw);
    }
    if(newtablet){
      newtablet->next = *tctxs;
      *tctxs = newtablet;
    }
    struct timespec cur;
    clock_gettime(CLOCK_MONOTONIC, &cur);
    if(timespec_subtract_ns(&cur, &deadline) >= 0){
      break;
    }
    //panelreel_validate(w, pr); // do what, if not assert()ing? FIXME
  }while(!done);
  return pr;
}

int panelreel_demo(struct notcurses* nc){
  tabletctx* tctxs = NULL;
  /* FIXME there's no eventfd on FreeBSD, so until we do a self-pipe
   * trick here or something, just pass -1. it means higher latency
   * on our keyboard events in this demo. oh well.
  int efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
  if(efd < 0){
    fprintf(stderr, "Error creating eventfd (%s)\n", strerror(errno));
    return -1;
  }*/
  int efd = -1;
  struct panelreel* pr;
  if((pr = panelreel_demo_core(nc, efd, &tctxs)) == NULL){
    close(efd);
    return -1;
  }
  while(tctxs){
    kill_tablet(&tctxs);
  }
  close(efd);
  if(panelreel_destroy(pr)){
    fprintf(stderr, "Error destroying panelreel\n");
    return -1;
  }
  close(efd);
  if(demo_render(nc)){
    return -1;
  }
  return 0;
}
