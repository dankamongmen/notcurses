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
  ncplane_set_scrolling(stdn, true);
  if(ncplane_putstr(stdn, "press any key\n") < 0){
    goto err;
  }
  if(notcurses_render(nc)){
    goto err;
  }
  ncinput ni;
  do{
    notcurses_get_blocking(nc, &ni);
  }while(ni.evtype == NCTYPE_RELEASE);
  if(notcurses_render(nc)){
    goto err;
  }
  notcurses_stop(nc);
  return EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}
