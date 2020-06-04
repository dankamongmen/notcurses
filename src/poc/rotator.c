#include <unistd.h>
#include <stdlib.h>
#include <locale.h>
#include <notcurses/notcurses.h>

static int
rotate(struct notcurses* nc){
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
  sleep(1);

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
  sleep(1);

  if(ncplane_blit_rgba(n, dimy / 2, XSIZE, XSIZE * 4, NCBLIT_DEFAULT,
                       rgba, 0, 0, 4, XSIZE) < 0){
    free(rgba);
    return -1;
  }
  notcurses_render(nc);
  sleep(1);

  ncplane_erase(n);
  notcurses_render(nc);
  sleep(1);

  // now promote it to a visual
  struct ncvisual* v = ncvisual_from_rgba(rgba, 4, XSIZE * 4, XSIZE);
  if(v == NULL){
    return -1;
  }
  struct ncvisual_options vopts = {
    .x = (dimx - XSIZE) / 2,
    .y = dimy / 2,
  };
  ncvisual_render(nc, v, &vopts);
  notcurses_render(nc);
  sleep(1);
  return 0;
}

int main(void){
  setlocale(LC_ALL, "");
  struct notcurses_options nopts = {
    .inhibit_alternate_screen = true,
    .flags = NCOPTION_INHIBIT_SETLOCALE,
  };
  struct notcurses* nc = notcurses_init(&nopts, NULL);
  int r = rotate(nc);
  r |= notcurses_stop(nc);
  return r ? EXIT_FAILURE : EXIT_SUCCESS;
}
