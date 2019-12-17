#include <cstdio>
#include <cstdlib>
#include <clocale>
#include <cassert>
#include <notcurses.h>

// What happens when we print over half of a wide glyph?

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
  cell c = CELL_TRIVIAL_INITIALIZER;
  if(cell_load(n, &c, "🐳") < 0){
    goto err;
  }
  if(ncplane_set_default(n, &c) < 0){
    goto err;
  }
  cell_release(n, &c);
  if(cell_load(n, &c, "x") < 0){
    goto err;
  }
  for(int i = 0 ; i < dimy ; ++i){
    for(int j = 0 ; j < dimx / 2 ; j += 2){
      if(ncplane_putc_yx(n, i, j, &c) < 0){
        goto err;
      }
    }
  }
  if(notcurses_render(nc)){
    goto err;
  }
  return notcurses_stop(nc) ? EXIT_FAILURE : EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}
