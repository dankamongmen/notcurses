#include <stdlib.h>
#include <notcurses/notcurses.h>

static uint64_t
ts_to_ns(const struct timespec* ts){
  return ts->tv_sec * 1000000000 + ts->tv_nsec;
}

static const uint64_t delay = 500000000ull;

static int
pbar_fill(struct notcurses* nc, struct ncprogbar* pbar){
  struct timespec cur;
  clock_gettime(CLOCK_MONOTONIC, &cur);
  const uint64_t startns = ts_to_ns(&cur);
  const uint64_t deadline = startns + delay;
  do{
    clock_gettime(CLOCK_MONOTONIC, &cur);
    uint64_t curns = ts_to_ns(&cur);
    double p = (curns - startns) / (double)delay;
    if(p > 1.0){
      p = 1;
    }
    if(ncprogbar_set_progress(pbar, p)){
      return -1;
    }
    notcurses_render(nc);
  }while(ts_to_ns(&cur) < deadline);
  return 0;
}

static struct ncprogbar*
hbar_make(struct notcurses* nc, uint64_t flags){
  int dimy, dimx;
  struct ncplane* std = notcurses_stddim_yx(nc, &dimy, &dimx);
  struct ncplane_options nopts = {
    .y = 1,
    .x = NCALIGN_CENTER,
    .rows = dimy - 4,
    .cols = 5,
    .name = "pbar",
    .flags = NCPLANE_OPTION_HORALIGNED,
  };
  struct ncplane* pbar = ncplane_create(std, &nopts);
  if(pbar == NULL){
    return NULL;
  }
  int posy, posx, pdimy, pdimx;
  ncplane_yx(pbar, &posy, &posx);
  ncplane_dim_yx(pbar, &pdimy, &pdimx);
  ncplane_cursor_move_yx(std, posy - 1, posx - 1);
  uint64_t channels = 0;
  channels_set_fg_rgb8(&channels, 0, 0xde, 0xde);
  if(ncplane_rounded_box(std, 0, channels, posy + pdimy, posx + pdimx, 0)){
    ncplane_destroy(pbar);
    return NULL;
  }
  struct ncprogbar_options popts = {
    .flags = flags,
  };
  ncchannel_set_rgb8(&popts.ulchannel, 0x80, 0x22, 0x22);
  ncchannel_set_rgb8(&popts.urchannel, 0x22, 0x22, 0x80);
  ncchannel_set_rgb8(&popts.blchannel, 0x22, 0x80, 0x22);
  ncchannel_set_rgb8(&popts.brchannel, 0x80, 0x22, 0x22);
  struct ncprogbar* ncp = ncprogbar_create(pbar, &popts);
  if(ncp == NULL){
    return NULL;
  }
  return ncp;
}

static struct ncprogbar*
pbar_make(struct notcurses* nc, uint64_t flags){
  int dimy, dimx;
  struct ncplane* std = notcurses_stddim_yx(nc, &dimy, &dimx);
  struct ncplane_options nopts = {
    .y = dimy / 2,
    .x = NCALIGN_CENTER,
    .rows = 3,
    .cols = dimx - 20,
    .name = "pbar",
    .flags = NCPLANE_OPTION_HORALIGNED,
  };
  struct ncplane* pbar = ncplane_create(std, &nopts);
  if(pbar == NULL){
    return NULL;
  }
  int posy, posx, pdimy, pdimx;
  ncplane_yx(pbar, &posy, &posx);
  ncplane_dim_yx(pbar, &pdimy, &pdimx);
  ncplane_cursor_move_yx(std, posy - 1, posx - 1);
  uint64_t channels = 0;
  channels_set_fg_rgb8(&channels, 0, 0xde, 0xde);
  if(ncplane_rounded_box(std, 0, channels, posy + pdimy, posx + pdimx, 0)){
    ncplane_destroy(pbar);
    return NULL;
  }
  struct ncprogbar_options popts = {
    .flags = flags,
  };
  ncchannel_set_rgb8(&popts.ulchannel, 0x80, 0xcc, 0xcc);
  ncchannel_set_rgb8(&popts.urchannel, 0xcc, 0xcc, 0x80);
  ncchannel_set_rgb8(&popts.blchannel, 0xcc, 0x80, 0xcc);
  ncchannel_set_rgb8(&popts.brchannel, 0x80, 0xcc, 0xcc);
  struct ncprogbar* ncp = ncprogbar_create(pbar, &popts);
  if(ncp == NULL){
    return NULL;
  }
  return ncp;
}

int main(void){
  struct notcurses* nc = notcurses_core_init(NULL, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  struct ncprogbar* ncp = pbar_make(nc, NCPROGBAR_OPTION_RETROGRADE);
  if(ncp == NULL){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  if(pbar_fill(nc, ncp)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  ncprogbar_destroy(ncp);
  ncp = pbar_make(nc, 0);
  if(ncp == NULL){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  if(pbar_fill(nc, ncp)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  ncprogbar_destroy(ncp);
  ncplane_erase(notcurses_stdplane(nc));
  ncp = hbar_make(nc, NCPROGBAR_OPTION_RETROGRADE);
  if(ncp == NULL){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  if(pbar_fill(nc, ncp)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  ncprogbar_destroy(ncp);
  ncp = hbar_make(nc, 0);
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
