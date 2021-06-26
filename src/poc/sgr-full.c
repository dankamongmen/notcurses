#include <stdlib.h>
#include <unistd.h>
#include <notcurses/notcurses.h>

int main(void){
  struct notcurses_options nopts = {
    .flags = NCOPTION_NO_ALTERNATE_SCREEN | NCOPTION_SUPPRESS_BANNERS,
  };
  struct notcurses* nc = notcurses_core_init(&nopts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  int dimy, dimx;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  ncplane_set_base(n, " ", 0, 0);
  notcurses_render(nc); // clear the screen
  int y = 0;
  // FIXME do full permutations?
  ncplane_set_styles(n, NCSTYLE_NONE);
  ncplane_putstr_yx(n, y++, 0, "a ═ none");
  ncplane_set_styles(n, NCSTYLE_BLINK);
  ncplane_putstr_yx(n, y++, 0, "a ═ blink");
  ncplane_set_styles(n, NCSTYLE_ITALIC);
  ncplane_putstr_yx(n, y++, 0, "a ═ italic");
  ncplane_set_styles(n, NCSTYLE_BOLD);
  ncplane_putstr_yx(n, y++, 0, "a ═ bold");
  ncplane_set_styles(n, NCSTYLE_UNDERCURL);
  ncplane_putstr_yx(n, y++, 0, "a ═ undercurl");
  ncplane_set_styles(n, NCSTYLE_UNDERLINE);
  ncplane_putstr_yx(n, y++, 0, "a ═ underline");
  ncplane_set_styles(n, NCSTYLE_STRUCK);
  ncplane_putstr_yx(n, y++, 0, "a ═ struck");
  ncplane_set_styles(n, NCSTYLE_ITALIC | NCSTYLE_BOLD);
  ncplane_putstr_yx(n, y++, 0, "a ═ italic bold");
  ncplane_set_styles(n, NCSTYLE_ITALIC | NCSTYLE_UNDERCURL);
  ncplane_putstr_yx(n, y++, 0, "a ═ italic undercurl");
  ncplane_set_styles(n, NCSTYLE_ITALIC | NCSTYLE_UNDERLINE);
  ncplane_putstr_yx(n, y++, 0, "a ═ italic underline");
  ncplane_set_styles(n, NCSTYLE_ITALIC | NCSTYLE_STRUCK);
  ncplane_putstr_yx(n, y++, 0, "a ═ italic struck");
  ncplane_set_styles(n, NCSTYLE_ITALIC | NCSTYLE_STRUCK);
  ncplane_putstr_yx(n, y++, 0, "a ═ italic struck");
  ncplane_set_styles(n, NCSTYLE_STRUCK | NCSTYLE_BOLD);
  ncplane_putstr_yx(n, y++, 0, "a ═ struck bold");
  ncplane_set_styles(n, NCSTYLE_STRUCK | NCSTYLE_UNDERCURL);
  ncplane_putstr_yx(n, y++, 0, "a ═ struck undercurl");
  ncplane_set_styles(n, NCSTYLE_STRUCK | NCSTYLE_UNDERLINE);
  ncplane_putstr_yx(n, y++, 0, "a ═ struck underline");
  ncplane_set_styles(n, NCSTYLE_BOLD | NCSTYLE_UNDERCURL);
  ncplane_putstr_yx(n, y++, 0, "a ═ bold undercurl");
  ncplane_set_styles(n, NCSTYLE_BOLD | NCSTYLE_UNDERLINE);
  ncplane_putstr_yx(n, y++, 0, "a ═ bold underline");
  ncplane_set_styles(n, NCSTYLE_BOLD | NCSTYLE_UNDERCURL | NCSTYLE_ITALIC);
  ncplane_putstr_yx(n, y++, 0, "a ═ bold undercurl italic");
  ncplane_set_styles(n, NCSTYLE_BOLD | NCSTYLE_UNDERLINE | NCSTYLE_ITALIC);
  ncplane_putstr_yx(n, y++, 0, "a ═ bold underline italic");
  ncplane_set_styles(n, NCSTYLE_STRUCK | NCSTYLE_UNDERCURL | NCSTYLE_ITALIC);
  ncplane_putstr_yx(n, y++, 0, "a ═ struck undercurl italic");
  ncplane_set_styles(n, NCSTYLE_STRUCK | NCSTYLE_UNDERLINE | NCSTYLE_ITALIC);
  ncplane_putstr_yx(n, y++, 0, "a ═ struck underline italic");
  ncplane_set_styles(n, NCSTYLE_STRUCK | NCSTYLE_UNDERCURL | NCSTYLE_BOLD);
  ncplane_putstr_yx(n, y++, 0, "a ═ struck undercurl bold");
  ncplane_set_styles(n, NCSTYLE_STRUCK | NCSTYLE_UNDERLINE | NCSTYLE_BOLD);
  ncplane_putstr_yx(n, y++, 0, "a ═ struck underline bold");
  ncplane_set_styles(n, NCSTYLE_BOLD | NCSTYLE_UNDERLINE | NCSTYLE_ITALIC | NCSTYLE_STRUCK);
  ncplane_putstr_yx(n, y++, 0, "a ═ bold underline italic struck");
  ncplane_set_styles(n, NCSTYLE_BOLD | NCSTYLE_UNDERCURL | NCSTYLE_ITALIC | NCSTYLE_STRUCK);
  ncplane_putstr_yx(n, y++, 0, "a ═ bold undercurl italic struck");
  ncplane_set_styles(n, NCSTYLE_BOLD | NCSTYLE_UNDERLINE | NCSTYLE_ITALIC | NCSTYLE_STRUCK | NCSTYLE_BLINK);
  ncplane_putstr_yx(n, y++, 0, "a ═ bold underline italic struck blink");
  ncplane_set_styles(n, NCSTYLE_BOLD | NCSTYLE_UNDERCURL | NCSTYLE_ITALIC | NCSTYLE_STRUCK | NCSTYLE_BLINK);
  ncplane_putstr_yx(n, y++, 0, "a ═ bold undercurl italic struck blink");

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
