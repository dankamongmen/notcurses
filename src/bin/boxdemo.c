#include <stdlib.h>
#include <unistd.h>
#include "demo.h"

int box_demo(struct notcurses* nc){
  int ymax, xmax;
  notcurses_term_dimyx(nc, &ymax, &xmax);
  struct ncplane* n = notcurses_stdplane(nc);
  cell ul, ll, lr, ur, hl, vl;
  ncplane_fg_rgb8(n, 255, 255, 255);
  ncplane_bg_rgb8(n, 180, 40, 180);
  cell_init(&ul);
  cell_init(&ur);
  cell_init(&ll);
  cell_init(&lr);
  cell_init(&hl);
  cell_init(&vl);
  if(ncplane_rounded_box_cells(n, &ul, &ur, &ll, &lr, &hl, &vl)){
    return -1;
  }
  int y = 0, x = 0;
  while(ymax - y - 1 > 2 && xmax - x - 1 > 2){
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
    if(ncplane_box_sized(n, &ul, &ur, &ll, &lr, &hl, &vl, ymax, xmax)){
      return -1;
    }
    ymax -= 2;
    xmax -= 2;
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
  sleep(1);
  return 0;
}
