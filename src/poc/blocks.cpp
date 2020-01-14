#include <cstdio>
#include <cstdlib>
#include <clocale>
#include <cassert>
#include <unistd.h>
#include <notcurses.h>

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

  const int HEIGHT = 5;
  const int WIDTH = 20;
  struct ncplane* na = ncplane_aligned(n, HEIGHT, WIDTH,
                                       dimy - (HEIGHT + 1),
                                       NCALIGN_CENTER,
                                       nullptr);
  uint64_t channels = 0;
  if(na == nullptr){
    goto err;
  }
  ncplane_set_fg(n, 0x00ff00);
  ncplane_set_fg(na, 0x00ff00);
  if(ncplane_cursor_move_yx(na, 0, 0)){
    goto err;
  }
  if(ncplane_rounded_box_sized(na, 0, channels, HEIGHT, WIDTH, 0) < 0){
    goto err;
  }
  if(ncplane_printf_yx(n, 0, 0, "arrrrp?") < 0){
    goto err;
  }
  if(ncplane_rounded_box_sized(n, 4, channels, HEIGHT, WIDTH, 0) < 0){
    goto err;
  }
  if(notcurses_render(nc)){
    goto err;
  }
  sleep(1);
  return notcurses_stop(nc) ? EXIT_FAILURE : EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}
