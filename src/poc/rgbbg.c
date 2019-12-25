#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <notcurses.h>

int main(void){
  if(!setlocale(LC_ALL, "")){
    fprintf(stderr, "Couldn't set locale\n");
    return EXIT_FAILURE;
  }
  struct notcurses_options opts;
  memset(&opts, 0, sizeof(opts));
  opts.inhibit_alternate_screen = true;
  struct notcurses* nc = notcurses_init(&opts, stdout);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  int y, x, dimy, dimx;
  struct ncplane* n = notcurses_stdplane(nc);
  ncplane_dim_yx(n, &dimy, &dimx);
  int r , g, b;
  r = 0;
  g = 0x80;
  b = 0;
  ncplane_set_fg_rgb(n, 0x40, 0x20, 0x40);
  for(y = 0 ; y < dimy ; ++y){
    for(x = 0 ; x < dimx ; ++x){
      if(ncplane_set_bg_rgb(n, r, g, b)){
        goto err;
      }
      if(ncplane_putsimple(n, 'x') <= 0){
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
