#include <stdlib.h>
#include <unistd.h>
#include <notcurses/notcurses.h>

int main(void){
  struct notcurses* nc = notcurses_init(NULL, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  int dimy, dimx;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  ncplane_set_styles(n, NCSTYLE_NONE);
  ncplane_putstr_yx(n, 0, 0, "a ═ none");
  ncplane_set_styles(n, NCSTYLE_ITALIC);
  ncplane_putstr_yx(n, 1, 0, "a ═ italic");
  ncplane_set_styles(n, NCSTYLE_BOLD);
  ncplane_putstr_yx(n, 2, 0, "a ═ bold");
  ncplane_set_styles(n, NCSTYLE_REVERSE);
  ncplane_putstr_yx(n, 3, 0, "a ═ reverse");
  ncplane_set_styles(n, NCSTYLE_UNDERLINE);
  ncplane_putstr_yx(n, 4, 0, "a ═ underline");
  ncplane_putstr_yx(n, 5, 0, "sleeping for 5s...");
  if(notcurses_render(nc)){
    goto err;
  }
  sleep(5);
  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}
