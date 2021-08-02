#include <notcurses/notcurses.h>

static int
interp(struct notcurses* nc, int cellpixy, int cellpixx){
  struct ncplane* stdn = notcurses_stdplane(nc);
  ncplane_printf_yx(stdn, 0, 0, "cellpix: %d/%d", cellpixy, cellpixx);
  ncplane_printf_yx(stdn, 10, 0, "press any key to continue");
  size_t rands = cellpixy * cellpixx * 3;
  unsigned char* randrgb = malloc(rands);
  if(randrgb == NULL){
    return -1;
  }
  for(size_t r = 0 ; r < rands ; ++r){
    randrgb[r] = rand() % 256;
  }
  struct ncvisual* ncv = ncvisual_from_rgb_packed(randrgb, cellpixy, cellpixx * 3, cellpixx, 0xff);
  if(ncv == NULL){
    free(randrgb);
    return -1;
  }
  struct ncvisual_options vopts = {
    .y = 1,
    .blitter = NCBLIT_PIXEL,
  };
  struct ncplane* ncvp = ncvisual_render(nc, ncv, &vopts);
  if(ncvp == NULL){
    free(randrgb);
    return -1;
  }
  struct ncplane_options popts = {
    .y = 3,
    .x = 1,
    .rows = 6,
    .cols = 12,
  };
  struct ncplane* scalep = ncplane_create(stdn, &popts);
  vopts.y = 0;
  vopts.n = scalep;
  vopts.scaling = NCSCALE_STRETCH;
  popts.x += ncplane_dim_x(scalep) + 1;
  if(ncvisual_render(nc, ncv, &vopts) == NULL){
    free(randrgb);
    return -1;
  }
  ncplane_putstr_yx(stdn, 2, 4, "scale");
  struct ncplane* scalepni = ncplane_create(stdn, &popts);
  vopts.n = scalepni;
  vopts.flags = NCVISUAL_OPTION_NOINTERPOLATE;
  if(ncvisual_render(nc, ncv, &vopts) == NULL){
    free(randrgb);
    return -1;
  }
  ncplane_putstr_yx(stdn, 2, 15, "scale(no)");
  popts.x += ncplane_dim_x(scalepni) + 1;
  struct ncplane* resizep = ncplane_create(stdn, &popts);
  if(resizep == NULL){
    free(randrgb);
    return -1;
  }
  if(ncvisual_resize(ncv, popts.rows * cellpixy, popts.cols * cellpixx)){
    free(randrgb);
    return -1;
  }
  vopts.flags = 0;
  vopts.n = resizep;
  vopts.scaling = NCSCALE_NONE;
  if(ncvisual_render(nc, ncv, &vopts) == NULL){
    free(randrgb);
    return -1;
  }
  ncplane_putstr_yx(stdn, 2, 30, "resize");
  ncvisual_destroy(ncv);
  ncv = ncvisual_from_rgb_packed(randrgb, cellpixy, cellpixx * 3, cellpixx, 0xff);
  free(randrgb);
  popts.x += ncplane_dim_x(scalepni) + 1;
  struct ncplane* inflatep = ncplane_create(stdn, &popts);
  if(inflatep == NULL){
    return -1;
  }
  vopts.n = inflatep;
  if(ncvisual_resize_noninterpolative(ncv, popts.rows * cellpixy, popts.cols * cellpixx)){
    return -1;
  }
  if(ncvisual_render(nc, ncv, &vopts) == NULL){
    return -1;
  }
  ncplane_putstr_yx(stdn, 2, 41, "resize(no)");
  notcurses_render(nc);
  ncplane_destroy(ncvp);
  ncplane_destroy(scalep);
  ncplane_destroy(scalepni);
  return 0;
}

int main(void){
  struct notcurses_options nopts = {
//    .loglevel = NCLOGLEVEL_TRACE,
  };
  struct notcurses* nc = notcurses_init(&nopts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  struct ncplane* stdn = notcurses_stdplane(nc);
  int cellpixy, cellpixx;
  ncplane_pixelgeom(stdn, NULL, NULL, &cellpixy, &cellpixx, NULL, NULL);
  if(interp(nc, cellpixy, cellpixx)){
    goto err;
  }
  ncinput ni;
  notcurses_getc_blocking(nc, &ni);
  notcurses_stop(nc);
  return EXIT_SUCCESS;

err:
  notcurses_stop(nc);
  return EXIT_FAILURE;
}
