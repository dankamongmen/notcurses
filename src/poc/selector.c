#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <stdlib.h>
#include <notcurses/notcurses.h>

static struct selector_item items[] = {
  { "first", "this is the first option", },
  { "2nd", "this is the second option", },
  { "3", "third, third, third option am i", },
  { "fourth", "i have another option here", },
  { "five", "golden rings", },
  { "666", "now it is time for me to REIGN IN BLOOD", },
  { "7seven7", "this monkey's gone to heaven", },
  { "8 8 8", "the chinese 平仮名平平仮名仮名love me, i'm told", },
  { "nine", "nine, nine, nine 'cause you left me", },
  { "ten", "stunning and brave", },
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
  notcurses_options opts;
  memset(&opts, 0, sizeof(opts));
  struct notcurses* nc = notcurses_init(&opts, stdout);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  if(notcurses_mouse_enable(nc)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  selector_options sopts;
  memset(&sopts, 0, sizeof(sopts));
  sopts.maxdisplay = 4;
  sopts.items = items;
  sopts.itemcount = sizeof(items) / sizeof(*items);
  sopts.title = "this is truly, absolutely an awfully long example of a selector title";
  sopts.secondary = "pick one (you will die regardless)";
  sopts.footer = "press q to exit (there is no exit)";
  sopts.defidx = 5;
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
  struct ncplane* n = notcurses_stdplane(nc);

  int averr;
  struct ncvisual* ncv = ncplane_visual_open(n, "../data/changes.jpg", &averr);
  if(!ncv){
    goto err;
  }
  if(!ncvisual_decode(ncv, &averr)){
    goto err;
  }
  if(ncvisual_render(ncv, 0, 0, -1, -1) <= 0){
    goto err;
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
