#include <unistd.h>
#include <locale.h>
#include <stdlib.h>
#include <notcurses/notcurses.h>

// gradient of 'A's changing color and background changing in reverse
static int
gradientA(struct notcurses* nc){
  int dimy, dimx;
  struct ncplane* stdn = notcurses_stddim_yx(nc, &dimy, &dimx);
  uint64_t ul = CHANNELS_RGB_INITIALIZER(0, 0, 0, 0xff, 0xff, 0xff);
  uint64_t ur = CHANNELS_RGB_INITIALIZER(0, 0xff, 0xff, 0xff, 0, 0);
  uint64_t ll = CHANNELS_RGB_INITIALIZER(0xff, 0, 0, 0, 0xff, 0xff);
  uint64_t lr = CHANNELS_RGB_INITIALIZER(0xff, 0xff, 0xff, 0, 0, 0);
  if(ncplane_gradient(stdn, "A", NCSTYLE_NONE, ul, ur, ll, lr, dimy - 1, dimx - 1) <= 0){
    return -1;
  }
  if(notcurses_render(nc)){
    return -1;
  }
  sleep(1);
  return 0;
}

static int
gradStriations(struct notcurses* nc){
  int dimy, dimx;
  struct ncplane* stdn = notcurses_stddim_yx(nc, &dimy, &dimx);
  uint64_t ul = CHANNELS_RGB_INITIALIZER(0, 0, 0, 0xff, 0xff, 0xff);
  uint64_t ur = CHANNELS_RGB_INITIALIZER(0, 0xff, 0xff, 0xff, 0, 0);
  uint64_t ll = CHANNELS_RGB_INITIALIZER(0xff, 0, 0, 0, 0xff, 0xff);
  uint64_t lr = CHANNELS_RGB_INITIALIZER(0xff, 0xff, 0xff, 0, 0, 0);
  if(ncplane_gradient(stdn, "▄", NCSTYLE_NONE, ul, ur, ll, lr, dimy - 1, dimx - 1) <= 0){
    return -1;
  }
  if(notcurses_render(nc)){
    return -1;
  }
  sleep(1);
  if(ncplane_gradient(stdn, "▀", NCSTYLE_NONE, ul, ur, ll, lr, dimy - 1, dimx - 1) <= 0){
    return -1;
  }
  if(notcurses_render(nc)){
    return -1;
  }
  sleep(1);
  return 0;
}

static int
gradHigh(struct notcurses* nc){
  int dimy, dimx;
  struct ncplane* stdn = notcurses_stddim_yx(nc, &dimy, &dimx);
  uint64_t ul = CHANNEL_RGB_INITIALIZER(0, 0, 0);
  uint64_t ur = CHANNEL_RGB_INITIALIZER(0, 0xff, 0xff);
  uint64_t ll = CHANNEL_RGB_INITIALIZER(0xff, 0, 0);
  uint64_t lr = CHANNEL_RGB_INITIALIZER(0xff, 0xff, 0xff);
  if(ncplane_highgradient(stdn, ul, ur, ll, lr, dimy - 1, dimx - 1) <= 0){
    return -1;
  }
  if(notcurses_render(nc)){
    return -1;
  }
  sleep(1);
  return 0;
}

int main(void){
  if(setlocale(LC_ALL, "") == NULL){
    return EXIT_FAILURE;
  }
  struct notcurses_options opts = {
    .flags = NCOPTION_INHIBIT_SETLOCALE,
  };
  struct notcurses* nc = notcurses_core_init(&opts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  // no short-circuiting, intentional |
  int ret = gradientA(nc) | gradStriations(nc) | gradHigh(nc);
  if(notcurses_stop(nc) || ret){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
