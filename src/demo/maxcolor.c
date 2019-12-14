#include <unistd.h>
#include "demo.h"

static void
grow_rgb(uint32_t* rgb){
  int r = channel_get_r(*rgb);
  int g = channel_get_g(*rgb);
  int b = channel_get_b(*rgb);
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
  ncplane_set_fg_rgb(n, 255, 255, 255);
  uint64_t channels = 0;
  channels_set_fg_rgb(&channels, 0, 128, 128);
  channels_set_bg_rgb(&channels, 90, 0, 90);
  int y = 0, x = 0;
  ncplane_cursor_move_yx(n, y, x);
  if(ncplane_rounded_box_sized(n, 0, channels, maxy, maxx, 0)){
    return -1;
  }
  uint32_t rgb = 0;
  for(y = 1 ; y < maxy - 1 ; ++y){
    x = 1;
    if(ncplane_cursor_move_yx(n, y, x)){
      return -1;
    }
    while(x < maxx - 1){
      channels_set_fg_rgb(&channels, (rgb & 0xff0000) >> 16u,
                        (rgb & 0xff00) >> 8u, rgb & 0xff);
      channels_set_bg_rgb(&channels, 0, 10, 0);
      ncplane_putsimple(n, x % 10 + '0', 0, channels);
      grow_rgb(&rgb);
      ++x;
    }
  }
  if(notcurses_render(nc)){
    return -1;
  }
  nanosleep(&demodelay, NULL);
  return 0;
}
