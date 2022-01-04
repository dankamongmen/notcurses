#include <stdlib.h>
#include <unistd.h>
#include "demo.h"

static int
reload_corners(struct ncplane* n, nccell* ul, nccell* ur, nccell* ll, nccell* lr){
  unsigned dimy, dimx;
  ncplane_dim_yx(n, &dimy, &dimx);
  char* egc;
  if( (egc = ncplane_at_yx(n, 1, dimx - 2, NULL, &ur->channels)) == NULL){
    return -1;
  }
  free(egc);
  if( (egc = ncplane_at_yx(n, 2, 0, NULL, &ul->channels)) == NULL){
    return -1;
  }
  free(egc);
  if( (egc = ncplane_at_yx(n, dimy - 2, dimx - 1, NULL, &lr->channels)) == NULL){
    return -1;
  }
  free(egc);
  if( (egc = ncplane_at_yx(n, dimy - 1, 1, NULL, &ll->channels)) == NULL){
    return -1;
  }
  free(egc);
  return 0;
}

static int
ascii_target(struct ncplane* n, int ytargbase){
  if(ncplane_putstr_aligned(n, ytargbase++, NCALIGN_CENTER, "/-----\\") < 0){
    return -1;
  }
  if(ncplane_putstr_aligned(n, ytargbase++, NCALIGN_CENTER, "|/---\\|") < 0){
    return -1;
  }
  if(ncplane_putstr_aligned(n, ytargbase++, NCALIGN_CENTER, "||\\|/||") < 0){
    return -1;
  }
  if(ncplane_putstr_aligned(n, ytargbase++, NCALIGN_CENTER, "||-X-||") < 0){
    return -1;
  }
  if(ncplane_putstr_aligned(n, ytargbase++, NCALIGN_CENTER, "||/|\\||") < 0){
    return -1;
  }
  if(ncplane_putstr_aligned(n, ytargbase++, NCALIGN_CENTER, "|\\---/|") < 0){
    return -1;
  }
  if(ncplane_putstr_aligned(n, ytargbase++, NCALIGN_CENTER, "\\-----/") < 0){
    return -1;
  }
  return 0;
}

static int
utf8_target(struct ncplane* n, int ytargbase){
  if(ncplane_putstr_aligned(n, ytargbase++, NCALIGN_CENTER, "┏━━┳━━┓") < 0){
    return -1;
  }
  if(ncplane_putstr_aligned(n, ytargbase++, NCALIGN_CENTER, "┃┌─╂─┐┃") < 0){
    return -1;
  }
  if(ncplane_putstr_aligned(n, ytargbase++, NCALIGN_CENTER, "┃│╲╿╱│┃") < 0){
    return -1;
  }
  if(ncplane_putstr_aligned(n, ytargbase++, NCALIGN_CENTER, "┣┿╾╳╼┿┫") < 0){
    return -1;
  }
  if(ncplane_putstr_aligned(n, ytargbase++, NCALIGN_CENTER, "┃│╱╽╲│┃") < 0){
    return -1;
  }
  if(ncplane_putstr_aligned(n, ytargbase++, NCALIGN_CENTER, "┃└─╂─┘┃") < 0){
    return -1;
  }
  if(ncplane_putstr_aligned(n, ytargbase++, NCALIGN_CENTER, "┗━━┻━━┛") < 0){
    return -1;
  }
  return 0;
}

struct ship {
  struct ncplane* n;
  int vely, velx;
};

// we want them 6 rows tall and 12 columns wide
const int SHIPHEIGHT = 6;
const int SHIPWIDTH = 12;

static int
move_ships(struct notcurses* nc, struct ship* ships, unsigned shipcount){
  const struct ncplane* stdn = notcurses_stdplane_const(nc);
  for(unsigned s = 0 ; s < shipcount ; ++s){
    if(ships[s].n == NULL){
      continue;
    }
    int yoff, xoff;
    unsigned ny, nx;
    ncplane_yx(ships[s].n, &yoff, &xoff);
    ncplane_dim_yx(ships[s].n, &ny, &nx);
    unsigned dimy = ncplane_dim_y(stdn);
    unsigned dimx = ncplane_dim_x(stdn);
    yoff += ships[s].vely;
    xoff += ships[s].velx;
    if(xoff <= 0){
      xoff = 0;
      ships[s].velx = -ships[s].velx;
    }else if((unsigned)xoff >= dimx - nx){
      xoff = dimx - nx - 1;
      ships[s].velx = -ships[s].velx;
    }
    if(yoff <= 1){
      yoff = 2;
      ships[s].vely = -ships[s].vely;
    }else if((unsigned)yoff >= dimy - ny){
      yoff = dimy - ny - 1;
      ships[s].vely = -ships[s].vely;
    }
    ncplane_move_yx(ships[s].n, yoff, xoff);
  }
  return 0;
}

static int
get_ships(struct notcurses* nc, struct ship* ships, unsigned shipcount){
  char* pic = find_data("spaceship.png");
  struct ncvisual* wmv = ncvisual_from_file(pic);
  free(pic);
  if(wmv == NULL){
    return -1;
  }
  unsigned cdimy, cdimx;
  ncplane_pixel_geom(notcurses_stdplane(nc), NULL, NULL, &cdimy, &cdimx, NULL, NULL);
  if(cdimy == 0 || cdimx == 0){
    ncvisual_destroy(wmv);
    return 0;
  }
  if(ncvisual_resize(wmv, cdimy * SHIPHEIGHT, cdimx * SHIPWIDTH)){
    ncvisual_destroy(wmv);
    return -1;
  }
  struct ncvisual_options vopts = {
    .n = notcurses_stdplane(nc),
    .y = 30,//rand() % (ncplane_dim_y(notcurses_stdplane_const(nc)) - SHIPHEIGHT),
    .x = 30,//rand() % (ncplane_dim_x(notcurses_stdplane_const(nc)) - SHIPWIDTH),
    .blitter = NCBLIT_PIXEL,
    .flags = NCVISUAL_OPTION_CHILDPLANE,
  };
  for(unsigned s = 0 ; s < shipcount ; ++s){
    if((ships[s].n = ncvisual_blit(nc, wmv, &vopts)) == NULL){
      while(s--){
        ncplane_destroy(ships[s].n);
        ncvisual_destroy(wmv);
      }
      return -1;
    }
    ncplane_move_below(ships[s].n, notcurses_stdplane(nc));
    if((ships[s].vely = rand() % 6 - 3) == 0){
      ships[s].vely = 3;
    }
    if((ships[s].velx = rand() % 6 - 3) == 0){
      ships[s].velx = 3;
    }
  }
  ncvisual_destroy(wmv);
  return 0;
}

int box_demo(struct notcurses* nc, uint64_t startns){
  (void)startns;
  unsigned ylen, xlen;
  struct ncplane* n = notcurses_stddim_yx(nc, &ylen, &xlen);
  ncplane_erase(n);
  uint64_t transchan = 0;
  ncchannels_set_bg_alpha(&transchan, NCALPHA_TRANSPARENT);
  ncchannels_set_fg_alpha(&transchan, NCALPHA_TRANSPARENT);
  ncplane_set_base(n, "", 0, transchan);
  nccell ul = NCCELL_TRIVIAL_INITIALIZER, ll = NCCELL_TRIVIAL_INITIALIZER;
  nccell lr = NCCELL_TRIVIAL_INITIALIZER, ur = NCCELL_TRIVIAL_INITIALIZER;
  nccell hl = NCCELL_TRIVIAL_INITIALIZER, vl = NCCELL_TRIVIAL_INITIALIZER;
  if(nccells_double_box(n, 0, 0, &ul, &ur, &ll, &lr, &hl, &vl)){
    return -1;
  }
  // target grid is 7x7
  const int targx = 7;
  const int targy = 7;
  int ytargbase = (ylen - targy) / 2;
  ncplane_set_fg_rgb8(n, 180, 40, 180);
  ncplane_set_bg_default(n);
  if(notcurses_canutf8(nc)){
    if(utf8_target(n, ytargbase)){
      return -1;
    }
  }else{
    if(ascii_target(n, ytargbase)){
      return -1;
    }
  }
  if(nccell_set_fg_rgb8(&ul, 0xff, 0, 0)){
    return -1;
  }
  if(nccell_set_bg_rgb8(&ul, 20, 40, 20)){
    return -1;
  }
  if(nccell_set_fg_rgb8(&ur, 0, 0xff, 0)){
    return -1;
  }
  if(nccell_set_bg_rgb8(&ur, 20, 40, 20)){
    return -1;
  }
  if(nccell_set_fg_rgb8(&ll, 0, 0, 0xff)){
    return -1;
  }
  if(nccell_set_bg_rgb8(&ll, 20, 40, 20)){
    return -1;
  }
  if(nccell_set_fg_rgb8(&lr, 0xff, 0xff, 0xff)){
    return -1;
  }
  if(nccell_set_bg_rgb8(&lr, 20, 40, 20)){
    return -1;
  }
  int y = 1, x = 0;
  ncplane_dim_yx(n, &ylen, &xlen);
  --ylen;
  while((int)ylen - y >= targy && (int)xlen - x >= targx){
    if(ncplane_cursor_move_yx(n, y, x)){
      return -1;
    }
    if(ncplane_box_sized(n, &ul, &ur, &ll, &lr, &hl, &vl, ylen, xlen,
                          NCBOXGRAD_LEFT | NCBOXGRAD_BOTTOM |
                          NCBOXGRAD_RIGHT | NCBOXGRAD_TOP)){
      return -1;
    }
    ylen -= 2;
    xlen -= 2;
    ++y;
    ++x;
  }
  int iters = 100;
  struct timespec iterdelay;
  ns_to_timespec(timespec_to_ns(&demodelay) * 3 / iters, &iterdelay);
  struct ship ships[3] = {0};
  if(notcurses_canopen_images(nc)){
    if(get_ships(nc, ships, sizeof(ships) / sizeof(*ships))){
      return -1;
    }
  }
  while(iters--){
    if(reload_corners(n, &ul, &ur, &ll, &lr)){
      return -1;
    }
    y = 1;
    x = 0;
    ncplane_dim_yx(n, &ylen, &xlen);
    --ylen;
    move_ships(nc, ships, sizeof(ships) / sizeof(*ships));
    while((int)ylen - y >= targy && (int)xlen - x >= targx){
      if(ncplane_cursor_move_yx(n, y, x)){
        return -1;
      }
      if(ncplane_box_sized(n, &ul, &ur, &ll, &lr, &hl, &vl, ylen, xlen,
                           NCBOXGRAD_LEFT | NCBOXGRAD_BOTTOM |
                           NCBOXGRAD_RIGHT | NCBOXGRAD_TOP)){
        return -1;
      }
      ylen -= 2;
      xlen -= 2;
      ++y;
      ++x;
    }
    DEMO_RENDER(nc);
    nanosleep(&iterdelay, NULL);
  }
  for(unsigned s = 0 ; s < sizeof(ships) / sizeof(*ships) ; ++s){
    ncplane_destroy(ships[s].n);
  }
  nccell_release(n, &ul);
  nccell_release(n, &ur);
  nccell_release(n, &ll);
  nccell_release(n, &lr);
  nccell_release(n, &hl);
  nccell_release(n, &vl);
  return 0;
}
