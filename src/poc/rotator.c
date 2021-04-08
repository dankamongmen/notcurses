#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <locale.h>
#include <notcurses/notcurses.h>
#include "compat/compat.h"

static int
rotate_grad(struct notcurses* nc){
  struct timespec ts = {
    .tv_sec = 0,
    .tv_nsec = 750000000,
  };
  int dimy, dimx;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  ncplane_home(n);
  uint32_t tl = 0, tr = 0, bl = 0, br = 0;
  channel_set_rgb8(&tl, 0xff, 0, 0);
  channel_set_rgb8(&tr, 0, 0, 0xff);
  channel_set_rgb8(&bl, 0, 0xff, 0);
  channel_set_rgb8(&br, 0, 0xff, 0xff);
  if(ncplane_highgradient(n, tl, tr, bl, br, dimy - 1, dimx - 1) <= 0){
    return -1;
  }
  notcurses_render(nc);
  clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
  uint32_t* rgba = ncplane_as_rgba(n, NCBLIT_DEFAULT, 0, 0,
                                   dimy, dimx, NULL, NULL);
  if(rgba == NULL){
    return -1;
  }
  ncplane_erase(n);
  notcurses_render(nc);
  clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);;

  struct ncvisual_options vopts = {
    .n = n,
    .leny = dimy * 2,
    .lenx = dimx,
  };
  if(ncblit_rgba(rgba, dimx * 4, &vopts) < 0){
    free(rgba);
    return -1;
  }
  notcurses_render(nc);
  clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);;

  ncplane_erase(n);
  notcurses_render(nc);
  clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);;

  // now promote it to a visual
  struct ncvisual* v = ncvisual_from_rgba(rgba, dimy * 2, dimx * 4, dimx);
  free(rgba);
  if(v == NULL){
    return -1;
  }
  vopts.leny = 0;
  vopts.lenx = 0;
  if(n != ncvisual_render(nc, v, &vopts)){
    ncvisual_destroy(v);
    return -1;
  }
  notcurses_render(nc);
  clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);;
  vopts.n = NULL;
  ncplane_erase(n);
  for(int i = 0 ; i < 4 ; ++i){
    int vy, vx, scaley, scalex;
    if(ncvisual_rotate(v, M_PI / 2)){
      return -1;
    }
    ncvisual_geom(nc, v, &vopts, &vy, &vx, &scaley, &scalex);
    vopts.x = (dimx - (vx / scalex)) / 2;
    vopts.y = (dimy - (vy / scaley)) / 2;
    struct ncplane* newn = ncvisual_render(nc, v, &vopts);
    if(newn == NULL){
      return -1;
    }
    notcurses_render(nc);
    ncplane_destroy(newn);
    clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);;
  }

  for(int i = 0 ; i < 8 ; ++i){
    int vy, vx, scaley, scalex;
    ncvisual_geom(nc, v, &vopts, &vy, &vx, &scaley, &scalex);
    vopts.x = (dimx - (vx / scalex)) / 2;
    vopts.y = (dimy - (vy / scaley)) / 2;
    if(ncvisual_rotate(v, M_PI / 4)){
      return -1;
    }
    ncplane_erase(n);
    struct ncplane* newn = ncvisual_render(nc, v, &vopts);
    if(newn == NULL){
      return -1;
    }
    notcurses_render(nc);
    ncplane_destroy(newn);
    clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);;
  }

  ncvisual_destroy(v);
  return 0;
}

static int
rotate(struct notcurses* nc){
  struct timespec ts = {
    .tv_sec = 0,
    .tv_nsec = 250000000,
  };
  const int XSIZE = 16;
  int dimy, dimx;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  if(dimy < XSIZE || dimx < 2){
    return -1;
  }
  int r = 255;
  int g = 0;
  int b = 0;
  for(int x = 0 ; x < XSIZE ; ++x){
    if(ncplane_set_fg_rgb8(n, r, g, b)){
      return -1;
    }
    if(ncplane_set_bg_rgb8(n, b, r, g)){
      return -1;
    }
    if(ncplane_putegc_yx(n, dimy / 2, x, "▀", NULL) < 0){
      return -1;
    }
    g += 15;
    r -= 14;
  }
  g = 0;
  b = 255;
  for(int x = 0 ; x < XSIZE ; ++x){
    if(ncplane_set_fg_rgb8(n, r, g, b)){
      return -1;
    }
    if(ncplane_set_bg_rgb8(n, b, r, g)){
      return -1;
    }
    if(ncplane_putegc_yx(n, dimy / 2 + 1, x, "▄", NULL) < 0){
      return -1;
    }
    g += 14;
    b -= 15;
  }
  notcurses_render(nc);
  clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);;

  // we now have 2 rows of 20 cells each, with gradients. load 'em.
  uint32_t* rgba = ncplane_as_rgba(n, NCBLIT_DEFAULT, dimy / 2, 0,
                                   2, XSIZE, NULL, NULL);
  if(rgba == NULL){
    return -1;
  }
  /*
  for(int y = 0 ; y < 4 ; ++y){
    for(int x = 0 ; x < XSIZE ; ++x){
      fprintf(stderr, "rgba %02d/%02d: %08x\n", y, x, rgba[y * XSIZE + x]);
    }
  }
  */
  ncplane_erase(n);
  notcurses_render(nc);
  clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);;

  struct ncvisual_options vopts = {
    .lenx = XSIZE,
    .leny = 4,
    .y = dimy / 2,
    .x = XSIZE,
    .n = n,
  };
  if(ncblit_rgba(rgba, XSIZE * 4, &vopts) < 0){
    free(rgba);
    return -1;
  }
  notcurses_render(nc);
  clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);;

  ncplane_erase(n);
  notcurses_render(nc);
  clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);;

  // now promote it to a visual
  struct ncvisual* v = ncvisual_from_rgba(rgba, 4, XSIZE * 4, XSIZE);
  free(rgba);
  if(v == NULL){
    return -1;
  }
  vopts.x = (dimx - XSIZE) / 2;
  vopts.y = dimy / 2;
  ncvisual_render(nc, v, &vopts);
  notcurses_render(nc);
  clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);;

  for(int i = 0 ; i < 4 ; ++i){
    if(ncvisual_rotate(v, M_PI / 2)){
      return -1;
    }
    ncplane_erase(n);
    ncvisual_render(nc, v, &vopts);
    notcurses_render(nc);
    clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);;
  }

  for(int i = 0 ; i < 8 ; ++i){
    if(ncvisual_rotate(v, M_PI / 4)){
      return -1;
    }
    ncplane_erase(n);
    ncvisual_render(nc, v, &vopts);
    notcurses_render(nc);
    clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);;
  }

  ncvisual_destroy(v);
  return 0;
}

int main(void){
  setlocale(LC_ALL, "");
  struct notcurses_options nopts = {
    .flags = NCOPTION_INHIBIT_SETLOCALE | NCOPTION_NO_ALTERNATE_SCREEN,
  };
  struct notcurses* nc = notcurses_init(&nopts, NULL);
  int r = 0;
  r |= rotate(nc);
  r |= rotate_grad(nc);
  r |= notcurses_stop(nc);
  return r ? EXIT_FAILURE : EXIT_SUCCESS;
}
