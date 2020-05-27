#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <clocale>
#include <cassert>
#include <unistd.h>
#include <notcurses/notcurses.h>

int main(int argc, char** argv){
  const char* file = "../data/changes.jpg";
  setlocale(LC_ALL, "");
  if(argc > 2){
    fprintf(stderr, "usage: visual [ file ]\n");
    return EXIT_FAILURE;
  }else if(argc == 2){
    file = argv[1];
  }
  notcurses_options opts{};
  opts.inhibit_alternate_screen = true;
  opts.flags = NCOPTION_INHIBIT_SETLOCALE;
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
  nc_err_e ncerr;
  auto ncv = ncvisual_from_file(file, &ncerr);
  if(!ncv){
    goto err;
  }
  if((ncerr = ncvisual_decode(ncv)) != NCERR_SUCCESS){
    goto err;
  }
  int scaley, scalex;
  vopts.n = n;
  if(ncvisual_render(nc, ncv, &vopts) == nullptr){
    goto err;
  }
notcurses_debug(nc, stderr);
  if(notcurses_render(nc)){
    goto err;
  }
sleep(2);
ncplane_erase(n);
  ncvisual_geom(nc, ncv, NCBLIT_DEFAULT, nullptr, nullptr, &scaley, &scalex);
  ncvisual_resize(ncv, dimy * scaley, dimx * scalex);
  vopts.n = n;
  if(ncvisual_render(nc, ncv, &vopts) == nullptr){
    goto err;
  }
notcurses_debug(nc, stderr);
  if(notcurses_render(nc)){
    goto err;
  }
sleep(2);
  for(double i = 0 ; i < 256 ; ++i){
    sleep(1);
    if(ncvisual_rotate(ncv, M_PI / 2)){
      failed = true;
      break;
    }
    ncplane_erase(n);
    if(ncvisual_render(nc, ncv, &vopts) == nullptr){
      failed = true;
      break;
    }
    if(notcurses_render(nc)){
      failed = true;
      break;
    }
  }
  ncvisual_destroy(ncv);
  return notcurses_stop(nc) || failed ? EXIT_FAILURE : EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}
