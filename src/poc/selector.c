#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <stdlib.h>
#include <notcurses.h>

static struct selector_item items[] = {
  { "first", "this is the first option", },
  { "2nd", "this is the second option", },
  { "3", "third, third, third option am i", },
  { "fourth", "i have another option here", },
  { "five", "golden rings", },
  { "666", "now it is time for me to REIGN IN BLOOD", },
  { "7seven7", "this monkey's gone to heaven", },
  { "8 8 8", "the chinese love me, i'm told", },
  { "nine", "nine, nine, nine 'cause you left me", },
  { "ten", "stunning and brave", },
};

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
  selector_options sopts;
  memset(&sopts, 0, sizeof(sopts));
  sopts.maxdisplay = 4;
  sopts.items = items;
  sopts.itemcount = sizeof(items) / sizeof(*items);
  sopts.title = "this is an awfully long example of a selector title";
  ncplane_set_fg(notcurses_stdplane(nc), 0x40f040);
  ncplane_putstr_aligned(notcurses_stdplane(nc), 0, NCALIGN_RIGHT, "selector widget demo");
  struct ncselector* ns = ncselector_create(notcurses_stdplane(nc), 3, 0, &sopts);
  if(ns == NULL){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  notcurses_render(nc);
  char32_t keypress;
  while((keypress = notcurses_getc_blocking(nc, NULL)) != (char32_t)-1){
    switch(keypress){
      case NCKEY_UP: case 'k': ncselector_previtem(ns, NULL); break;
      case NCKEY_DOWN: case 'j': ncselector_nextitem(ns, NULL); break;
    }
    if(keypress == 'q'){
      break;
    }
    notcurses_render(nc);
  }
  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
