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

static struct ncmultiselector*
multiselector_demo(struct notcurses* nc, struct ncplane* n, int dimx, int y){
  ncmultiselector_options mopts = {
    .maxdisplay = 8,
    .title = "multi-item selector",
  };
  channels_set_fg(&mopts.boxchannels, 0x20e040);
  channels_set_fg(&mopts.opchannels, 0xe08040);
  channels_set_fg(&mopts.descchannels, 0x80e040);
  channels_set_bg(&mopts.opchannels, 0);
  channels_set_bg(&mopts.descchannels, 0);
  channels_set_fg(&mopts.footchannels, 0xe00040);
  channels_set_fg(&mopts.titlechannels, 0xffff80);
  channels_set_fg(&mopts.bgchannels, 0x002000);
  channels_set_bg(&mopts.bgchannels, 0x002000);
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
  for(int i = 0 ; i < dimx - (length + 1) ; ++i){
    if(demo_render(nc)){
      ncmultiselector_destroy(mselector);
      return NULL;
    }
    ncplane_move_yx(mplane, y, i);
    demo_nanosleep(nc, &swoopdelay);
  }
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

int zoo_demo(struct notcurses* nc){
  int dimx;
  if(draw_background(nc)){
    return -1;
  }
  struct ncplane* n = notcurses_stddim_yx(nc, NULL, &dimx);
  ncselector_options sopts = {
    .maxdisplay = 4,
    .title = "single-item selector",
    .items = select_items,
  };
  channels_set_fg(&sopts.boxchannels, 0x20e040);
  channels_set_fg(&sopts.opchannels, 0xe08040);
  channels_set_fg(&sopts.descchannels, 0x80e040);
  channels_set_bg(&sopts.opchannels, 0);
  channels_set_bg(&sopts.descchannels, 0);
  channels_set_fg(&sopts.footchannels, 0xe00040);
  channels_set_fg(&sopts.titlechannels, 0xffff80);
  channels_set_fg(&sopts.bgchannels, 0x002000);
  channels_set_bg(&sopts.bgchannels, 0x002000);
  channels_set_fg_alpha(&sopts.bgchannels, CELL_ALPHA_BLEND);
  channels_set_bg_alpha(&sopts.bgchannels, CELL_ALPHA_BLEND);
  struct ncselector* selector = ncselector_create(n, 2, dimx, &sopts);
  if(selector == NULL){
    return -1;
  }
  struct ncplane* splane = ncselector_plane(selector);
  struct timespec swoopdelay;
  timespec_div(&demodelay, dimx / 3, &swoopdelay);
  for(int i = dimx - 1 ; i > 1 ; --i){
    DEMO_RENDER(nc);
    ncplane_move_yx(splane, 2, i);
    demo_nanosleep(nc, &swoopdelay);
  }
  struct ncmultiselector* mselector;
  mselector = multiselector_demo(nc, n, dimx, 8); // FIXME calculate from splane
  ncselector_destroy(selector, NULL);
  ncmultiselector_destroy(mselector);
  DEMO_RENDER(nc);
  return 0;
}
