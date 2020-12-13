#include <stdlib.h>
#include <notcurses/notcurses.h>

static uint64_t
ts_to_ns(const struct timespec* ts){
  return ts->tv_sec * 1000000000 + ts->tv_nsec;
}

static const uint64_t delay = 10000000000ull;

static int
pbar_fill(struct notcurses* nc, struct ncprogbar* pbar){
  struct timespec cur;
  clock_gettime(CLOCK_MONOTONIC, &cur);
  const uint64_t startns = ts_to_ns(&cur);
  const uint64_t deadline = startns + delay;
  do{
    uint64_t curns = ts_to_ns(&cur);
    if(ncprogbar_set_progress(pbar, (curns - startns) / delay)){
      return -1;
    }
    notcurses_render(nc);
    clock_gettime(CLOCK_MONOTONIC, &cur);
  }while(ts_to_ns(&cur) < deadline);
  return 0;
}

int main(void){
  struct notcurses* nc = notcurses_init(NULL, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  int dimy, dimx;
  struct ncplane* std = notcurses_stddim_yx(nc, &dimy, &dimx);
  struct ncplane_options nopts = {
    .y = dimy / 2,
    .x = NCALIGN_CENTER,
    .rows = 1,
    .cols = dimx - 20,
    .name = "pbar",
    .flags = NCPLANE_OPTION_HORALIGNED,
  };
  struct ncplane* pbar = ncplane_create(std, &nopts);
  if(pbar == NULL){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  struct ncprogbar* ncp = ncprogbar_create(pbar, NULL);
  if(ncp == NULL){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  if(pbar_fill(nc, ncp)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  ncprogbar_destroy(ncp);
  notcurses_stop(nc);
  return EXIT_SUCCESS;
}
