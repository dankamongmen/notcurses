#include <stdlib.h>
#include <unistd.h>
#include "demo.h"

int box_demo(struct notcurses* nc){
  int ylen, xlen;
  notcurses_term_dim_yx(nc, &ylen, &xlen);
  struct ncplane* n = notcurses_stdplane(nc);
  ncplane_erase(n);
  cell ul = CELL_TRIVIAL_INITIALIZER, ll = CELL_TRIVIAL_INITIALIZER;
  cell lr = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
  cell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
  ncplane_fg_rgb8(n, 255, 255, 255);
  ncplane_bg_rgb8(n, 180, 40, 180);
  if(cells_double_box(n, 0, 0, &ul, &ur, &ll, &lr, &hl, &vl)){
    return -1;
  }
  int y = 0, x = 0;
  while(ylen - y - 1 > 2 && xlen - x - 1 > 2){
    cell_set_fg(&ul, 107 - (y * 2), 40, 107 + (y * 2));
    cell_set_bg(&ul, 20 + y, 20 + y, 20 + y);
    cell_set_fg(&ur, 107 - (y * 2), 40, 107 + (y * 2));
    cell_set_bg(&ur, 20 + y, 20 + y, 20 + y);
    cell_set_fg(&hl, 107 - (y * 2), 40, 107 + (y * 2));
    cell_set_bg(&hl, 20, 20, 20);
    cell_set_fg(&ll, 107 - (y * 2), 40, 107 + (y * 2));
    cell_set_bg(&ll, 20 + y, 20 + y, 20 + y);
    cell_set_fg(&lr, 107 - (y * 2), 40, 107 + (y * 2));
    cell_set_bg(&lr, 20 + y, 20 + y, 20 + y);
    cell_set_fg(&vl, 20, 20, 20);
    cell_set_bg(&vl, 107 - (y * 2), 40, 107 + (y * 2));
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
  cell_release(n, &ul);
  cell_release(n, &ur);
  cell_release(n, &ll);
  cell_release(n, &lr);
  cell_release(n, &hl);
  cell_release(n, &vl);
  if(notcurses_render(nc)){
    return -1;
  }
  nanosleep(&demodelay, NULL);
  return 0;
}
