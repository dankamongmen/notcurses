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
  opts.flags = NCOPTION_INHIBIT_SETLOCALE | NCOPTION_NO_ALTERNATE_SCREEN;
  struct notcurses* nc;
  if((nc = notcurses_init(&opts, nullptr)) == nullptr){
    return EXIT_FAILURE;
  }
  int dimy, dimx;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  struct ncvisual_options vopts{};
  bool failed = false;
  nc_err_e ncerr;
  auto ncv = ncvisual_from_file(file, &ncerr);
  if(!ncv){
    goto err;
  }
  int scaley, scalex;
  ncvisual_geom(nc, ncv, NCBLIT_DEFAULT, nullptr, nullptr, &scaley, &scalex);
  //ncvisual_resize(ncv, dimy * scaley, dimx * scalex);
  vopts.n = n;
  vopts.scaling = NCSCALE_STRETCH;
  if(ncvisual_render(nc, ncv, &vopts) == nullptr){
    goto err;
  }
  if(notcurses_render(nc)){
    goto err;
  }
  return notcurses_stop(nc) || failed ? EXIT_FAILURE : EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}
