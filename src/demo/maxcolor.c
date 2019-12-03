#include <unistd.h>
#include "demo.h"

static void
grow_rgb(uint32_t* rgb){
  int r = cell_rgb_red(*rgb);
  int g = cell_rgb_green(*rgb);
  int b = cell_rgb_blue(*rgb);
  int delta = (*rgb & 0x80000000ul) ? -1 : 1;
  if(b == r){
    b += delta;
  }else if(g == r){
    g += delta;
  }else{
    r += delta;
  }
  if(b == 256 || r == 256 || g == 256){
    b = r = g = 255;
    *rgb |= 0x80000000ul;
  }else if(b == -1 || r == -1 || g == -1){
    b = r = g = 0;
    *rgb &= ~0x80000000ul;
  }
  *rgb = (*rgb & 0xff000000ul) | (r * 65536 + g * 256 + b);
}

// draws a border along the perimeter, then fills the inside with position
// markers, each a slightly different color. the goal is to make sure we can
// have a great many colors, that they progress reasonably through the space,
// and that we can write to every coordinate.
int maxcolor_demo(struct notcurses* nc){
  int maxx, maxy;
  notcurses_term_dim_yx(nc, &maxy, &maxx);
  struct ncplane* n = notcurses_stdplane(nc);
  ncplane_fg_rgb8(n, 255, 255, 255);
  cell ul = CELL_TRIVIAL_INITIALIZER;
  cell ll = CELL_TRIVIAL_INITIALIZER;
  cell lr = CELL_TRIVIAL_INITIALIZER;
  cell ur = CELL_TRIVIAL_INITIALIZER;
  cell hl = CELL_TRIVIAL_INITIALIZER;
  cell vl = CELL_TRIVIAL_INITIALIZER;
  if(cells_rounded_box(n, &ul, &ur, &ll, &lr, &hl, &vl)){
    return -1;
  }
  cell_set_fg(&ul, 0, 128, 128);
  cell_set_fg(&ur, 0, 128, 128);
  cell_set_fg(&ll, 0, 128, 128);
  cell_set_fg(&lr, 0, 128, 128);
  cell_set_fg(&vl, 0, 128, 128);
  cell_set_fg(&hl, 0, 128, 128);
  cell_set_bg(&ul, 90, 0, 90);
  cell_set_bg(&ur, 90, 0, 90);
  cell_set_bg(&ll, 90, 0, 90);
  cell_set_bg(&lr, 90, 0, 90);
  cell_set_bg(&vl, 90, 0, 90);
  cell_set_bg(&hl, 90, 0, 90);
  int y = 0, x = 0;
  ncplane_cursor_move_yx(n, y, x);
  if(ncplane_box_sized(n, &ul, &ur, &ll, &lr, &hl, &vl, maxy, maxx)){
    return -1;
  }
  uint32_t rgb = 0;
  char buf[2] = "";
  cell c = CELL_TRIVIAL_INITIALIZER;
  for(y = 1 ; y < maxy - 1 ; ++y){
    x = 1;
    if(ncplane_cursor_move_yx(n, y, x)){
      return -1;
    }
    while(x < maxx - 1){
      cell_set_fg(&c, (rgb & 0xff0000) >> 16u, (rgb & 0xff00) >> 8u, rgb & 0xff);
      cell_set_bg(&c, 0, 10, 0);
      buf[0] = (x % 10) + '0';

      cell_load(n, &c, buf);
      ncplane_putc(n, &c);
      grow_rgb(&rgb);
      ++x;
    }
  }
  cell_release(n, &c);
  if(notcurses_render(nc)){
    return -1;
  }
  cell_release(n, &ul);
  cell_release(n, &ur);
  cell_release(n, &ll);
  cell_release(n, &lr);
  cell_release(n, &vl);
  cell_release(n, &hl);
  nanosleep(&demodelay, NULL);
  return 0;
}
