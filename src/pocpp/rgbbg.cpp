#include <cstdio>
#include <cstdlib>
#include <clocale>
#include <notcurses/notcurses.h>

auto main() -> int {
  if(!setlocale(LC_ALL, "")){
    fprintf(stderr, "Couldn't set locale\n");
    return EXIT_FAILURE;
  }
  notcurses_options opts{};
  opts.flags = NCOPTION_INHIBIT_SETLOCALE
               | NCOPTION_NO_ALTERNATE_SCREEN
               | NCOPTION_DRAIN_INPUT;
  struct notcurses* nc = notcurses_init(&opts, nullptr);
  if(nc == nullptr){
    return EXIT_FAILURE;
  }
  unsigned dimy, dimx;
  struct ncplane* n = notcurses_stdplane(nc);
  ncplane_dim_yx(n, &dimy, &dimx);
  int r , g, b;
  r = 0;
  g = 0x80;
  b = 0;
  ncplane_set_fg_rgb8(n, 0x40, 0x20, 0x40);
  for(unsigned y = 0 ; y < dimy ; ++y){
    if(ncplane_cursor_move_yx(n, y, 0)){
      goto err;
    }
    for(unsigned x = 0 ; x < dimx ; ++x){
      if(ncplane_set_bg_rgb8(n, r, g, b)){
        goto err;
      }
      if(ncplane_putchar(n, 'x') <= 0){
        goto err;
      }
      if(g % 2){
        if(--b <= 0){
          ++g;
          b = 0;
        }
      }else{
        if(++b >= 256){
          ++g;
          b = 255;
        }
      }
    }
  }
  if(notcurses_render(nc)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  notcurses_stop(nc);
  return EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}
