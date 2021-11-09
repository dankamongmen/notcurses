#include <unistd.h>
#include <stdlib.h>
#include <notcurses/notcurses.h>

static int
handle(struct notcurses* nc, const char *fn){
  struct ncvisual* ncv = ncvisual_from_file(fn);
  if(ncv == NULL){
    return -1;
  }
  unsigned dimy, dimx;
  struct ncplane* stdn = notcurses_stddim_yx(nc, &dimy, &dimx);
  for(unsigned y = 0 ; y < dimy ; y += 15){
    for(unsigned x = 0 ; x < dimx ; x += 15){
      uint64_t channels = NCCHANNELS_INITIALIZER(rand() % 256, rand() % 256, 100, rand() % 256, 100, 140);
      ncplane_set_base(stdn, "a", 0, channels);
      struct ncvisual_options vopts = {
        .n = notcurses_stdplane(nc),
        .y = y,
        .x = x,
        .scaling = NCSCALE_NONE_HIRES,
        .blitter = NCBLIT_PIXEL,
        .flags = NCVISUAL_OPTION_CHILDPLANE | NCVISUAL_OPTION_NODEGRADE,
      };
      struct ncplane* nv = ncvisual_blit(nc, ncv, &vopts);
      if(nv == NULL){
        ncvisual_destroy(ncv);
        return -1;
      }
      notcurses_render(nc);
      sleep(1);
      channels = NCCHANNELS_INITIALIZER(rand() % 256, rand() % 256, 100, rand() % 256, 100, 140);
      ncplane_set_base(stdn, "a", 0, channels);
      notcurses_render(nc);
      sleep(1);
      ncplane_destroy(nv);
    }
  }
  ncvisual_destroy(ncv);
  return 0;
}

int main(int argc, char** argv){
  if(argc < 2){
    fprintf(stderr, "need image arguments\n");
    return EXIT_FAILURE;
  }
  char** a = argv + 1;
  struct notcurses_options opts = {
    .margin_t = 2,
    .margin_l = 2,
    .margin_b = 2,
    .margin_r = 2,
    .flags = NCOPTION_DRAIN_INPUT,
  };
  struct notcurses* nc = notcurses_init(&opts, NULL);
  if(notcurses_check_pixel_support(nc) <= 0){
    notcurses_stop(nc);
    fprintf(stderr, "this program requires pixel graphics support\n");
    return EXIT_FAILURE;
  }
  do{
    handle(nc, *a);
  }while(*++a);
  notcurses_stop(nc);
  return EXIT_SUCCESS;
}
