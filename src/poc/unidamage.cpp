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
  cell_set_bg_rgb(&c, 0, 0x80, 0);
  //ncplane_set_default(n, &c);
  if(cell_load(n, &c, "🐳") < 0){
    goto err;
  }
  if(dimy > 5){
    dimy = 5;
  }
  for(int i = 0 ; i < dimy ; ++i){
    for(int j = 8 ; j < dimx / 2 ; ++j){ // leave some empty spaces
      if(ncplane_putc_yx(n, i, j * 2, &c) < 0){
        goto err;
      }
    }
  }
  ncplane_putc_yx(n, dimy, dimx - 3, &c);
  ncplane_putc_yx(n, dimy, dimx - 1, &c);
  ncplane_putc_yx(n, dimy + 1, dimx - 2, &c);
  ncplane_putc_yx(n, dimy + 1, dimx - 4, &c);
  cell_release(n, &c);
  // put these on the right side of the wide glyphs
  for(int i = 0 ; i < dimy / 2 ; ++i){
    for(int j = 5 ; j < dimx / 2 ; j += 2){
      if(ncplane_putsimple_yx(n, i, j, (j % 10) + '0') < 0){
        goto err;
      }
    }
  }
  // put these on the left side of the wide glyphs
  for(int i = dimy / 2 ; i < dimy ; ++i){
    for(int j = 4 ; j < dimx / 2 ; j += 2){
      if(ncplane_putsimple_yx(n, i, j, (j % 10) + '0') < 0){
        goto err;
      }
    }
  }
  if(notcurses_render(nc)){
    goto err;
  }
  printf("\n");
  return notcurses_stop(nc) ? EXIT_FAILURE : EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}
