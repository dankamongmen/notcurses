#include <locale.h>
#include <notcurses/notcurses.h>
#include "compat/compat.h"

int main(void){
  setlocale(LC_ALL, "");
  struct notcurses_options opts = {
    .flags = NCOPTION_INHIBIT_SETLOCALE,
  };
  struct timespec ts = {
    .tv_sec = 1,
    .tv_nsec = 500000000,
  };
  struct notcurses* nc = notcurses_init(&opts, NULL);
  int dimy, dimx;
  struct ncplane* std = notcurses_stddim_yx(nc, &dimy, &dimx);
  struct ncvisual* ncv = ncvisual_from_plane(std, NCBLIT_2x1, 0, 0, dimy / 2, dimx / 2);
  if(ncv == NULL){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  struct ncvisual_options vopts = {
    .n = std,
  };
  if(ncvisual_set_yx(ncv, dimy / 2 - 1, dimx / 2 - 1, 0xffffffff) < 0){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  if(std != ncvisual_render(nc, ncv, &vopts)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  notcurses_render(nc);
  clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);

  if(ncvisual_polyfill_yx(ncv, 0, 0, ncpixel(0x00, 0x80, 0x80)) <= 0){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  if(std != ncvisual_render(nc, ncv, &vopts)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  notcurses_render(nc);
  clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);

  notcurses_stop(nc);
  return EXIT_SUCCESS;
}
