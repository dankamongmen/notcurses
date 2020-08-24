#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <stdlib.h>
#include <notcurses/notcurses.h>
#include "version.h"

static struct ncselector_item items[] = {
#define SITEM(short, long) { short, long, 0, 0, }
  SITEM("first", "this is the first option"),
  SITEM("2nd", "this is the second option"),
  SITEM("3", "third, third, third option am i"),
  SITEM("fourth", "i have another option here"),
  SITEM("five", "golden rings"),
  SITEM("666", "now it is time for me to REIGN IN BLOOD"),
  SITEM("7seven7", "this monkey's gone to heaven"),
  SITEM("8 8 8", "the chinese 平仮名平平仮名仮名love me, i'm told"),
  SITEM("nine", "nine, nine, nine 'cause you left me"),
  SITEM("ten", "stunning and brave"),
  SITEM(NULL, NULL),
#undef SITEM
};

static void
run_selector(struct notcurses* nc, struct ncselector* ns){
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
    if(!ncselector_offer_input(ns, &ni)){
      switch(keypress){
        case NCKEY_ENTER: ncselector_destroy(ns, NULL); return;
        case 'M': case 'J': if(ni.ctrl){ ncselector_destroy(ns, NULL); return; }
      }
      if(keypress == 'q'){
        break;
      }
    }
    notcurses_render(nc);
  }
  ncselector_destroy(ns, NULL);
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
  ncselector_options sopts;
  memset(&sopts, 0, sizeof(sopts));
  sopts.maxdisplay = 4;
  sopts.items = items;
  sopts.title = "this is truly, absolutely an awfully long example of a selector title";
  sopts.secondary = "pick one (you will die regardless)";
  sopts.footer = "press q to exit (there is no exit)";
  sopts.defidx = 1;
  sopts.boxchannels = CHANNELS_RGB_INITIALIZER(0x20, 0xe0, 0x40, 0x20, 0x20, 0x20),
  sopts.opchannels = CHANNELS_RGB_INITIALIZER(0xe0, 0x80, 0x40, 0, 0, 0),
  sopts.descchannels = CHANNELS_RGB_INITIALIZER(0x80, 0xe0, 0x40, 0, 0, 0),
  sopts.footchannels = CHANNELS_RGB_INITIALIZER(0xe0, 0, 0x40, 0x20, 0, 0),
  sopts.titlechannels = CHANNELS_RGB_INITIALIZER(0xff, 0xff, 0x80, 0, 0, 0x20),
  sopts.bgchannels = CHANNELS_RGB_INITIALIZER(0, 0x20, 0, 0, 0x20, 0),
  channels_set_fg_alpha(&sopts.bgchannels, CELL_ALPHA_BLEND);
  channels_set_bg_alpha(&sopts.bgchannels, CELL_ALPHA_BLEND);
  struct ncplane* n = notcurses_stdplane(nc);

  if(notcurses_canopen_images(nc)){
    struct ncvisual* ncv = ncvisual_from_file("../data/changes.jpg");
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
    ncvisual_destroy(ncv);
  }

  ncplane_set_fg(n, 0x40f040);
  ncplane_putstr_aligned(n, 0, NCALIGN_RIGHT, "selector widget demo");
  struct ncselector* ns = ncselector_create(n, 3, 0, &sopts);
  run_selector(nc, ns);

  sopts.title = "short round title";
  ns = ncselector_create(n, 3, 0, &sopts);
  run_selector(nc, ns);

  sopts.title = "short round title";
  sopts.secondary = "now this secondary is also very, very, very outlandishly long, you see";
  ns = ncselector_create(n, 3, 0, &sopts);
  run_selector(nc, ns);

  sopts.title = "the whole world is watching";
  sopts.secondary = NULL;
  sopts.footer = "now this FOOTERFOOTER is also very, very, very outlandishly long, you see";
  ns = ncselector_create(n, 3, 0, &sopts);
  run_selector(nc, ns);

  sopts.title = "chomps";
  sopts.secondary = NULL;
  sopts.footer = NULL;
  ns = ncselector_create(n, 3, 0, &sopts);
  run_selector(nc, ns);

  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}
