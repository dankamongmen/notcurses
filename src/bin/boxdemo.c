#include <stdlib.h>
#include <unistd.h>
#include "demo.h"

int box_demo(struct notcurses* nc){
  int ymax, xmax;
  notcurses_term_dimyx(nc, &ymax, &xmax);
  struct ncplane* n = notcurses_stdplane(nc);
  cell ul, ll, lr, ur, hl, vl;
  ncplane_fg_rgb8(n, 255, 255, 255);
  cell_init(&ul);
  cell_init(&ur);
  cell_init(&ll);
  cell_init(&lr);
  cell_init(&hl);
  cell_init(&vl);
  cell_load(n, &ul, "╭");
  cell_load(n, &ur, "╮");
  cell_load(n, &ll, "╰");
  cell_load(n, &lr, "╯");
  cell_load(n, &vl, "│");
  cell_load(n, &hl, "─");
  cell_set_fg(&hl, 107, 40, 107);
  cell_set_bg(&vl, 107, 40, 107);
  int y = 0, x = 0;
  while(ymax - y - 1 > 2 && xmax - x - 1 > 2){
    ncplane_cursor_move_yx(n, y, x);
    if(ncplane_box(n, &ul, &ur, &ll, &lr, &hl, &vl, ymax, xmax)){
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
