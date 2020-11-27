#include <stdlib.h>
#include <unistd.h>
#include <notcurses/notcurses.h>

int main(void){
  struct notcurses_options nopts = {
    .flags = NCOPTION_NO_ALTERNATE_SCREEN,
  };
  struct notcurses* nc = notcurses_init(&nopts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  int dimy, dimx;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  int y = 0;
  // FIXME do full permutations?
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
  ncplane_putstr_yx(n, y++, 0, "a ═ struck");
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
  ncplane_set_styles(n, NCSTYLE_BOLD | NCSTYLE_REVERSE | NCSTYLE_UNDERLINE |
                        NCSTYLE_BLINK | NCSTYLE_ITALIC | NCSTYLE_STRUCK);
  ncplane_putstr_yx(n, y++, 0, "a ═ whoomp! there it is");

  ncplane_set_styles(n, NCSTYLE_NONE);
  if(notcurses_render(nc)){
    goto err;
  }
  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}
