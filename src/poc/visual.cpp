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
  struct notcurses* nc;
  if((nc = notcurses_init(&opts, stdout)) == nullptr){
    return EXIT_FAILURE;
  }
  struct ncplane* n = notcurses_stdplane(nc);
  int dimx, dimy;
  ncplane_dim_yx(n, &dimy, &dimx);
  bool failed = false;
  nc_err_e ncerr;
  auto ncv = ncplane_visual_open(n, file, &ncerr);
  if(!ncv){
    goto err;
  }
  if((ncerr = ncvisual_decode(ncv)) != NCERR_SUCCESS){
    goto err;
  }
  if(ncvisual_render(ncv, 0, 0, -1, -1) <= 0){
    goto err;
  }
  if(notcurses_render(nc)){
    goto err;
  }
  for(int i = 0 ; i < 64 ; ++i){
    if(ncvisual_rotate(ncv, 2 * M_PI / i)){
      failed = true;
      break;
    }
    if(ncvisual_render(ncv, 0, 0, -1, -1) < 0){
      failed = true;
      break;
    }
sleep(1);
  }
  return notcurses_stop(nc) || failed ? EXIT_FAILURE : EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}
