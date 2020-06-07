#include <unistd.h>
#include "notcurses/notcurses.h"

int main(void){
  struct notcurses_options nopts = {
    .flags = NCOPTION_NO_ALTERNATE_SCREEN,
  };
  struct notcurses* nc = notcurses_init(&nopts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  if(ncplane_putstr_aligned(notcurses_stdplane(nc), 0, NCALIGN_CENTER,
                            "Heute Die Welt, Morgens Das Sonnensystem!") <= 0){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  if(ncplane_putstr_aligned(notcurses_stdplane(nc), 2, NCALIGN_CENTER,
                            "Heute Die Welt, Morgens Das Sonnensystem!") <= 0){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  notcurses_render(nc);
  sleep(5);
  notcurses_stop(nc);
  return EXIT_SUCCESS;
}
