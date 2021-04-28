#include <time.h>
#include <stdio.h>
#include <notcurses/notcurses.h>

// drag plane |t| across plane |n| at cell row |y|
static int
across_row(struct notcurses* nc, int y, struct ncplane* n, struct ncplane* t,
           const struct timespec* ds){
  for(int x = 0 ; x < ncplane_dim_x(n) ; ++x){
    if(ncplane_move_yx(t, y, x)){
      return -1;
    }
    if(notcurses_render(nc)){
      return -1;
    }
    clock_nanosleep(CLOCK_MONOTONIC, 0, ds, NULL);
  }
  return 0;
}

static int
handle(struct notcurses* nc, const char* fn){
  struct ncvisual* ncv = ncvisual_from_file(fn);
  struct timespec ds = { .tv_sec = 0, .tv_nsec = 25000000, };
  if(!ncv){
    return -1;
  }
  // render by itself
  struct ncvisual_options vopts = {
    .blitter = NCBLIT_PIXEL,
    .flags = NCVISUAL_OPTION_NODEGRADE,
  };
  struct ncplane* n = ncvisual_render(nc, ncv, &vopts);
  if(n == NULL){
    ncvisual_destroy(ncv);
    return -1;
  }
  notcurses_render(nc);
  clock_nanosleep(CLOCK_MONOTONIC, 0, &ds, NULL);
  // render a single cell atop it
  struct ncplane_options opts = {
    .rows = 1,
    .cols = 1,
  };
  struct ncplane* t = ncplane_create(n, &opts);
  if(!t){
    ncplane_destroy(n);
    ncvisual_destroy(ncv);
    return -1;
  }
  uint64_t channels = CHANNELS_RGB_INITIALIZER(0xff, 0xff, 0xff, 0xff, 0xff, 0xff);
  ncplane_set_base(t, " ", 0, channels);
  notcurses_render(nc);
  clock_nanosleep(CLOCK_MONOTONIC, 0, &ds, NULL);
  // move said cell through the sprixel
  for(int y = 0 ; y < ncplane_dim_y(n) - 1 ; ++y){
    if(across_row(nc, y, n, t, &ds)){
      ncplane_destroy(n);
      ncvisual_destroy(ncv);
    }
  }
  // now make it big and throw it over the bottom
  if(ncplane_resize_simple(t, 6, ncplane_dim_x(n))){
    ncplane_destroy(n);
    ncvisual_destroy(ncv);
    return -1;
  }
  if(ncplane_move_yx(t, ncplane_dim_y(n) - ncplane_dim_y(t), 0)){
    ncplane_destroy(n);
    ncvisual_destroy(ncv);
    return -1;
  }
  notcurses_render(nc);
  clock_nanosleep(CLOCK_MONOTONIC, 0, &ds, NULL);
  // now restore the sprixel entirely
  ncplane_destroy(t);
  notcurses_render(nc);
  clock_nanosleep(CLOCK_MONOTONIC, 0, &ds, NULL);
  ncplane_destroy(n);
  ncvisual_destroy(ncv);
  return 0;
}

int main(int argc, char **argv){
  if(argc < 2){
    fprintf(stderr, "need image arguments\n");
    return EXIT_FAILURE;
  }
  char** a = argv + 1;
  struct notcurses_options opts = { };
  struct notcurses* nc = notcurses_init(&opts, NULL);
  if(notcurses_check_pixel_support(nc) <= 0){
    notcurses_stop(nc);
    fprintf(stderr, "this program requires pixel graphics support\n");
    return EXIT_FAILURE;
  }
  do{
    if(handle(nc, *a)){
      notcurses_stop(nc);
      fprintf(stderr, "Error working with %s\n", *a);
      return EXIT_FAILURE;
    }
  }while(*++a);
  notcurses_stop(nc);
  return EXIT_SUCCESS;
}
