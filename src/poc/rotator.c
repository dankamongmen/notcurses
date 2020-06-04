#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <locale.h>
#include <notcurses/notcurses.h>

static int
rotate_grad(struct notcurses* nc){
  struct timespec ts = {
    .tv_sec = 0,
    .tv_nsec = 750000000,
  };
  int dimy, dimx;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  ncplane_cursor_move_yx(n, 0, 0);
  uint64_t tl = 0, tr = 0, bl = 0, br = 0;
  channels_set_fg_rgb(&tl, 0, 0, 0);
  channels_set_fg_rgb(&tr, 0, 0, 0xff);
  channels_set_fg_rgb(&bl, 0, 0xff, 0);
  channels_set_fg_rgb(&br, 0, 0xff, 0xff);
  channels_set_bg_rgb(&tl, 0, 0, 0);
  channels_set_bg_rgb(&tr, 0, 0xff, 0);
  channels_set_bg_rgb(&bl, 0, 0, 0xff);
  channels_set_bg_rgb(&br, 0, 0xff, 0xff);
  //if(ncplane_gradient(n, "a", 0, tl, tr, bl, br, dimy - 1, dimx - 1) <= 0){
  if(ncplane_highgradient(n, tl, tr, bl, br, dimy - 1, dimx - 1) <= 0){
    return -1;
  }
  notcurses_render(nc);
  clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
  uint32_t* rgba = ncplane_rgba(n, 0, 0, dimy, dimx);
  if(rgba == NULL){
    return -1;
  }
  ncplane_erase(n);
  notcurses_render(nc);
  clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);;

  if(ncplane_blit_rgba(n, 0, 0, dimx * 4, NCBLIT_DEFAULT,
                       rgba, 0, 0, dimy * 2, dimx) < 0){
    free(rgba);
    return -1;
  }
  free(rgba);
  notcurses_render(nc);
  clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);;

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
    if(ncplane_set_fg_rgb(n, r, g, b)){
      return -1;
    }
    if(ncplane_set_bg_rgb(n, b, r, g)){
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
    if(ncplane_set_fg_rgb(n, r, g, b)){
      return -1;
    }
    if(ncplane_set_bg_rgb(n, b, r, g)){
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
  uint32_t* rgba = ncplane_rgba(n, dimy / 2, 0, 2, XSIZE);
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

  if(ncplane_blit_rgba(n, dimy / 2, XSIZE, XSIZE * 4, NCBLIT_DEFAULT,
                       rgba, 0, 0, 4, XSIZE) < 0){
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
  struct ncvisual_options vopts = {
    .x = (dimx - XSIZE) / 2,
    .y = dimy / 2,
    .n = n,
  };
  ncvisual_render(nc, v, &vopts);
  notcurses_render(nc);
  clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);;

  for(int i = 0 ; i < 4 ; ++i){
    if(NCERR_SUCCESS != ncvisual_rotate(v, M_PI / 2)){
      return -1;
    }
    ncplane_erase(n);
    ncvisual_render(nc, v, &vopts);
    notcurses_render(nc);
    clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);;
  }

  for(int i = 0 ; i < 8 ; ++i){
    if(NCERR_SUCCESS != ncvisual_rotate(v, M_PI / 4)){
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
    .inhibit_alternate_screen = true,
    .flags = NCOPTION_INHIBIT_SETLOCALE,
  };
  struct notcurses* nc = notcurses_init(&nopts, NULL);
  int r = 0;
  r |= rotate(nc);
  r |= rotate_grad(nc);
  r |= notcurses_stop(nc);
  return r ? EXIT_FAILURE : EXIT_SUCCESS;
}
