#include <unistd.h>
#include <locale.h>
#include <stdlib.h>
#include <notcurses/notcurses.h>

// gradient of 'A's changing color and background changing in reverse
static int
gradientA(struct notcurses* nc){
  unsigned dimy, dimx;
  struct ncplane* stdn = notcurses_stddim_yx(nc, &dimy, &dimx);
  uint64_t ul = NCCHANNELS_INITIALIZER(0, 0, 0, 0xff, 0xff, 0xff);
  uint64_t ur = NCCHANNELS_INITIALIZER(0, 0xff, 0xff, 0xff, 0, 0);
  uint64_t ll = NCCHANNELS_INITIALIZER(0xff, 0, 0, 0, 0xff, 0xff);
  uint64_t lr = NCCHANNELS_INITIALIZER(0xff, 0xff, 0xff, 0, 0, 0);
  if(ncplane_gradient(stdn, 0, 0, 0, 0, "A", NCSTYLE_NONE, ul, ur, ll, lr) <= 0){
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
  unsigned dimy, dimx;
  struct ncplane* stdn = notcurses_stddim_yx(nc, &dimy, &dimx);
  uint64_t ul = NCCHANNELS_INITIALIZER(0, 0, 0, 0xff, 0xff, 0xff);
  uint64_t ur = NCCHANNELS_INITIALIZER(0, 0xff, 0xff, 0xff, 0, 0);
  uint64_t ll = NCCHANNELS_INITIALIZER(0xff, 0, 0, 0, 0xff, 0xff);
  uint64_t lr = NCCHANNELS_INITIALIZER(0xff, 0xff, 0xff, 0, 0, 0);
  if(ncplane_gradient(stdn, 0, 0, 0, 0, "▄", NCSTYLE_NONE, ul, ur, ll, lr) <= 0){
    return -1;
  }
  if(notcurses_render(nc)){
    return -1;
  }
  sleep(1);
  if(ncplane_gradient(stdn, 0, 0, 0, 0, "▀", NCSTYLE_NONE, ul, ur, ll, lr) <= 0){
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
  unsigned dimy, dimx;
  struct ncplane* stdn = notcurses_stddim_yx(nc, &dimy, &dimx);
  uint64_t ul = NCCHANNEL_INITIALIZER(0, 0, 0);
  uint64_t ur = NCCHANNEL_INITIALIZER(0, 0xff, 0xff);
  uint64_t ll = NCCHANNEL_INITIALIZER(0xff, 0, 0);
  uint64_t lr = NCCHANNEL_INITIALIZER(0xff, 0xff, 0xff);
  if(ncplane_gradient2x1(stdn, 0, 0, 0, 0, ul, ur, ll, lr) <= 0){
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
    .flags = NCOPTION_INHIBIT_SETLOCALE
             | NCOPTION_DRAIN_INPUT,
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
