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
  struct ncplane* n = ncplane_dup(notcurses_stdplane(nc), nullptr);
  if(!n){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
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
  for(double i = 0 ; i < 64 ; ++i){
    if(ncvisual_rotate(ncv, M_PI / 2)){
fprintf(stderr, "rotation %f failed\n", i);
      failed = true;
      break;
    }
    if(ncvisual_render(ncv, 0, 0, -1, -1) < 0){
fprintf(stderr, "vrender %f failed\n", i);
      failed = true;
      break;
    }
    if(notcurses_render(nc)){
fprintf(stderr, "render %f failed\n", i);
      failed = true;
      break;
    }
  }
  return notcurses_stop(nc) || failed ? EXIT_FAILURE : EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}
