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
  struct ncplane* n = ncplane_dup(notcurses_stdplane(nc), nullptr);
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
  vopts.scaling = NCSCALE_STRETCH;
  vopts.n = n;
  if(ncvisual_render(nc, ncv, &vopts) == nullptr){
    goto err;
  }
  if(notcurses_render(nc)){
    goto err;
  }
  //ncplane_erase(n);
  sleep(1);
  vopts.scaling = NCSCALE_NONE;
  for(double i = 0 ; i < 256 ; ++i){
    if(ncvisual_rotate(ncv, M_PI / 16)){
      failed = true;
      break;
    }
    if(ncvisual_render(nc, ncv, &vopts) == nullptr){
      failed = true;
      break;
    }
    if(notcurses_render(nc)){
      failed = true;
      break;
    }
  }
  return notcurses_stop(nc) || failed ? EXIT_FAILURE : EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}
