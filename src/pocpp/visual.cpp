#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <clocale>
#include <cassert>
#include <unistd.h>
#include <notcurses/notcurses.h>
#include "compat/compat.h"

int main(int argc, char** argv){
  struct timespec ts = {
    .tv_sec = 0,
    .tv_nsec = 250000000,
  };
  const char* file = "../data/changes.jpg";
  setlocale(LC_ALL, "");
  if(argc > 2){
    fprintf(stderr, "usage: visual [ file ]\n");
    return EXIT_FAILURE;
  }else if(argc == 2){
    file = argv[1];
  }
  notcurses_options opts{};
  //opts.loglevel = NCLOGLEVEL_TRACE;
  opts.flags = NCOPTION_INHIBIT_SETLOCALE | NCOPTION_NO_ALTERNATE_SCREEN;
  struct notcurses* nc;
  if((nc = notcurses_init(&opts, nullptr)) == nullptr){
    return EXIT_FAILURE;
  }
  int dimy, dimx;
  struct ncplane* n = ncplane_dup(notcurses_stddim_yx(nc, &dimy, &dimx), nullptr);
  if(!n){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  struct ncvisual_options vopts{};
  bool failed = false;
  auto ncv = ncvisual_from_file(file);
  if(!ncv){
    goto err;
  }
  int scaley, scalex;
  vopts.n = n;
  if(ncvisual_render(nc, ncv, &vopts) == nullptr){
    goto err;
  }
  if(notcurses_render(nc)){
    goto err;
  }
  clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);

  ncplane_erase(n);
  ncvisual_blitter_geom(nc, ncv, &vopts, nullptr, nullptr, &scaley, &scalex, nullptr);
  if(ncvisual_resize(ncv, dimy * scaley, dimx * scalex)){
    goto err;
  }
  vopts.n = n;
  if(ncvisual_render(nc, ncv, &vopts) == nullptr){
    goto err;
  }
  if(notcurses_render(nc)){
    goto err;
  }
  clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);

  vopts.n = NULL;
  vopts.x = NCALIGN_CENTER;
  vopts.y = NCALIGN_CENTER;
  vopts.flags |= NCVISUAL_OPTION_HORALIGNED | NCVISUAL_OPTION_VERALIGNED;
  ncplane_destroy(n);
  for(double i = 0 ; i < 256 ; ++i){
    clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
    if(ncvisual_rotate(ncv, M_PI / ((i / 32) + 2))){
      failed = true;
      break;
    }
    struct ncplane* newn;
    if((newn = ncvisual_render(nc, ncv, &vopts)) == nullptr){
      failed = true;
      break;
    }
    if(notcurses_render(nc)){
      ncplane_destroy(newn);
      failed = true;
      break;
    }
    ncplane_destroy(newn);
  }
  ncvisual_destroy(ncv);
  return notcurses_stop(nc) || failed ? EXIT_FAILURE : EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}
