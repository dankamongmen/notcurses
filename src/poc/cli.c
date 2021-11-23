#include <notcurses/notcurses.h>

int main(void){
  struct notcurses_options nopts = {
    .flags = NCOPTION_PRESERVE_CURSOR |
             NCOPTION_NO_CLEAR_BITMAPS |
             NCOPTION_NO_ALTERNATE_SCREEN,
  };
  struct notcurses* nc = notcurses_init(&nopts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  struct ncplane* stdn = notcurses_stdplane(nc);
  ncplane_putstr(stdn, "press any key");
  notcurses_render(nc);
  ncinput ni;
  do{
    notcurses_get_blocking(nc, &ni);
  }while(ni.evtype == NCTYPE_RELEASE);
  notcurses_stop(nc);
  return EXIT_SUCCESS;
}
