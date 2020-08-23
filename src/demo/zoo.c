#include "demo.h"

// open up changes.jpg, stretch it to fill, drop it to greyscale
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
    ncvisual_destroy(ncv);
  }
  return 0;
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
multiselector_demo(struct ncplane* n, struct ncplane* under, int dimx, int y){
  ncmultiselector_options mopts = {
    .maxdisplay = 8,
    .title = "multi-item selector",
    .items = mselect_items,
    .boxchannels = CHANNELS_RGB_INITIALIZER(0x20, 0xe0, 0xe0, 0x20, 0, 0),
    .opchannels = CHANNELS_RGB_INITIALIZER(0xe0, 0x80, 0x40, 0, 0, 0),
    .descchannels = CHANNELS_RGB_INITIALIZER(0x80, 0xe0, 0x40, 0, 0, 0),
    .footchannels = CHANNELS_RGB_INITIALIZER(0xe0, 0, 0x40, 0x20, 0x20, 0),
    .titlechannels = CHANNELS_RGB_INITIALIZER(0x80, 0x80, 0xff, 0, 0, 0x20),
    .bgchannels = CHANNELS_RGB_INITIALIZER(0, 0x40, 0, 0, 0x40, 0),
  };
  channels_set_fg_alpha(&mopts.bgchannels, CELL_ALPHA_BLEND);
  channels_set_bg_alpha(&mopts.bgchannels, CELL_ALPHA_BLEND);
  struct ncmultiselector* mselect = ncmultiselector_create(n, y, 0, &mopts);
  if(mselect == NULL){
    return NULL;
  }
  struct ncplane* mplane = ncmultiselector_plane(mselect);
  ncplane_move_below(mplane, under);
  return mselect;
}

static struct ncselector*
selector_demo(struct ncplane* n, struct ncplane* under, int dimx, int y){
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
  struct ncselector* selector = ncselector_create(n, y, dimx, &sopts);
  if(selector == NULL){
    return NULL;
  }
  struct ncplane* mplane = ncselector_plane(selector);
  ncplane_move_below(mplane, under);
  return selector;
}

// wait one demodelay period, offering input to the multiselector, then fade
// out both widgets (if supported).
static int
reader_post(struct notcurses* nc, struct ncselector* selector, struct ncmultiselector* mselector){
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  uint64_t cur = timespec_to_ns(&ts);
  uint64_t targ = cur + timespec_to_ns(&demodelay);
  do{
    struct timespec rel;
    ns_to_timespec(targ - cur, &rel);
    ncinput ni;
    char32_t wc = demo_getc(nc, &rel, &ni);
    if(wc == (char32_t)-1){
      return -1;
    }else if(wc){
      ncmultiselector_offer_input(mselector, &ni);
    }
    clock_gettime(CLOCK_MONOTONIC, &ts);
    cur = timespec_to_ns(&ts);
  }while(cur < targ);
  if(notcurses_canfade(nc)){
    if(ncplane_fadeout(ncselector_plane(selector), &demodelay, demo_fader, NULL)){
      return -1;
    }
    if(ncplane_fadeout(ncmultiselector_plane(mselector), &demodelay, demo_fader, NULL)){
      return -1;
    }
  }
  return 0;
}

// creates an ncreader, ncselector, and ncmultiselector, and moves them into
// place. the latter two are then faded out. all three are then destroyed.
static int
reader_demo(struct notcurses* nc){
  int ret = -1;
  int dimy, dimx;
  struct ncplane* std = notcurses_stddim_yx(nc, &dimy, &dimx);
  const int READER_COLS = 64;
  const int READER_ROWS = 8;
  ncreader_options nopts = {
    .tchannels = CHANNELS_RGB_INITIALIZER(0xa0, 0xe0, 0xe0, 0, 0, 0),
    .echannels = CHANNELS_RGB_INITIALIZER(0x20, 0xe0, 0xe0, 0, 0, 0),
    .egc = " ",
    .physcols = READER_COLS,
    .physrows = READER_ROWS,
  };
  channels_set_bg_alpha(&nopts.echannels, CELL_ALPHA_BLEND);
  const int x = ncplane_align(std, NCALIGN_CENTER, nopts.physcols);
  struct ncselector* selector = NULL;
  struct ncmultiselector* mselector = NULL;
  struct ncreader* reader = ncreader_create(std, dimy, x, &nopts);
  if(reader == NULL){
    goto done;
  }
  ncplane_set_scrolling(ncreader_plane(reader), true);
  // Bring the selector left across the top, while raising the exposition
  // halfway to its target height.
  selector = selector_demo(std, ncreader_plane(reader), dimx, 2);
  if(selector == NULL){
    goto done;
  }
  // FIXME
  // Bring the multiselector right across the top, while raising the exposition
  // the remainder of its path to the center of the screen.
  mselector = multiselector_demo(std, ncreader_plane(reader), dimx, 8);
  if(mselector == NULL){
    goto done;
  }
  // FIXME
  // Delay and fade
  if( (ret = reader_post(nc, selector, mselector)) ){
    goto done;
  }

done:
  ncselector_destroy(selector, NULL);
  ncmultiselector_destroy(mselector);
  ncreader_destroy(reader, NULL);
  return ret;
}


// a plane with exposition text rises from the bottom to the center of the
// screen. as it does so, two widgets (selector and multiselector) come in
// from the left and right, respectively. they then fade out.
int zoo_demo(struct notcurses* nc){
  int dimx;
  if(draw_background(nc)){
    return -1;
  }
  return reader_demo(nc);
}
