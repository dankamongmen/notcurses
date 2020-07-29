#include "demo.h"

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
multiselector_demo(struct notcurses* nc, struct ncplane* n, int dimx, int y){
  ncmultiselector_options mopts = {
    .maxdisplay = 8,
    .title = "multi-item selector",
    .items = mselect_items,
    .boxchannels = CHANNELS_RGB_INITIALIZER(0x20, 0xe0, 0xe0, 0x20, 0, 0),
    .opchannels = CHANNELS_RGB_INITIALIZER(0xe0, 0x80, 0x40, 0, 0, 0),
    .descchannels = CHANNELS_RGB_INITIALIZER(0x80, 0xe0, 0x40, 0, 0, 0),
    .footchannels = CHANNELS_RGB_INITIALIZER(0xe0, 0, 0x40, 0x20, 0x20, 0),
    .titlechannels = CHANNELS_RGB_INITIALIZER(0x20, 0xff, 0xff, 0, 0, 0x20),
    .bgchannels = CHANNELS_RGB_INITIALIZER(0, 0x20, 0, 0, 0x20, 0),
  };
  channels_set_fg_alpha(&mopts.bgchannels, CELL_ALPHA_BLEND);
  channels_set_bg_alpha(&mopts.bgchannels, CELL_ALPHA_BLEND);
  struct ncmultiselector* mselector = ncmultiselector_create(n, y, 0, &mopts);
  if(mselector == NULL){
    return NULL;
  }
  struct ncplane* mplane = ncmultiselector_plane(mselector);
  struct timespec swoopdelay;
  timespec_div(&demodelay, dimx / 3, &swoopdelay);
  int length = ncplane_dim_x(mplane);
  ncplane_move_yx(mplane, y, -length);
  ncinput ni;
  for(int i = -length + 1 ; i < dimx - (length + 1) ; ++i){
    if(demo_render(nc)){
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
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  uint64_t cur = timespec_to_ns(&ts);
  uint64_t targ = cur + GIG;
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
    struct ncvisual* ncv = ncvisual_from_file("../data/changes.jpg", &err);
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
  }
  return 0;
}

static struct ncselector*
selector_demo(struct notcurses* nc, struct ncplane* n, int dimx, int y){
  ncselector_options sopts = {
    .title = "single-item selector",
    .items = select_items,
    .boxchannels = CHANNELS_RGB_INITIALIZER(0x20, 0xe0, 0x40, 0x20, 0x20, 0x20),
    .opchannels = CHANNELS_RGB_INITIALIZER(0xe0, 0x80, 0x40, 0, 0, 0),
    .descchannels = CHANNELS_RGB_INITIALIZER(0x80, 0xe0, 0x40, 0, 0, 0),
    .footchannels = CHANNELS_RGB_INITIALIZER(0xe0, 0, 0x40, 0x20, 0, 0),
    .titlechannels = CHANNELS_RGB_INITIALIZER(0xff, 0xff, 0x80, 0, 0, 0x20),
    .bgchannels = CHANNELS_RGB_INITIALIZER(0, 0x20, 0, 0, 0x20, 0),
  };
  channels_set_fg_alpha(&sopts.bgchannels, CELL_ALPHA_BLEND);
  channels_set_bg_alpha(&sopts.bgchannels, CELL_ALPHA_BLEND);
  struct ncselector* selector = ncselector_create(n, y, dimx, &sopts);
  if(selector == NULL){
    return NULL;
  }
  struct ncplane* splane = ncselector_plane(selector);
  struct timespec swoopdelay;
  timespec_div(&demodelay, dimx / 3, &swoopdelay);
  ncinput ni;
  for(int i = dimx - 1 ; i > 1 ; --i){
    if(demo_render(nc)){
      ncselector_destroy(selector, NULL);
      return NULL;
    }
    ncplane_move_yx(splane, y, i);
    char32_t wc = demo_getc(nc, &swoopdelay, &ni);
    if(wc == (char32_t)-1){
      ncselector_destroy(selector, NULL);
      return NULL;
    }else if(wc){
      ncselector_offer_input(selector, &ni);
    }
  }
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  uint64_t cur = timespec_to_ns(&ts);
  uint64_t targ = cur + GIG;
  do{
    struct timespec rel;
    ns_to_timespec(targ - cur, &rel);
    char32_t wc = demo_getc(nc, &rel, &ni);
    if(wc == (char32_t)-1){
      ncselector_destroy(selector, NULL);
      return NULL;
    }else if(wc){
      ncselector_offer_input(selector, &ni);
    }
    clock_gettime(CLOCK_MONOTONIC, &ts);
    cur = timespec_to_ns(&ts);
  }while(cur < targ);
  return selector;
}

int zoo_demo(struct notcurses* nc){
  int dimx;
  if(draw_background(nc)){
    return -1;
  }
  struct ncplane* n = notcurses_stddim_yx(nc, NULL, &dimx);
  struct ncselector* selector = selector_demo(nc, n, dimx, 2);
  struct ncmultiselector* mselector = multiselector_demo(nc, n, dimx, 8); // FIXME calculate from splane
  if(selector == NULL || mselector == NULL){
    goto err;
  }
  ncselector_destroy(selector, NULL);
  ncmultiselector_destroy(mselector);
  DEMO_RENDER(nc);
  return 0;

err:
  ncselector_destroy(selector, NULL);
  ncmultiselector_destroy(mselector);
  return -1;
}
