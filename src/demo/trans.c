#include <unistd.h>
#include "demo.h"

static void
grow_rgb(uint32_t* rgb){
  int r = channel_r(*rgb);
  int g = channel_g(*rgb);
  int b = channel_b(*rgb);
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

static struct ncplane*
legend(struct notcurses* nc, const char* msg){
  int dimx, dimy;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  // FIXME replace with ncplane_new_aligned()
  struct ncplane* n = ncplane_aligned(notcurses_stdplane(nc), 3,
                                      strlen(msg) + 4, dimy - 4,
                                      NCALIGN_CENTER, NULL);
  if(n == NULL){
    return NULL;
  }
  cell c = CELL_TRIVIAL_INITIALIZER;
  cell_set_fg_rgb(&c, 0, 0, 0); // darken surrounding characters by half
  cell_set_fg_alpha(&c, CELL_ALPHA_BLEND);
  cell_set_bg_alpha(&c, CELL_ALPHA_TRANSPARENT); // don't touch background
  ncplane_set_base(n, &c);
  ncplane_set_fg(n, 0xd78700);
  ncplane_set_bg(n, 0);
  if(ncplane_printf_aligned(n, 1, NCALIGN_CENTER, " %s ", msg) < 0){
    ncplane_destroy(n);
    return NULL;
  }
  return n;
}

static int
slideitslideit(struct notcurses* nc, struct ncplane* n, uint64_t deadline,
               int* vely, int* velx){
  int dimy, dimx;
  int yoff, xoff;
  int ny, nx;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  ncplane_dim_yx(n, &ny, &nx);
  ncplane_yx(n, &yoff, &xoff);
  struct timespec iterdelay = { .tv_sec = 0, .tv_nsec = 25000000, };
  struct timespec cur;
  do{
    if(demo_render(nc)){
      return -1;
    }
    yoff += *vely;
    xoff += *velx;
    if(xoff <= 0){
      xoff = 0;
      *velx = -*velx;
    }else if(xoff >= dimx - nx){
      xoff = dimx - nx - 1;
      *velx = -*velx;
    }
    if(yoff <= 0){
      yoff = 0;
      *vely = -*vely;
    }else if(yoff >= dimy - ny){
      yoff = dimy - ny - 1;
      *vely = -*vely;
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
  const int DELAYSCALE = 2;
  int dimy, dimx;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  int ny = dimy / 4;
  int nx = dimx / 3;
  int yoff = random() % (dimy - ny - 2) + 1; // don't start atop a border
  int xoff = random() % (dimx - nx - 2) + 1;
  struct ncplane* l;

  // First we just create a plane with no styling and no glyphs.
  struct ncplane* n = ncplane_new(nc, ny, nx, yoff, xoff, NULL);

  // Zero-initialized channels use the default color, opaquely. Since we have
  // no glyph, we should show underlying glyphs in the default colors. The
  // background default might be transparent, at the window level (i.e. a copy
  // of the underlying desktop).
  cell c = CELL_SIMPLE_INITIALIZER(' ');
  struct timespec cur;
  ncplane_set_base(n, &c);
  clock_gettime(CLOCK_MONOTONIC, &cur);
  uint64_t deadlinens = timespec_to_ns(&cur) + DELAYSCALE * timespec_to_ns(&demodelay);
  int velx = random() % 4 + 1;
  int vely = random() % 4 + 1;
  l = legend(nc, "default background, all opaque, whitespace glyph");
  if(slideitslideit(nc, n, deadlinens, &vely, &velx)){
    ncplane_destroy(n);
    ncplane_destroy(l);
    return -1;
  }
  ncplane_destroy(l);

  cell_load_simple(n, &c, '\0');
  ncplane_set_base(n, &c);
  clock_gettime(CLOCK_MONOTONIC, &cur);
  deadlinens = timespec_to_ns(&cur) + DELAYSCALE * timespec_to_ns(&demodelay);
  l = legend(nc, "default background, all opaque, no glyph");
  if(slideitslideit(nc, n, deadlinens, &vely, &velx)){
    ncplane_destroy(n);
    ncplane_destroy(l);
    return -1;
  }
  ncplane_destroy(l);

  // Next, we set our foreground transparent, allowing characters underneath to
  // be seen in their natural colors. Our background remains opaque+default.
  cell_set_fg_alpha(&c, CELL_ALPHA_TRANSPARENT);
  ncplane_set_base(n, &c);
  clock_gettime(CLOCK_MONOTONIC, &cur);
  deadlinens = timespec_to_ns(&cur) + DELAYSCALE * timespec_to_ns(&demodelay);
  l = legend(nc, "default background, fg transparent, no glyph");
  if(slideitslideit(nc, n, deadlinens, &vely, &velx)){
    ncplane_destroy(n);
    ncplane_destroy(l);
    return -1;
  }
  ncplane_destroy(l);

  // Set the foreground color, setting it to blend. We should get the underlying
  // glyphs in a blended color, with the default background color.
  cell_set_fg(&c, 0x80c080);
  cell_set_fg_alpha(&c, CELL_ALPHA_BLEND);
  ncplane_set_base(n, &c);
  clock_gettime(CLOCK_MONOTONIC, &cur);
  l = legend(nc, "default background, fg blended, no glyph");
  deadlinens = timespec_to_ns(&cur) + DELAYSCALE * timespec_to_ns(&demodelay);
  if(slideitslideit(nc, n, deadlinens, &vely, &velx)){
    ncplane_destroy(n);
    ncplane_destroy(l);
    return -1;
  }
  ncplane_destroy(l);

  // Opaque foreground color. This produces underlying glyphs in the specified,
  // fixed color, with the default background color.
  cell_set_fg(&c, 0x80c080);
  cell_set_fg_alpha(&c, CELL_ALPHA_OPAQUE);
  ncplane_set_base(n, &c);
  clock_gettime(CLOCK_MONOTONIC, &cur);
  l = legend(nc, "default background, fg colored opaque, no glyph");
  deadlinens = timespec_to_ns(&cur) + DELAYSCALE * timespec_to_ns(&demodelay);
  if(slideitslideit(nc, n, deadlinens, &vely, &velx)){
    ncplane_destroy(n);
    ncplane_destroy(l);
    return -1;
  }
  ncplane_destroy(l);

  // Now we replace the characters with X's, colored as underneath us.
  // Our background color remains opaque default.
  cell_load_simple(n, &c, 'X');
  cell_set_fg_default(&c);
  cell_set_fg_alpha(&c, CELL_ALPHA_TRANSPARENT);
  cell_set_bg_alpha(&c, CELL_ALPHA_OPAQUE);
  ncplane_set_base(n, &c);
  clock_gettime(CLOCK_MONOTONIC, &cur);
  l = legend(nc, "default colors, fg transparent, print glyph");
  deadlinens = timespec_to_ns(&cur) + DELAYSCALE * timespec_to_ns(&demodelay);
  if(slideitslideit(nc, n, deadlinens, &vely, &velx)){
    ncplane_destroy(n);
    ncplane_destroy(l);
    return -1;
  }
  ncplane_destroy(l);

  // Now we replace the characters with X's, but draw the foreground and
  // background color from below us.
  cell_set_fg_alpha(&c, CELL_ALPHA_TRANSPARENT);
  cell_set_bg_alpha(&c, CELL_ALPHA_TRANSPARENT);
  ncplane_set_base(n, &c);
  clock_gettime(CLOCK_MONOTONIC, &cur);
  l = legend(nc, "all transparent, print glyph");
  deadlinens = timespec_to_ns(&cur) + DELAYSCALE * timespec_to_ns(&demodelay);
  if(slideitslideit(nc, n, deadlinens, &vely, &velx)){
    ncplane_destroy(n);
    ncplane_destroy(l);
    return -1;
  }
  ncplane_destroy(l);

  // Finally, we populate the plane for the first time with non-transparent
  // characters. We blend, however, to show the underlying color in our glyphs.
  cell_set_fg_alpha(&c, CELL_ALPHA_BLEND);
  cell_set_bg_alpha(&c, CELL_ALPHA_BLEND);
  cell_set_fg(&c, 0x80c080);
  cell_set_bg(&c, 0x204080);
  ncplane_set_base(n, &c);
  clock_gettime(CLOCK_MONOTONIC, &cur);
  l = legend(nc, "all blended, print glyph");
  deadlinens = timespec_to_ns(&cur) + DELAYSCALE * timespec_to_ns(&demodelay);
  if(slideitslideit(nc, n, deadlinens, &vely, &velx)){
    ncplane_destroy(n);
    ncplane_destroy(l);
    return -1;
  }
  ncplane_destroy(l);

  return ncplane_destroy(n);
}

// draws a border along the perimeter, then fills the inside with position
// markers, each a slightly different color. the goal is to make sure we can
// have a great many colors, that they progress reasonably through the space,
// and that we can write to every coordinate.
int trans_demo(struct notcurses* nc){
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
  struct ncplane* l = legend(nc, "what say we explore transparency together?");
  if(demo_render(nc)){
    return -1;
  }
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC_RAW, &now);
  ncplane_pulse(l, &demodelay, pulser, &now);
  ncplane_destroy(l);
  return slidepanel(nc);
}
