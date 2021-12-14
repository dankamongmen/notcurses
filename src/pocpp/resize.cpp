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
  opts.flags = NCOPTION_INHIBIT_SETLOCALE
               | NCOPTION_NO_ALTERNATE_SCREEN
               | NCOPTION_DRAIN_INPUT;
  struct notcurses* nc;
  if((nc = notcurses_init(&opts, nullptr)) == nullptr){
    return EXIT_FAILURE;
  }
  struct ncvisual_options vopts{};
  bool failed = false;
  unsigned dimy, dimx;
  int top = 0;
  int bot;
  auto ncv = ncvisual_from_file(file);
  if(!ncv){
    goto err;
  }
  vopts.n = notcurses_stdplane(nc);
  vopts.scaling = NCSCALE_STRETCH;
  vopts.flags = NCVISUAL_OPTION_CHILDPLANE;
  struct ncplane* ntarg;
  if((ntarg = ncvisual_blit(nc, ncv, &vopts)) == nullptr){
    goto err;
  }
  if(notcurses_render(nc)){
    goto err;
  }
  ncplane_dim_yx(ntarg, &dimy, &dimx);
  // start chopping off rows
  bot = dimy - 1;
  while(top <= bot){
    // one from top
    sleep(1);
    if(ncplane_resize(ntarg, 1, 0, bot - top, dimx, 0, 0, bot - top, dimx)){
      goto err;
    }
    ++top;
    if(notcurses_render(nc)){
      goto err;
    }
    if(top >= bot){
      break;
    }
    // one from bottom
    if(ncplane_resize(ntarg, 0, 0, bot - top, dimx, 0, 0, bot - top, dimx)){
      goto err;
    }
    --bot;
    if(notcurses_render(nc)){
      goto err;
    }
  }
  return notcurses_stop(nc) || failed ? EXIT_FAILURE : EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}
