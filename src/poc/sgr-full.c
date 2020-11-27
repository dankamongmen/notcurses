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
  int y = 0;
  ncplane_set_styles(n, NCSTYLE_NONE);
  ncplane_putstr_yx(n, y++, 0, "a ═ none");
  ncplane_set_styles(n, NCSTYLE_ITALIC);
  ncplane_putstr_yx(n, y++, 0, "a ═ italic");
  ncplane_set_styles(n, NCSTYLE_BOLD);
  ncplane_putstr_yx(n, y++, 0, "a ═ bold");
  ncplane_set_styles(n, NCSTYLE_REVERSE);
  ncplane_putstr_yx(n, y++, 0, "a ═ reverse");
  ncplane_set_styles(n, NCSTYLE_UNDERLINE);
  ncplane_putstr_yx(n, y++, 0, "a ═ underline");
  ncplane_set_styles(n, NCSTYLE_BLINK);
  ncplane_putstr_yx(n, y++, 0, "a ═ blink");
  ncplane_set_styles(n, NCSTYLE_STRUCK);
  ncplane_putstr_yx(n, y++, 0, "a ═ strikethrough");
  ncplane_set_styles(n, NCSTYLE_ITALIC | NCSTYLE_BOLD);
  ncplane_putstr_yx(n, y++, 0, "a ═ italic bold");
  ncplane_set_styles(n, NCSTYLE_ITALIC | NCSTYLE_REVERSE);
  ncplane_putstr_yx(n, y++, 0, "a ═ italic reverse");
  ncplane_set_styles(n, NCSTYLE_ITALIC | NCSTYLE_UNDERLINE);
  ncplane_putstr_yx(n, y++, 0, "a ═ italic underline");
  ncplane_set_styles(n, NCSTYLE_ITALIC | NCSTYLE_BLINK);
  ncplane_putstr_yx(n, y++, 0, "a ═ italic blink");
  ncplane_set_styles(n, NCSTYLE_ITALIC | NCSTYLE_STRUCK);
  ncplane_putstr_yx(n, y++, 0, "a ═ italic struck");
  ncplane_set_styles(n, NCSTYLE_STRUCK | NCSTYLE_BOLD);
  ncplane_putstr_yx(n, y++, 0, "a ═ struck bold");
  ncplane_set_styles(n, NCSTYLE_STRUCK | NCSTYLE_REVERSE);
  ncplane_putstr_yx(n, y++, 0, "a ═ struck reverse");
  ncplane_set_styles(n, NCSTYLE_STRUCK | NCSTYLE_UNDERLINE);
  ncplane_putstr_yx(n, y++, 0, "a ═ struck underline");
  ncplane_set_styles(n, NCSTYLE_STRUCK | NCSTYLE_BLINK);
  ncplane_putstr_yx(n, y++, 0, "a ═ struck blink");
  ncplane_set_styles(n, NCSTYLE_BOLD | NCSTYLE_REVERSE);
  ncplane_putstr_yx(n, y++, 0, "a ═ bold reverse");
  ncplane_set_styles(n, NCSTYLE_BOLD | NCSTYLE_UNDERLINE);
  ncplane_putstr_yx(n, y++, 0, "a ═ bold underline");
  ncplane_set_styles(n, NCSTYLE_BOLD | NCSTYLE_BLINK);
  ncplane_putstr_yx(n, y++, 0, "a ═ bold blink");
  ncplane_putstr_yx(n, y++, 0, "sleeping for 15s...");
  if(notcurses_render(nc)){
    goto err;
  }
  sleep(15);
  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}
