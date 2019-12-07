#include <stdlib.h>
#include <unistd.h>
#include "demo.h"

#define ITERATIONS 10

int box_demo(struct notcurses* nc){
  const int64_t totalns = timespec_to_ns(&demodelay);
  struct timespec iterdelay;
  timespec_div(&demodelay, ITERATIONS, &iterdelay);
  struct ncplane* n = notcurses_stdplane(nc);
  ncplane_erase(n);
  cell ul = CELL_TRIVIAL_INITIALIZER, ll = CELL_TRIVIAL_INITIALIZER;
  cell lr = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
  cell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
  ncplane_set_fg_rgb(n, 255, 255, 255);
  ncplane_set_bg_rgb(n, 180, 40, 180);
  if(cells_double_box(n, 0, 0, &ul, &ur, &ll, &lr, &hl, &vl)){
    return -1;
  }
  struct timespec start, now;
  clock_gettime(CLOCK_MONOTONIC_RAW, &start);
  int zbonus = 40;
  int zbonusdelta = 20;
  do{
    int ylen, xlen;
    ncplane_dim_yx(n, &ylen, &xlen);
    int y = 0, x = 0;
    while(ylen - y - 1 > 2 && xlen - x - 1 > 2){
      cell_set_fg(&ul, 107 - (y * 2), zbonus, 107 + (y * 2));
      cell_set_bg(&ul, zbonus, 20 + y, 20 + y);
      cell_set_fg(&ur, 107 - (y * 2), zbonus, 107 + (y * 2));
      cell_set_bg(&ur, zbonus, 20 + y, 20 + y);
      cell_set_fg(&hl, 107 - (y * 2), zbonus, 107 + (y * 2));
      cell_set_bg(&hl, 20, zbonus, 20);
      cell_set_fg(&ll, 107 - (y * 2), zbonus, 107 + (y * 2));
      cell_set_bg(&ll, zbonus, 20 + y, 20 + y);
      cell_set_fg(&lr, 107 - (y * 2), zbonus, 107 + (y * 2));
      cell_set_bg(&lr, zbonus, 20 + y, 20 + y);
      cell_set_fg(&vl, 20, zbonus, 20);
      cell_set_bg(&vl, 107 - (y * 2), zbonus, 107 + (y * 2));
      if(ncplane_cursor_move_yx(n, y, x)){
        return -1;
      }
      if(ncplane_box_sized(n, &ul, &ur, &ll, &lr, &hl, &vl, ylen, xlen, 0)){
        return -1;
      }
      ylen -= 2;
      xlen -= 2;
      ++y;
      ++x;
    }
    if(notcurses_render(nc)){
      return -1;
    }
    nanosleep(&iterdelay, NULL);
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    if((zbonus += zbonusdelta > 255) || zbonus < 0){
      zbonusdelta = -zbonusdelta;
      zbonus += zbonusdelta;
    }
  }while(timespec_subtract_ns(&now, &start) <= totalns);
  cell_release(n, &ul);
  cell_release(n, &ur);
  cell_release(n, &ll);
  cell_release(n, &lr);
  cell_release(n, &hl);
  cell_release(n, &vl);
  return 0;
}
