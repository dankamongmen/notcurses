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

static int
slideitslideit(struct notcurses* nc, struct ncplane* n, uint64_t deadline,
               int* direction){
  int dimy, dimx;
  int yoff, xoff;
  int ny, nx;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  ncplane_dim_yx(n, &ny, &nx);
  ncplane_yx(n, &yoff, &xoff);
  struct timespec iterdelay = { .tv_sec = 0, .tv_nsec = 10000000, };
  struct timespec cur;
  do{
    if(notcurses_render(nc)){
      return -1;
    }
    switch(*direction){
      case 0: --yoff; --xoff; break;
      case 1: --yoff; ++xoff; break;
      case 2: ++yoff; ++xoff; break;
      case 3: ++yoff; --xoff; break;
    }
    if(xoff == 0){
      ++xoff;
      if(*direction == 0){
        *direction = 1;
      }else if(*direction == 3){
        *direction = 2;
      }
    }else if(xoff == dimx - nx){
      --xoff;
      if(*direction == 1){
        *direction = 2;
      }else if(*direction == 2){
        *direction = 3;
      }
    }
    if(yoff == 0){
      ++yoff;
      if(*direction == 0){
        *direction = 3;
      }else if(*direction == 1){
        *direction = 2;
      }
    }else if(yoff == dimy - ny){
      --yoff;
      if(*direction == 2){
        *direction = 1;
      }else if(*direction == 3){
        *direction = 0;
      }
    }
    ncplane_move_yx(n, yoff, xoff);
    nanosleep(&iterdelay, NULL);
    clock_gettime(CLOCK_MONOTONIC, &cur);
  }while(timespec_to_ns(&cur) < deadline);
  return 0;
}

// run panels atop the display in an exploration of transparency
static int
slidepanel(struct notcurses* nc){
  int dimy, dimx;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  int ny = dimy / 4;
  int nx = dimx / 3;
  int yoff = random() % (dimy - ny - 2) + 1; // don't start atop a border
  int xoff = random() % (dimx - nx - 2) + 1;
  // First we'll have one using the default background. This will typically be
  // either white, black, or transparent (to the console, not other planes).
  struct ncplane* n = notcurses_newplane(nc, ny, nx, yoff, xoff, NULL);
  struct timespec cur;
  clock_gettime(CLOCK_MONOTONIC, &cur);
  uint64_t deadlinens = timespec_to_ns(&cur) + 5 * timespec_to_ns(&demodelay);
  int direction = random() % 4;
  if(slideitslideit(nc, n, deadlinens, &direction)){
    ncplane_destroy(n);
    return -1;
  }

  // Now, we use an explicitly black background, but blend it.
  cell c = CELL_SIMPLE_INITIALIZER(' ');
  cell_set_bg_alpha(&c, CELL_ALPHA_BLEND);
  cell_set_bg(&c, 0);
  ncplane_set_default(n, &c);
  cell_release(n, &c);
  clock_gettime(CLOCK_MONOTONIC, &cur);
  deadlinens = timespec_to_ns(&cur) + 5 * timespec_to_ns(&demodelay);
  if(slideitslideit(nc, n, deadlinens, &direction)){
    ncplane_destroy(n);
    return -1;
  }
  
  // Finally, we populate the plane for the first time with non-transparent
  // characters. We blend, however, to show the underlying color in our glyphs.
  cell_load_simple(n, &c, 'X');
  cell_set_fg_alpha(&c, CELL_ALPHA_BLEND);
  cell_set_bg_alpha(&c, CELL_ALPHA_OPAQUE);
  cell_set_bg(&c, 0);
  ncplane_set_default(n, &c);
  cell_release(n, &c);
  clock_gettime(CLOCK_MONOTONIC, &cur);
  deadlinens = timespec_to_ns(&cur) + 5 * timespec_to_ns(&demodelay);
  if(slideitslideit(nc, n, deadlinens, &direction)){
    ncplane_destroy(n);
    return -1;
  }

  return ncplane_destroy(n);
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
      ncplane_set_fg_rgb(n, (rgb & 0xff0000) >> 16u, (rgb & 0xff00) >> 8u, rgb & 0xff);
      ncplane_set_bg_rgb(n, 0, 10, 0);
      ncplane_putsimple(n, x % 10 + '0');
      grow_rgb(&rgb);
      ++x;
    }
  }
  if(notcurses_render(nc)){
    return -1;
  }
  nanosleep(&demodelay, NULL);
  return slidepanel(nc);
}
