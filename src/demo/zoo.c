#include "demo.h"
#include <pthread.h>

static int
locked_demo_render(struct notcurses* nc, pthread_mutex_t* lock){
  int ret;

  if(pthread_mutex_lock(lock)){
    return -1;
  }
  ret = demo_render(nc);
  if(pthread_mutex_unlock(lock)){
    return -1;
  }
  return ret;
}

// we list all distributions on which notcurses is known to exist
static struct ncselector_item select_items[] = {
#define SITEM(short, long) { short, long, 0, 0, }
  SITEM("fbsd", "FreeBSD"),
  SITEM("deb", "Debian Unstable Linux"),
  SITEM("rpm", "Fedora Rawhide Linux"),
  SITEM("pac", "Arch Linux"),
  SITEM("apk", "Alpine Edge Linux"),
  SITEM(NULL, NULL),
#undef SITEM
};

static struct ncmselector_item mselect_items[] = {
  { "Pa231", "Protactinium-231 (162kg)", .selected = false, },
  { "U233", "Uranium-233 (15kg)", .selected = false, },
  { "U235", "Uranium-235 (50kg)", .selected = false, },
  { "Np236", "Neptunium-236 (7kg)", .selected = false, },
  { "Np237", "Neptunium-237 (60kg)", .selected = false, },
  { "Pu238", "Plutonium-238 (10kg)", .selected = false, },
  { "Pu239", "Plutonium-239 (10kg)", .selected = false, },
  { "Pu240", "Plutonium-240 (40kg)", .selected = false, },
  { "Pu241", "Plutonium-241 (13kg)", .selected = false, },
  { "Am241", "Americium-241 (100kg)", .selected = false, },
  { "Pu242", "Plutonium-242 (100kg)", .selected = false, },
  { "Am242", "Americium-242 (18kg)", .selected = false, },
  { "Am243", "Americium-243 (155kg)", .selected = false, },
  { "Cm243", "Curium-243 (10kg)", .selected = false, },
  { "Cm244", "Curium-244 (30kg)", .selected = false, },
  { "Cm245", "Curium-245 (13kg)", .selected = false, },
  { "Cm246", "Curium-246 (84kg)", .selected = false, },
  { "Cm247", "Curium-247 (7kg)", .selected = false, },
  { "Bk247", "Berkelium-247 (10kg)", .selected = false, },
  { "Cf249", "Californium-249 (6kg)", .selected = false, },
  { "Cf251", "Californium-251 (9kg)", .selected = false, },
  { NULL, NULL, .selected = false, },
};

static struct ncmultiselector*
multiselector_demo(struct ncplane* n, int dimx, int y, pthread_mutex_t* lock){
  struct notcurses* nc = ncplane_notcurses(n);
  ncmultiselector_options mopts = {
    .maxdisplay = 8,
    .title = "multi-item selector",
    .items = mselect_items,
    .boxchannels = CHANNELS_RGB_INITIALIZER(0x20, 0xe0, 0xe0, 0x20, 0, 0),
    .opchannels = CHANNELS_RGB_INITIALIZER(0xe0, 0x80, 0x40, 0, 0, 0),
    .descchannels = CHANNELS_RGB_INITIALIZER(0x80, 0xe0, 0x40, 0, 0, 0),
    .footchannels = CHANNELS_RGB_INITIALIZER(0xe0, 0, 0x40, 0x20, 0x20, 0),
    .titlechannels = CHANNELS_RGB_INITIALIZER(0x20, 0xff, 0xff, 0, 0, 0x20),
    .bgchannels = CHANNELS_RGB_INITIALIZER(0, 0x40, 0, 0, 0x40, 0),
  };
  channels_set_fg_alpha(&mopts.bgchannels, CELL_ALPHA_BLEND);
  channels_set_bg_alpha(&mopts.bgchannels, CELL_ALPHA_BLEND);
  pthread_mutex_lock(lock);
  struct ncmultiselector* mselector = ncmultiselector_create(n, y, 0, &mopts);
  pthread_mutex_unlock(lock);
  if(mselector == NULL){
    return NULL;
  }
  struct timespec swoopdelay;
  timespec_div(&demodelay, dimx / 3, &swoopdelay);
  pthread_mutex_lock(lock);
  struct ncplane* mplane = ncmultiselector_plane(mselector);
  int length = ncplane_dim_x(mplane);
  ncplane_move_yx(mplane, y, -length);
  pthread_mutex_unlock(lock);
  ncinput ni;
  for(int i = -length + 1 ; i < dimx - (length + 1) ; ++i){
    if(locked_demo_render(nc, lock)){
      ncmultiselector_destroy(mselector);
      return NULL;
    }
    ncplane_move_yx(mplane, y, i);
    char32_t wc = demo_getc(nc, &swoopdelay, &ni);
    if(wc == (char32_t)-1){
      ncmultiselector_destroy(mselector);
      return NULL;
    }else if(wc){
      ncmultiselector_offer_input(mselector, &ni);
    }
  }
  if(locked_demo_render(nc, lock)){
    ncmultiselector_destroy(mselector);
    return NULL;
  }
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  uint64_t cur = timespec_to_ns(&ts);
  uint64_t targ = cur + timespec_to_ns(&demodelay);
  do{
    struct timespec rel;
    ns_to_timespec(targ - cur, &rel);
    char32_t wc = demo_getc(nc, &rel, &ni);
    if(wc == (char32_t)-1){
      ncmultiselector_destroy(mselector);
      return NULL;
    }else if(wc){
      ncmultiselector_offer_input(mselector, &ni);
    }
    clock_gettime(CLOCK_MONOTONIC, &ts);
    cur = timespec_to_ns(&ts);
  }while(cur < targ);
  return mselector;
}

static int
draw_background(struct notcurses* nc){
  if(notcurses_canopen_images(nc)){
    struct ncplane* n = notcurses_stdplane(nc);
    nc_err_e err;
    char* path = find_data("changes.jpg");
    struct ncvisual* ncv = ncvisual_from_file(path, &err);
    free(path);
    if(!ncv){
      return -1;
    }
    struct ncvisual_options vopts = {
      .scaling = NCSCALE_STRETCH,
      .n = n,
    };
    if(ncvisual_render(nc, ncv, &vopts) == NULL){
      ncvisual_destroy(ncv);
      return -1;
    }
    ncplane_greyscale(n);
  }
  return 0;
}

static struct ncselector*
selector_demo(struct ncplane* n, int dimx, int y, pthread_mutex_t* lock){
  struct notcurses* nc = ncplane_notcurses(n);
  ncselector_options sopts = {
    .title = "single-item selector",
    .items = select_items,
    .defidx = 4,
    .maxdisplay = 3,
    .boxchannels = CHANNELS_RGB_INITIALIZER(0xe0, 0x20, 0x40, 0x20, 0x20, 0x20),
    .opchannels = CHANNELS_RGB_INITIALIZER(0xe0, 0x80, 0x40, 0, 0, 0),
    .descchannels = CHANNELS_RGB_INITIALIZER(0x80, 0xe0, 0x40, 0, 0, 0),
    .footchannels = CHANNELS_RGB_INITIALIZER(0xe0, 0, 0x40, 0x20, 0, 0),
    .titlechannels = CHANNELS_RGB_INITIALIZER(0xff, 0xff, 0x80, 0, 0, 0x20),
    .bgchannels = CHANNELS_RGB_INITIALIZER(0, 0, 0x40, 0, 0, 0x40),
  };
  channels_set_fg_alpha(&sopts.bgchannels, CELL_ALPHA_BLEND);
  channels_set_bg_alpha(&sopts.bgchannels, CELL_ALPHA_BLEND);
  pthread_mutex_lock(lock);
  struct ncselector* selector = ncselector_create(n, y, dimx, &sopts);
  pthread_mutex_unlock(lock);
  if(selector == NULL){
    return NULL;
  }
  struct ncplane* splane = ncselector_plane(selector);
  struct timespec swoopdelay;
  timespec_div(&demodelay, dimx / 3, &swoopdelay);
  ncinput ni;
  for(int i = dimx - 1 ; i > 1 ; --i){
    pthread_mutex_lock(lock);
      demo_render(nc);
      ncplane_move_yx(splane, y, i);
    pthread_mutex_unlock(lock);
    char32_t wc = demo_getc(nc, &swoopdelay, &ni);
    if(wc == (char32_t)-1){
      ncselector_destroy(selector, NULL);
      return NULL;
    }else if(wc){
      pthread_mutex_lock(lock);
        ncselector_offer_input(selector, &ni);
      pthread_mutex_unlock(lock);
    }
  }
  if(locked_demo_render(nc, lock)){
    ncselector_destroy(selector, NULL);
    return NULL;
  }
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  uint64_t cur = timespec_to_ns(&ts);
  uint64_t targ = cur + timespec_to_ns(&demodelay);
  do{
    struct timespec rel;
    ns_to_timespec(targ - cur, &rel);
    char32_t wc = demo_getc(nc, &rel, &ni);
    if(wc == (char32_t)-1){
      ncselector_destroy(selector, NULL);
      return NULL;
    }else if(wc){
      pthread_mutex_lock(lock);
        ncselector_offer_input(selector, &ni);
      pthread_mutex_unlock(lock);
    }
    clock_gettime(CLOCK_MONOTONIC, &ts);
    cur = timespec_to_ns(&ts);
  }while(cur < targ);
  return selector;
}

typedef struct read_marshal {
  struct notcurses* nc;
  struct ncreader* reader;
  pthread_mutex_t* lock;
} read_marshal;

static void*
reader_thread(void* vmarsh){
  // FIXME use ncplane_puttext() to handle word breaking; this is ugly
  const char text[] =
    "Notcurses provides several widgets to quickly build vivid TUIs.\n\n"
    "This NCReader widget facilitates free-form text entry complete with readline-style bindings. "
    "NCSelector allows a single option to be selected from a list. "
    "NCMultiselector allows 0..n options to be selected from a list of n items. "
    "NCFdplane streams a file descriptor, while NCSubproc spawns a subprocess and streams its output. "
    "A variety of plots are supported, and menus can be placed along the top and/or bottom of any plane.\n\n"
    "Widgets can be controlled with the keyboard and/or mouse. They are implemented atop ncplanes, and these planes can be manipulated like all others.";
  const size_t textlen = strlen(text);
  read_marshal* marsh = vmarsh;
  struct notcurses* nc = marsh->nc;
  struct ncreader* reader = marsh->reader;
  pthread_mutex_t* lock = marsh->lock; 
  free(marsh);
  int x, y;
  struct ncplane* rplane = ncreader_plane(reader);
  struct timespec rowdelay;
  ncplane_yx(rplane, &y, &x);
  int targrow = y / 2;
  // the other widgets divide the movement range by 3 (and thus take about 3
  // demodelays to transit). take about 3 demodelays to rise to midscreen. this
  // also affects the "typing" speed.
  timespec_div(&demodelay, (y - targrow) / 3, &rowdelay);
  // we usually won't be done rendering the text before reaching our target row
  size_t textpos = 0;
  const int TOWRITEMAX = 4; // FIXME throw in some jitter!
  while(y > targrow){
    pthread_mutex_lock(lock);
      if(demo_render(nc)){
        pthread_mutex_unlock(lock);
        return NULL; // FIXME
      }
      ncplane_move_yx(rplane, --y, x);
      size_t towrite = textlen - textpos;
      if(towrite > TOWRITEMAX){
        towrite = TOWRITEMAX;
      }
      if(towrite){
        ncplane_putnstr(rplane, towrite, text + textpos);
        textpos += towrite;
      }
    pthread_mutex_unlock(lock);
    clock_nanosleep(CLOCK_MONOTONIC, 0, &rowdelay, NULL);
  }
  while(textpos < textlen){
    pthread_mutex_lock(lock);
      if(demo_render(nc)){
        pthread_mutex_unlock(lock);
        return NULL; // FIXME
      }
      size_t towrite = textlen - textpos;
      if(towrite > TOWRITEMAX){
        towrite = TOWRITEMAX;
      }
      if(towrite){
        ncplane_putnstr(rplane, towrite, text + textpos);
        textpos += towrite;
      }
    pthread_mutex_unlock(lock);
    clock_nanosleep(CLOCK_MONOTONIC, 0, &rowdelay, NULL);
  }
  // FIXME unsafe if other widgets aren't yet done!
  demo_nanosleep(nc, &demodelay);
  // FIXME want to communicate exceptional exit vs successful run...
  return NULL;
}

// creates an ncreader, and spawns a thread which will fill it with text
// describing the rest of the demo
static struct ncreader*
reader_demo(struct notcurses* nc, pthread_t* tid, pthread_mutex_t* lock){
  read_marshal* marsh = malloc(sizeof(*marsh));
  if(marsh == NULL){
    return NULL;
  }
  marsh->nc = nc;
  marsh->lock = lock;
  int dimy;
  struct ncplane* std = notcurses_stddim_yx(nc, &dimy, NULL);
  const int READER_COLS = 64;
  const int READER_ROWS = 8;
  ncreader_options nopts = {
    .echannels = CHANNELS_RGB_INITIALIZER(0x20, 0xe0, 0xe0, 0, 0, 0),
    .egc = " ",
    .physcols = READER_COLS,
    .physrows = READER_ROWS,
  };
  channels_set_bg_alpha(&nopts.echannels, CELL_ALPHA_BLEND);
  const int x = ncplane_align(std, NCALIGN_CENTER, nopts.physcols);
  if((marsh->reader = ncreader_create(std, dimy, x, &nopts)) == NULL){
    free(marsh);
    return NULL;
  }
  struct ncreader* reader = marsh->reader;
  ncplane_set_scrolling(ncreader_plane(reader), true);
  if(pthread_create(tid, NULL, reader_thread, marsh)){
    ncreader_destroy(marsh->reader, NULL);
    free(marsh);
    return NULL;
  }
  return reader;
}

static int
zap_reader(pthread_t tid, struct ncreader* reader, unsigned cancel){
  if(cancel){
    pthread_cancel(tid);
  }
  int ret = pthread_join(tid, NULL);
  ncreader_destroy(reader, NULL);
  return ret;
}

int zoo_demo(struct notcurses* nc){
  int dimx;
  if(draw_background(nc)){
    return -1;
  }
  pthread_mutex_t lock;
  if(pthread_mutex_init(&lock, NULL)){
    return -1;
  }
  struct ncplane* n = notcurses_stddim_yx(nc, NULL, &dimx);
  pthread_t readertid;
  struct ncreader* reader = reader_demo(nc, &readertid, &lock);
  // if we didn't get a reader, need to hand-roll the exit, since we have no
  // thread at which we might go off blasting
  if(reader == NULL){
    pthread_mutex_destroy(&lock);
    return -1;
  }
  struct ncselector* selector = selector_demo(n, dimx, 2, &lock);
  struct ncmultiselector* mselector = multiselector_demo(n, dimx, 8, &lock); // FIXME calculate from splane
  if(selector == NULL || mselector == NULL){
    goto err;
  }
  int ret = 0;
  ret |= zap_reader(readertid, reader, false); // let the thread do its thang
  ret |= pthread_mutex_destroy(&lock);
  ncselector_destroy(selector, NULL);
  ncmultiselector_destroy(mselector);
  return ret;

err:
  zap_reader(readertid, reader, true);
  pthread_mutex_destroy(&lock);
  ncselector_destroy(selector, NULL);
  ncmultiselector_destroy(mselector);
  return -1;
}
