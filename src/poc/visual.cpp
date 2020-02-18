#include <cstdio>
#include <cstdlib>
#include <clocale>
#include <cassert>
#include <unistd.h>
#include <notcurses/notcurses.h>

int main(int argc, char** argv){
  setlocale(LC_ALL, "");
  notcurses_options opts{};
  opts.inhibit_alternate_screen = true;
  struct notcurses* nc;
  if((nc = notcurses_init(&opts, stdout)) == nullptr){
    return EXIT_FAILURE;
  }
  struct ncplane* n = notcurses_stdplane(nc);
  int dimx, dimy;
  ncplane_dim_yx(n, &dimy, &dimx);

  int averr;
  auto ncv = ncplane_visual_open(n, "../data/changes.jpg", &averr);
  if(!ncv){
    goto err;
  }
  if(!ncvisual_decode(ncv, &averr)){
    goto err;
  }
  if(ncvisual_render(ncv, 0, 0, 0, 0)){
    goto err;
  }
  if(notcurses_render(nc)){
    goto err;
  }
  return notcurses_stop(nc) ? EXIT_FAILURE : EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}
