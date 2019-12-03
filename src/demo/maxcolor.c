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
  cell style = CELL_TRIVIAL_INITIALIZER;
  cell_set_fg(&style, 0, 128, 128);
  cell_set_bg(&style, 90, 0, 90);
  int y = 0, x = 0;
  ncplane_cursor_move_yx(n, y, x);
  if(ncplane_rounded_box_sized(n, style.attrword, style.channels, maxy, maxx)){
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
  cell_release(n, &style);
  nanosleep(&demodelay, NULL);
  return 0;
}
