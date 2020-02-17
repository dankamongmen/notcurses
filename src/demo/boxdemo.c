#include <stdlib.h>
#include <unistd.h>
#include "demo.h"

static int
reload_corners(struct ncplane* n, cell* ul, cell* ur, cell* ll, cell* lr){
  int dimy, dimx;

  ncplane_dim_yx(n, &dimy, &dimx);
  cell c = CELL_TRIVIAL_INITIALIZER;
  if(ncplane_at_yx(n, 1, dimx - 1, &c) < 0){
    cell_release(n, &c);
    return -1;
  }
  ul->channels = c.channels;
  if(ncplane_at_yx(n, dimy - 1, dimx - 1, &c) < 0){
    cell_release(n, &c);
    return -1;
  }
  ur->channels = c.channels;
  if(ncplane_at_yx(n, dimy - 1, 0, &c) < 0){
    cell_release(n, &c);
    return -1;
  }
  lr->channels = c.channels;
  if(ncplane_at_yx(n, 1, 0, &c) < 0){
    cell_release(n, &c);
    return -1;
  }
  ll->channels = c.channels;
  cell_release(n, &c);
  return 0;
}

int box_demo(struct notcurses* nc){
  struct ncplane* n = notcurses_stdplane(nc);
  ncplane_erase(n);
  cell ul = CELL_TRIVIAL_INITIALIZER, ll = CELL_TRIVIAL_INITIALIZER;
  cell lr = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
  cell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
  if(cells_double_box(n, 0, 0, &ul, &ur, &ll, &lr, &hl, &vl)){
    return -1;
  }
  int ylen, xlen;
  ncplane_dim_yx(n, &ylen, &xlen);
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
