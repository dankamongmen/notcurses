#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <stdlib.h>
#include <notcurses/notcurses.h>
#include "version.h"

// http://theboomerbible.com/tbb112.html
static struct ncmselector_item items[] = {
  { "1", "Across the Atlantic Ocean, there was a place called North America", .selected = false, },
  { "2", "Discovered by an Italian in the employ of the queen of Spain", .selected = false, },
  { "3", "Colonized extensively by the Spanish and the French", .selected = false, },
  { "4", "Developed into a rich nation by Dutch-supplied African slaves", .selected = false, },
  { "5", "And thus became the largest English-speaking nation on earth", .selected = false, },
  { "6", "Namely, the United States of America", .selected = false, },
  { "7", "The inhabitants of the United States called themselves Yankees", .selected = false, },
  { "8", "For some reason", .selected = false, },
  { "9", "And, eventually noticing the rest of the world was there,", .selected = false, },
  { "10", "Decided to rule it.", .selected = false, },
  { "11", "This is their story.", .selected = false, },
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
  char32_t keypress;
  ncinput ni;
  while((keypress = notcurses_getc_blocking(nc, &ni)) != (char32_t)-1){
    if(!ncmultiselector_offer_input(ns, &ni)){
      switch(keypress){
        case NCKEY_ENTER: ncmultiselector_destroy(ns, NULL); return;
        case 'M': case 'J': if(ni.ctrl){ ncmultiselector_destroy(ns, NULL); return; }
      }
      if(keypress == 'q'){
        break;
      }
    }
    notcurses_render(nc);
  }
  ncmultiselector_destroy(ns, NULL);
}

int main(void){
  if(!setlocale(LC_ALL, "")){
    return EXIT_FAILURE;
  }
  notcurses_options opts = {
    .flags = NCOPTION_INHIBIT_SETLOCALE,
  };
  struct notcurses* nc = notcurses_init(&opts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  if(notcurses_mouse_enable(nc)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  ncmultiselector_options sopts;
  memset(&sopts, 0, sizeof(sopts));
  sopts.maxdisplay = 10;
  sopts.items = items;
  sopts.itemcount = sizeof(items) / sizeof(*items);
  sopts.title = "this is truly an awfully long example of a MULTISELECTOR title";
  sopts.secondary = "pick one (you will die regardless)";
  sopts.footer = "press q to exit (there is sartrev(\"no exit\"))";
  channels_set_fg(&sopts.boxchannels, 0x20e0e0);
  channels_set_fg(&sopts.opchannels, 0xe08040);
  channels_set_fg(&sopts.descchannels, 0xe0e040);
  channels_set_bg(&sopts.opchannels, 0);
  channels_set_bg(&sopts.descchannels, 0);
  channels_set_fg(&sopts.footchannels, 0xe00040);
  channels_set_fg(&sopts.titlechannels, 0x80ffff);
  channels_set_fg(&sopts.bgchannels, 0x002000);
  channels_set_bg(&sopts.bgchannels, 0x002000);
  channels_set_fg_alpha(&sopts.bgchannels, CELL_ALPHA_BLEND);
  channels_set_bg_alpha(&sopts.bgchannels, CELL_ALPHA_BLEND);
  struct ncplane* n = notcurses_stdplane(nc);

  if(notcurses_canopen_images(nc)){
    nc_err_e err;
    struct ncvisual_options vopts = {
      .style = NCSCALE_STRETCH,
    };
    struct ncvisual* ncv = ncplane_visual_open(n, &vopts, "../data/covid19.jpg", &err);
    if(!ncv){
      goto err;
    }
    if((err = ncvisual_decode(ncv)) != NCERR_SUCCESS){
      goto err;
    }
    if(ncvisual_render(ncv, 0, 0, -1, -1) <= 0){
      goto err;
    }
  }

  ncplane_set_fg(n, 0x40f040);
  ncplane_putstr_aligned(n, 0, NCALIGN_RIGHT, "multiselect widget demo");
  struct ncmultiselector* ns = ncmultiselector_create(n, 3, 0, &sopts);
  run_mselect(nc, ns);

  sopts.title = "short round title";
  ns = ncmultiselector_create(n, 3, 0, &sopts);
  run_mselect(nc, ns);

  sopts.title = "short round title";
  sopts.secondary = "now this secondary is also very, very, very outlandishly long, you see";
  ns = ncmultiselector_create(n, 3, 0, &sopts);
  run_mselect(nc, ns);

  sopts.title = "the whole world is watching";
  sopts.secondary = NULL;
  sopts.footer = "now this FOOTERFOOTER is also very, very, very outlandishly long, you see";
  ns = ncmultiselector_create(n, 3, 0, &sopts);
  run_mselect(nc, ns);

  sopts.title = "chomps";
  sopts.secondary = NULL;
  sopts.footer = NULL;
  ns = ncmultiselector_create(n, 3, 0, &sopts);
  run_mselect(nc, ns);

  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}
