#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <sys/poll.h>
#include <notcurses.h>
#include <sys/eventfd.h>
#include "demo.h"

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
    ncplane_cursor_move_yx(w, y, begx);
    snprintf(cchbuf, sizeof(cchbuf) / sizeof(*cchbuf), "%x", idx % 16);
    cell_load(w, &c, cchbuf);
    cell_set_fg(&c, (rgb >> 16u) % 0xffu, (rgb >> 8u) % 0xffu, rgb % 0xffu);
    int x;
    for(x = begx ; x <= maxx ; ++x){
      // lower-right corner always returns an error unless scrollok() is used
      ncplane_putc(w, &c);
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
    ncplane_cursor_move_yx(w, y, begx);
    snprintf(cchbuf, sizeof(cchbuf) / sizeof(*cchbuf), "%x", y % 16);
    cell_load(w, &c, cchbuf);
    cell_set_fg(&c, (rgb >> 16u) % 0xffu, (rgb >> 8u) % 0xffu, rgb % 0xffu);
    int x;
    for(x = begx ; x <= maxx ; ++x){
      // lower-right corner always returns an error unless scrollok() is used
      ncplane_putc(w, &c);
    }
    cell_release(w, &c);
  }
  return y - begy;
}

static int
tabletdraw(struct ncplane* p, int begx, int begy, int maxx, int maxy,
           bool cliptop, void* vtabletctx){
  int err = 0;
  tabletctx* tctx = vtabletctx;
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
    err |= ncplane_cursor_move_yx(p, summaryy, begx);
    ncplane_printf(p, "[#%u %d line%s %u/%u] ", tctx->id, tctx->lines,
                          tctx->lines == 1 ? "" : "s", begy, maxy);
  }
/*fprintf(stderr, "  \\--> callback for %d, %d lines (%d/%d -> %d/%d) dir: %s wrote: %d ret: %d\n", tctx->id,
    tctx->lines, begy, begx, maxy, maxx,
    cliptop ? "up" : "down", ll, err);*/
  pthread_mutex_unlock(&tctx->lock);
  assert(0 == err);
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
handle_input(struct notcurses* nc, struct panelreel* pr, int efd){
  struct pollfd fds[2] = {
    { .fd = STDIN_FILENO, .events = POLLIN, .revents = 0, },
    { .fd = efd,          .events = POLLIN, .revents = 0, },
  };
  wchar_t key = -1;
  int pret;
  notcurses_render(nc);
  do{
    pret = poll(fds, sizeof(fds) / sizeof(*fds), -1);
    if(pret < 0){
      fprintf(stderr, "Error polling on stdin/eventfd (%s)\n", strerror(errno));
    }else{
      if(fds[0].revents & POLLIN){
        key = notcurses_getc_blocking(nc);
        if(key < 0){
          return -1;
        }
      }
      if(fds[1].revents & POLLIN){
        uint64_t val;
        if(read(efd, &val, sizeof(val)) != sizeof(val)){
          fprintf(stderr, "Error reading from eventfd %d (%s)\n", efd, strerror(errno)); }else if(key < 0){
          panelreel_redraw(pr);
          notcurses_render(nc);
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
    .bordermask = NCBOXMASK_BOTTOM | NCBOXMASK_TOP |
                  NCBOXMASK_LEFT | NCBOXMASK_RIGHT,
    .borderattr = CELL_TRIVIAL_INITIALIZER,
    .tabletattr = CELL_TRIVIAL_INITIALIZER,
    .focusedattr = CELL_TRIVIAL_INITIALIZER,
    .toff = y,
    .loff = x,
    .roff = x,
    .boff = y,
    .bgchannel = 0,
  };
  cell_set_fg(&popts.focusedattr, 58, 150, 221);
  cell_set_bg(&popts.focusedattr, 97, 214, 214);
  cell_set_fg(&popts.tabletattr, 19, 161, 14);
  cell_set_fg(&popts.borderattr, 136, 23, 152);
  cell_set_bg(&popts.borderattr, 0, 0, 0);
  if(notcurses_bg_set_alpha(&popts.bgchannel, 3)){
    return NULL;
  }
  struct ncplane* w = notcurses_stdplane(nc);
  struct panelreel* pr = panelreel_create(w, &popts, efd);
  if(pr == NULL){
    fprintf(stderr, "Error creating panelreel\n");
    return NULL;
  }
  // Press a for a new panel above the current, c for a new one below the
  // current, and b for a new block at arbitrary placement. q quits.
  ncplane_set_fg_rgb(w, 58, 150, 221);
  ncplane_bg_default(w);
  ncplane_cursor_move_yx(w, 1, 1);
  ncplane_printf(w, "a, b, c create tablets, DEL deletes, q quits.");
  // FIXME clrtoeol();
  /*
  struct timespec fadets = { .tv_sec = 1, .tv_nsec = 0, };
  if(ncplane_fadein(panelreel_plane(pr), &fadets)){
    return NULL;
  }
  */
  unsigned id = 0;
  do{
    ncplane_styles_set(w, 0);
    ncplane_set_fg_rgb(w, 197, 15, 31);
    int count = panelreel_tabletcount(pr);
    ncplane_cursor_move_yx(w, 2, 2);
    ncplane_printf(w, "%d tablet%s", count, count == 1 ? "" : "s");
    // FIXME wclrtoeol(w);
    ncplane_set_fg_rgb(w, 0, 55, 218);
    wchar_t rw;
    if((rw = handle_input(nc, pr, efd)) < 0){
      done = true;
      break;
    }
    // FIXME clrtoeol();
    struct tabletctx* newtablet = NULL;
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
        ncplane_cursor_move_yx(w, 3, 2);
        ncplane_printf(w, "Unknown keycode (0x%x)\n", rw);
    }
    if(newtablet){
      newtablet->next = *tctxs;
      *tctxs = newtablet;
    }
    //panelreel_validate(w, pr); // do what, if not assert()ing? FIXME
  }while(!done);
  return pr;
}

int panelreel_demo(struct notcurses* nc){
  tabletctx* tctxs = NULL;
  int efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
  if(efd < 0){
    fprintf(stderr, "Error creating eventfd (%s)\n", strerror(errno));
    return -1;
  }
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
  if(notcurses_render(nc)){
    return -1;
  }
  return 0;
}
