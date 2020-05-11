#include <stdlib.h>
#include <unistd.h>
#include "demo.h"

static int
reload_corners(struct ncplane* n, cell* ul, cell* ur, cell* ll, cell* lr){
  int dimy, dimx;
  ncplane_dim_yx(n, &dimy, &dimx);
  char* egc;
  if( (egc = ncplane_at_yx(n, 1, dimx - 1, NULL, &ul->channels)) == NULL){
    return -1;
  }
  free(egc);
  if( (egc = ncplane_at_yx(n, dimy - 1, dimx - 1, NULL, &ur->channels)) == NULL){
    return -1;
  }
  free(egc);
  if( (egc = ncplane_at_yx(n, dimy - 1, 0, NULL, &lr->channels)) == NULL){
    return -1;
  }
  free(egc);
  if( (egc = ncplane_at_yx(n, 1, 0, NULL, &ll->channels)) == NULL){
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

int box_demo(struct notcurses* nc){
  int ylen, xlen;
  struct ncplane* n = notcurses_stddim_yx(nc, &ylen, &xlen);
  ncplane_erase(n);
  cell ul = CELL_TRIVIAL_INITIALIZER, ll = CELL_TRIVIAL_INITIALIZER;
  cell lr = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
  cell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
  if(cells_double_box(n, 0, 0, &ul, &ur, &ll, &lr, &hl, &vl)){
    return -1;
  }
  // target grid is 7x7
  const int targx = 7;
  const int targy = 7;
  int ytargbase = (ylen - targy) / 2;
  cell c = CELL_SIMPLE_INITIALIZER(' ');
  cell_set_bg_default(&c);
  ncplane_set_base_cell(n, &c);
  cell_release(n, &c);
  ncplane_set_fg_rgb(n, 180, 40, 180);
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
  if(cell_set_fg_rgb(&ul, 0xff, 0, 0)){
    return -1;
  }
  if(cell_set_bg_rgb(&ul, 20, 40, 20)){
    return -1;
  }
  if(cell_set_fg_rgb(&ur, 0, 0xff, 0)){
    return -1;
  }
  if(cell_set_bg_rgb(&ur, 20, 40, 20)){
    return -1;
  }
  if(cell_set_fg_rgb(&ll, 0, 0, 0xff)){
    return -1;
  }
  if(cell_set_bg_rgb(&ll, 20, 40, 20)){
    return -1;
  }
  if(cell_set_fg_rgb(&lr, 0xff, 0xff, 0xff)){
    return -1;
  }
  if(cell_set_bg_rgb(&lr, 20, 40, 20)){
    return -1;
  }
  int y = 1, x = 0;
  ncplane_dim_yx(n, &ylen, &xlen);
  --ylen;
  while(ylen - y >= targy && xlen - x >= targx){
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
  int iters = 50;
  struct timespec iterdelay;
  ns_to_timespec(timespec_to_ns(&demodelay) * 3 / iters, &iterdelay);
  nanosleep(&iterdelay, NULL);
  while(iters--){
    if(reload_corners(n, &ul, &ur, &ll, &lr)){
      return -1;
    }
    y = 1;
    x = 0;
    ncplane_dim_yx(n, &ylen, &xlen);
    --ylen;
    while(ylen - y >= targy && xlen - x >= targx){
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
  cell_release(n, &ul);
  cell_release(n, &ur);
  cell_release(n, &ll);
  cell_release(n, &lr);
  cell_release(n, &hl);
  cell_release(n, &vl);
  return 0;
}
