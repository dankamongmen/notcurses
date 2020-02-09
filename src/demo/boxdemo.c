#include <stdlib.h>
#include <unistd.h>
#include "demo.h"

int box_demo(struct notcurses* nc){
  struct timespec iterdelay;
  timespec_div(&demodelay, 256, &iterdelay);
  struct ncplane* n = notcurses_stdplane(nc);
  ncplane_erase(n);
  cell ul = CELL_TRIVIAL_INITIALIZER, ll = CELL_TRIVIAL_INITIALIZER;
  cell lr = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
  cell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
  if(cells_double_box(n, 0, 0, &ul, &ur, &ll, &lr, &hl, &vl)){
    return -1;
  }
  struct timespec start, now;
  clock_gettime(CLOCK_MONOTONIC, &start);
  int zbonus = 40;
  int zbonusdelta = 20;
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
  for(int idx = 0 ; idx < 256 ; ++idx){
    int y = 1, x = 0;
    ncplane_dim_yx(n, &ylen, &xlen);
    --ylen;
    while(ylen - y >= targy && xlen - x >= targx){
      if(cell_set_fg_rgb(&ul, idx, 255 - (y * 2), 255 - idx)){
        return -1;
      }
      if(cell_set_bg_rgb(&ul, 20, zbonus, 20)){
        return -1;
      }
      if(cell_set_fg_rgb(&ur, idx / 2, zbonus, (255 - idx) / 2)){
        return -1;
      }
      if(cell_set_bg_rgb(&ur, 20, zbonus, 20)){
        return -1;
      }
      if(cell_set_fg_rgb(&ll, idx / 2, zbonus, (255 - idx) / 2)){
        return -1;
      }
      if(cell_set_bg_rgb(&ll, 20, zbonus, 20)){
        return -1;
      }
      if(cell_set_fg_rgb(&lr, 255 - idx, 255 - (y * 2), zbonus)){
        return -1;
      }
      if(cell_set_bg_rgb(&lr, 20, zbonus, 20)){
        return -1;
      }
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
    if(demo_render(nc)){
      return -1;
    }
    nanosleep(&iterdelay, NULL);
    clock_gettime(CLOCK_MONOTONIC, &now);
    if((zbonus += zbonusdelta > 255) || zbonus < 0){
      zbonusdelta = -zbonusdelta;
      zbonus += zbonusdelta;
    }
  }
  cell_release(n, &ul);
  cell_release(n, &ur);
  cell_release(n, &ll);
  cell_release(n, &lr);
  cell_release(n, &hl);
  cell_release(n, &vl);
  return 0;
}
