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
  char32_t keypress;
  ncinput ni;
  while((keypress = notcurses_getc_blocking(nc, &ni)) != (char32_t)-1){
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
  sopts.title = "this is truly an awfully long example of a MULTISELECTOR title";
  sopts.secondary = "pick one (you will die regardless)";
  sopts.footer = "press q to exit (there is sartrev(\"no exit\"))";
  sopts.boxchannels = CHANNELS_RGB_INITIALIZER(0x20, 0xe0, 0xe0, 0x20, 0, 0),
  sopts.opchannels = CHANNELS_RGB_INITIALIZER(0xe0, 0x80, 0x40, 0, 0, 0),
  sopts.descchannels = CHANNELS_RGB_INITIALIZER(0x80, 0xe0, 0x40, 0, 0, 0),
  sopts.footchannels = CHANNELS_RGB_INITIALIZER(0xe0, 0, 0x40, 0x20, 0x20, 0),
  sopts.titlechannels = CHANNELS_RGB_INITIALIZER(0x20, 0xff, 0xff, 0, 0, 0x20),
  sopts.bgchannels = CHANNELS_RGB_INITIALIZER(0, 0x20, 0, 0, 0x20, 0),
  channels_set_fg_alpha(&sopts.bgchannels, CELL_ALPHA_BLEND);
  channels_set_bg_alpha(&sopts.bgchannels, CELL_ALPHA_BLEND);
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
    if(ncvisual_render(nc, ncv, &vopts) == NULL){
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
