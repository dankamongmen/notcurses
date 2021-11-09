#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <stdlib.h>
#include <notcurses/notcurses.h>
#include "version.h"

// http://theboomerbible.com/tbb112.html
static struct ncmselector_item items[] = {
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

static void
run_mselect(struct notcurses* nc, struct ncmultiselector* ns){
  static int item = 0;
  ++item;
  if(ns == NULL){
    notcurses_stop(nc);
    fprintf(stderr, "Error creating selector %d\n", item);
    exit(EXIT_FAILURE);
  }
  notcurses_render(nc);
  uint32_t keypress;
  ncinput ni;
  while((keypress = notcurses_get_blocking(nc, &ni)) != (uint32_t)-1){
    if(ni.evtype == NCTYPE_RELEASE){
      continue;
    }
    if(!ncmultiselector_offer_input(ns, &ni)){
      switch(keypress){
        case NCKEY_ENTER: ncmultiselector_destroy(ns); return;
        case 'M': case 'J': if(ni.ctrl){ ncmultiselector_destroy(ns); return; }
      }
      if(keypress == 'q'){
        break;
      }
    }
    notcurses_render(nc);
  }
  ncmultiselector_destroy(ns);
}

int main(void){
  notcurses_options opts = {
    .loglevel = NCLOGLEVEL_ERROR,
  };
  struct notcurses* nc = notcurses_init(&opts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  notcurses_mice_enable(nc, NCMICE_BUTTON_EVENT);
  ncmultiselector_options sopts;
  memset(&sopts, 0, sizeof(sopts));
  sopts.maxdisplay = 10;
  sopts.items = items;
  sopts.title = "this is truly an awfully long example of a MULTISELECTOR title";
  sopts.secondary = "pick one (you will die regardless)";
  sopts.footer = "press q to exit (there is sartrev(\"no exit\"))";
  sopts.boxchannels = NCCHANNELS_INITIALIZER(0x20, 0xe0, 0xe0, 0x20, 0, 0);
  sopts.opchannels = NCCHANNELS_INITIALIZER(0xe0, 0x80, 0x40, 0, 0, 0);
  sopts.descchannels = NCCHANNELS_INITIALIZER(0x80, 0xe0, 0x40, 0, 0, 0);
  sopts.footchannels = NCCHANNELS_INITIALIZER(0xe0, 0, 0x40, 0x20, 0x20, 0);
  sopts.titlechannels = NCCHANNELS_INITIALIZER(0x20, 0xff, 0xff, 0, 0, 0x20);
  uint64_t bgchannels = NCCHANNELS_INITIALIZER(0, 0x20, 0, 0, 0x20, 0);
  ncchannels_set_fg_alpha(&bgchannels, NCALPHA_BLEND);
  ncchannels_set_bg_alpha(&bgchannels, NCALPHA_BLEND);
  struct ncplane* n = notcurses_stdplane(nc);

  if(notcurses_canopen_images(nc)){
    struct ncvisual* ncv = ncvisual_from_file("../data/covid19.jpg");
    if(!ncv){
      goto err;
    }
    struct ncvisual_options vopts = {
      .scaling = NCSCALE_STRETCH,
      .n = n,
    };
    if(ncvisual_blit(nc, ncv, &vopts) == NULL){
      goto err;
    }
  }

  ncplane_set_fg_rgb(n, 0x40f040);
  ncplane_putstr_aligned(n, 0, NCALIGN_RIGHT, "multiselect widget demo");
  struct ncplane_options nopts = {
    .y = 3,
    .x = 0,
    .rows = 1,
    .cols = 1,
    .userptr = NULL,
    .name = NULL,
    .resizecb = NULL,
    .flags = 0,
  };
  struct ncplane* mseln = ncplane_create(n, &nopts);
  if(mseln == NULL){
    goto err;
  }
  ncplane_set_base(mseln, "", 0, bgchannels);
  struct ncmultiselector* ns = ncmultiselector_create(mseln, &sopts);
  run_mselect(nc, ns);

  sopts.title = "short round title";
  mseln = ncplane_create(n, &nopts);
  ncplane_set_base(mseln, "", 0, bgchannels);
  ns = ncmultiselector_create(mseln, &sopts);
  run_mselect(nc, ns);

  sopts.title = "short round title";
  sopts.secondary = "now this secondary is also very, very, very outlandishly long, you see";
  mseln = ncplane_create(n, &nopts);
  ncplane_set_base(mseln, "", 0, bgchannels);
  ns = ncmultiselector_create(mseln, &sopts);
  run_mselect(nc, ns);

  sopts.title = "the whole world is watching";
  sopts.secondary = NULL;
  sopts.footer = "now this FOOTERFOOTER is also very, very, very outlandishly long, you see";
  mseln = ncplane_create(n, &nopts);
  ncplane_set_base(mseln, "", 0, bgchannels);
  ns = ncmultiselector_create(mseln, &sopts);
  run_mselect(nc, ns);

  sopts.title = "chomps";
  sopts.secondary = NULL;
  sopts.footer = NULL;
  mseln = ncplane_create(n, &nopts);
  ncplane_set_base(mseln, "", 0, bgchannels);
  ns = ncmultiselector_create(mseln, &sopts);
  run_mselect(nc, ns);

  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}
