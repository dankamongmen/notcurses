#include <stdlib.h>
#include <notcurses/notcurses.h>

int main(void){
  notcurses_options nopts = {
    .flags = NCOPTION_NO_ALTERNATE_SCREEN,
  };
  struct notcurses* nc = notcurses_init(&nopts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  notcurses_check_pixel_support(nc);
  notcurses_debug_caps(nc, stdout);
  return notcurses_stop(nc) ? EXIT_FAILURE : EXIT_SUCCESS;
}
