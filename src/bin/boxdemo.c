#include <stdlib.h>
#include <unistd.h>
#include "demo.h"

int box_demo(struct notcurses* nc){
  int ymax, xmax;
  notcurses_term_dimyx(nc, &ymax, &xmax);
  struct ncplane* n = notcurses_stdplane(nc);
  ncplane_cursor_move_yx(n, 0, 0);
  cell ul, ll, lr, ur, hl, vl;
  ncplane_erase(n);
  if(notcurses_render(nc)){
    return -1;
  }
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
  if(ncplane_box(n, &ul, &ur, &ll, &lr, &hl, &vl, ymax, xmax)){
    return -1;
  }
  ncplane_cursor_move_yx(n, 1, 1);
  if(ncplane_hline(n, &hl, xmax - 2) != xmax - 2){
    return -1;
  }
  ncplane_cursor_move_yx(n, ymax - 2, 1);
  if(ncplane_hline(n, &hl, xmax - 2) != xmax - 2){
    return -1;
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
