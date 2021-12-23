#include <stdlib.h>
#include <unistd.h>
#include <notcurses/notcurses.h>

int main(void){
  struct notcurses_options nopts = {
    .flags = NCOPTION_CLI_MODE
             | NCOPTION_SUPPRESS_BANNERS
             | NCOPTION_DRAIN_INPUT,
  };
  struct notcurses* nc = notcurses_core_init(&nopts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  unsigned dimy, dimx;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  // FIXME do full permutations?
  ncplane_set_styles(n, NCSTYLE_NONE);
  ncplane_putstr(n, "a ═ none\n");
  ncplane_set_styles(n, NCSTYLE_ITALIC);
  ncplane_putstr(n, "a ═ italic\n");
  ncplane_set_styles(n, NCSTYLE_BOLD);
  ncplane_putstr(n, "a ═ bold\n");
  ncplane_set_styles(n, NCSTYLE_UNDERCURL);
  ncplane_putstr(n, "a ═ undercurl\n");
  ncplane_set_styles(n, NCSTYLE_UNDERLINE);
  ncplane_putstr(n, "a ═ underline\n");
  ncplane_set_styles(n, NCSTYLE_STRUCK);
  ncplane_putstr(n, "a ═ struck\n");
  ncplane_set_styles(n, NCSTYLE_ITALIC | NCSTYLE_BOLD);
  ncplane_putstr(n, "a ═ italic bold\n");
  ncplane_set_styles(n, NCSTYLE_ITALIC | NCSTYLE_BOLD | NCSTYLE_STRUCK);
  ncplane_putstr(n, "a ═ italic bold struck\n");
  ncplane_set_styles(n, NCSTYLE_ITALIC | NCSTYLE_UNDERCURL);
  ncplane_putstr(n, "a ═ italic undercurl\n");
  ncplane_set_styles(n, NCSTYLE_ITALIC | NCSTYLE_UNDERLINE);
  ncplane_putstr(n, "a ═ italic underline\n");
  ncplane_set_styles(n, NCSTYLE_ITALIC | NCSTYLE_STRUCK);
  ncplane_putstr(n, "a ═ italic struck\n");
  ncplane_set_styles(n, NCSTYLE_STRUCK | NCSTYLE_BOLD);
  ncplane_putstr(n, "a ═ struck bold\n");
  ncplane_set_styles(n, NCSTYLE_STRUCK | NCSTYLE_BOLD | NCSTYLE_ITALIC);
  ncplane_putstr(n, "a ═ struck bold italic\n");
  ncplane_set_styles(n, NCSTYLE_STRUCK | NCSTYLE_UNDERCURL);
  ncplane_putstr(n, "a ═ struck undercurl\n");
  ncplane_set_styles(n, NCSTYLE_STRUCK | NCSTYLE_UNDERLINE);
  ncplane_putstr(n, "a ═ struck underline\n");
  ncplane_set_styles(n, NCSTYLE_BOLD | NCSTYLE_UNDERCURL);
  ncplane_putstr(n, "a ═ bold undercurl\n");
  ncplane_set_styles(n, NCSTYLE_BOLD | NCSTYLE_UNDERLINE);
  ncplane_putstr(n, "a ═ bold underline\n");
  ncplane_set_styles(n, NCSTYLE_BOLD | NCSTYLE_UNDERCURL | NCSTYLE_ITALIC);
  ncplane_putstr(n, "a ═ bold undercurl italic\n");
  ncplane_set_styles(n, NCSTYLE_BOLD | NCSTYLE_UNDERLINE | NCSTYLE_ITALIC);
  ncplane_putstr(n, "a ═ bold underline italic\n");
  ncplane_set_styles(n, NCSTYLE_STRUCK | NCSTYLE_UNDERCURL | NCSTYLE_ITALIC);
  ncplane_putstr(n, "a ═ struck undercurl italic\n");
  ncplane_set_styles(n, NCSTYLE_STRUCK | NCSTYLE_UNDERLINE | NCSTYLE_ITALIC);
  ncplane_putstr(n, "a ═ struck underline italic\n");
  ncplane_set_styles(n, NCSTYLE_STRUCK | NCSTYLE_UNDERCURL | NCSTYLE_BOLD);
  ncplane_putstr(n, "a ═ struck undercurl bold\n");
  ncplane_set_styles(n, NCSTYLE_STRUCK | NCSTYLE_UNDERLINE | NCSTYLE_BOLD);
  ncplane_putstr(n, "a ═ struck underline bold\n");
  ncplane_set_styles(n, NCSTYLE_BOLD | NCSTYLE_UNDERLINE | NCSTYLE_ITALIC | NCSTYLE_STRUCK);
  ncplane_putstr(n, "a ═ bold underline italic struck\n");
  ncplane_set_styles(n, NCSTYLE_BOLD | NCSTYLE_UNDERCURL | NCSTYLE_ITALIC | NCSTYLE_STRUCK);
  ncplane_putstr(n, "a ═ bold undercurl italic struck\n");
  ncplane_set_styles(n, NCSTYLE_NONE);
  ncplane_putstr(n, "a ═ none\n");
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
