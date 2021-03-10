#include <unistd.h>
#include <stdlib.h>
#include <notcurses/notcurses.h>

static int
handle(struct notcurses* nc, const char *fn){
  struct ncvisual* ncv = ncvisual_from_file(fn);
  if(ncv == NULL){
    return -1;
  }
  int dimy, dimx;
  struct ncplane* stdn = notcurses_stddim_yx(nc, &dimy, &dimx);
  uint64_t channels = CHANNELS_RGB_INITIALIZER(100, 140, 100, 140, 100, 140);
  ncplane_set_base(stdn, "a", 0, channels);
  for(int x = 0 ; x < dimx ; ++x){
    struct ncvisual_options vopts = {
      .x = x,
      .scaling = NCSCALE_NONE_HIRES,
      .blitter = NCBLIT_PIXEL,
    };
    struct ncplane* nv = ncvisual_render(nc, ncv, &vopts);
    if(nv == NULL){
      ncvisual_destroy(ncv);
      return -1;
    }
    notcurses_render(nc);
    sleep(1);
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
  struct notcurses* nc = notcurses_init(NULL, NULL);
  if(!notcurses_check_pixel_support(nc)){
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
