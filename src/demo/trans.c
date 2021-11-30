#include <unistd.h>
#include "demo.h"

static void
grow_rgb8(uint32_t* rgb){
  int r = ncchannel_r(*rgb);
  int g = ncchannel_g(*rgb);
  int b = ncchannel_b(*rgb);
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
  unsigned dimx, dimy;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  ncplane_options nopts = {
    .rows = 3,
    .cols = strlen(msg) + 4,
    .y = 3,
    .x = NCALIGN_CENTER,
    .flags = NCPLANE_OPTION_HORALIGNED,
  };
  struct ncplane* n = ncplane_create(notcurses_stdplane(nc), &nopts);
  if(n == NULL){
    return NULL;
  }
  nccell c = NCCELL_TRIVIAL_INITIALIZER;
  nccell_set_fg_rgb8(&c, 0, 0, 0); // darken surrounding characters by half
  nccell_set_fg_alpha(&c, NCALPHA_BLEND);
  nccell_set_bg_alpha(&c, NCALPHA_TRANSPARENT); // don't touch background
  if(ncplane_set_base_cell(n, &c)){
    ncplane_destroy(n);
    return NULL;
  }
  if(ncplane_set_fg_rgb(n, 0xd78700) || ncplane_set_bg_rgb(n, 0)){
    ncplane_destroy(n);
    return NULL;
  }
  ncplane_on_styles(n, NCSTYLE_BOLD | NCSTYLE_ITALIC);
  if(ncplane_printf_aligned(n, 1, NCALIGN_CENTER, " %s ", msg) < 0){
    ncplane_destroy(n);
    return NULL;
  }
  return n;
}

static int
slideitslideit(struct notcurses* nc, struct ncplane* n, uint64_t deadline,
               int* vely, int* velx){
  int yoff, xoff;
  unsigned dimy, dimx;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  unsigned ny, nx;
  ncplane_dim_yx(n, &ny, &nx);
  ncplane_yx(n, &yoff, &xoff);
  struct timespec iterdelay;
  timespec_div(&demodelay, 60, &iterdelay);
  struct timespec cur;
  do{
    DEMO_RENDER(nc);
    yoff += *vely;
    xoff += *velx;
    if(xoff <= 1){
      xoff = 1;
      *velx = -*velx;
    }else if((unsigned)xoff >= dimx - nx){
      xoff = dimx - nx - 1;
      *velx = -*velx;
    }
    if(yoff <= 2){
      yoff = 2;
      *vely = -*vely;
    }else if((unsigned)yoff >= dimy - ny){
      yoff = dimy - ny - 1;
      *vely = -*vely;
    }
    ncplane_move_yx(n, yoff, xoff);
    demo_nanosleep(nc, &iterdelay);
    clock_gettime(CLOCK_MONOTONIC, &cur);
  }while(timespec_to_ns(&cur) < deadline);
  return 0;
}

// run panels atop the display in an exploration of transparency
static int
slidepanel(struct notcurses* nc, struct ncplane* stdn){
  unsigned dimy, dimx;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  int ny = dimy / 4;
  int nx = dimx / 3;
  int yoff = rand() % (dimy - ny - 2) + 1; // don't start atop a border
  int xoff = rand() % (dimx - nx - 2) + 1;
  struct ncplane* l;

  // First we just create a plane with no styling and no glyphs.
  struct ncplane_options nopts = {
    .y = yoff,
    .x = xoff,
    .rows = ny,
    .cols = nx,
  };
  struct ncplane* n = ncplane_create(stdn, &nopts);

  // Zero-initialized channels use the default color, opaquely. Since we have
  // no glyph, we should show underlying glyphs in the default colors. The
  // background default might be transparent, at the window level (i.e. a copy
  // of the underlying desktop).
  nccell c = NCCELL_CHAR_INITIALIZER(' ');
  struct timespec cur;
  ncplane_set_base_cell(n, &c);
  clock_gettime(CLOCK_MONOTONIC, &cur);
  uint64_t deadlinens = timespec_to_ns(&cur) + timespec_to_ns(&demodelay);
  int velx = rand() % 4 + 1;
  int vely = rand() % 4 + 1;
  l = legend(nc, "default background, all opaque, whitespace glyph");
  int err = slideitslideit(nc, n, deadlinens, &vely, &velx);
  if(err){
    ncplane_destroy(n);
    ncplane_destroy(l);
    return err;
  }
  ncplane_destroy(l);

  nccell_load_char(n, &c, '\0');
  ncplane_set_base_cell(n, &c);
  clock_gettime(CLOCK_MONOTONIC, &cur);
  deadlinens = timespec_to_ns(&cur) + timespec_to_ns(&demodelay);
  l = legend(nc, "default background, all opaque, no glyph");
  if( (err = slideitslideit(nc, n, deadlinens, &vely, &velx)) ){
    ncplane_destroy(n);
    ncplane_destroy(l);
    return err;
  }
  ncplane_destroy(l);

  // Next, we set our foreground transparent, allowing characters underneath to
  // be seen in their natural colors. Our background remains opaque+default.
  nccell_set_fg_alpha(&c, NCALPHA_TRANSPARENT);
  ncplane_set_base_cell(n, &c);
  clock_gettime(CLOCK_MONOTONIC, &cur);
  deadlinens = timespec_to_ns(&cur) + timespec_to_ns(&demodelay);
  l = legend(nc, "default background, fg transparent, no glyph");
  if( (err = slideitslideit(nc, n, deadlinens, &vely, &velx)) ){
    ncplane_destroy(n);
    ncplane_destroy(l);
    return err;
  }
  ncplane_destroy(l);

  // Set the foreground color, setting it to blend. We should get the underlying
  // glyphs in a blended color, with the default background color.
  nccell_set_fg_rgb(&c, 0x80c080);
  nccell_set_fg_alpha(&c, NCALPHA_BLEND);
  ncplane_set_base_cell(n, &c);
  clock_gettime(CLOCK_MONOTONIC, &cur);
  l = legend(nc, "default background, fg blended, no glyph");
  deadlinens = timespec_to_ns(&cur) + timespec_to_ns(&demodelay);
  if( (err = slideitslideit(nc, n, deadlinens, &vely, &velx)) ){
    ncplane_destroy(n);
    ncplane_destroy(l);
    return err;
  }
  ncplane_destroy(l);

  // Opaque foreground color. This produces underlying glyphs in the specified,
  // fixed color, with the default background color.
  nccell_set_fg_rgb(&c, 0x80c080);
  nccell_set_fg_alpha(&c, NCALPHA_OPAQUE);
  ncplane_set_base_cell(n, &c);
  clock_gettime(CLOCK_MONOTONIC, &cur);
  l = legend(nc, "default background, fg colored opaque, no glyph");
  deadlinens = timespec_to_ns(&cur) + timespec_to_ns(&demodelay);
  if( (err = slideitslideit(nc, n, deadlinens, &vely, &velx)) ){
    ncplane_destroy(n);
    ncplane_destroy(l);
    return err;
  }
  ncplane_destroy(l);

  // Now we replace the characters with X's, colored as underneath us.
  // Our background color remains opaque default.
  nccell_load_char(n, &c, 'X');
  nccell_set_fg_default(&c);
  nccell_set_fg_alpha(&c, NCALPHA_TRANSPARENT);
  nccell_set_bg_alpha(&c, NCALPHA_OPAQUE);
  ncplane_set_base_cell(n, &c);
  clock_gettime(CLOCK_MONOTONIC, &cur);
  l = legend(nc, "default colors, fg transparent, print glyph");
  deadlinens = timespec_to_ns(&cur) + timespec_to_ns(&demodelay);
  if( (err = slideitslideit(nc, n, deadlinens, &vely, &velx)) ){
    ncplane_destroy(n);
    ncplane_destroy(l);
    return err;
  }
  ncplane_destroy(l);

  // Now we replace the characters with X's, but draw the foreground and
  // background color from below us.
  nccell_set_fg_alpha(&c, NCALPHA_TRANSPARENT);
  nccell_set_bg_alpha(&c, NCALPHA_TRANSPARENT);
  ncplane_set_base_cell(n, &c);
  clock_gettime(CLOCK_MONOTONIC, &cur);
  l = legend(nc, "all transparent, print glyph");
  deadlinens = timespec_to_ns(&cur) + timespec_to_ns(&demodelay);
  if( (err = slideitslideit(nc, n, deadlinens, &vely, &velx)) ){
    ncplane_destroy(n);
    ncplane_destroy(l);
    return err;
  }
  ncplane_destroy(l);

  // Finally, we populate the plane for the first time with non-transparent
  // characters. We blend, however, to show the underlying color in our glyphs.
  nccell_set_fg_alpha(&c, NCALPHA_BLEND);
  nccell_set_bg_alpha(&c, NCALPHA_BLEND);
  nccell_set_fg_rgb(&c, 0x80c080);
  nccell_set_bg_rgb(&c, 0x204080);
  ncplane_set_base_cell(n, &c);
  clock_gettime(CLOCK_MONOTONIC, &cur);
  l = legend(nc, "all blended, print glyph");
  deadlinens = timespec_to_ns(&cur) + timespec_to_ns(&demodelay);
  if( (err = slideitslideit(nc, n, deadlinens, &vely, &velx)) ){
    ncplane_destroy(n);
    ncplane_destroy(l);
    return err;
  }
  ncplane_destroy(l);

  char* logop = find_data("notcurses.png");
  struct ncvisual* ncv = ncvisual_from_file(logop);
  if(ncv == NULL){
    ncplane_destroy(n);
    return err;
  }
  free(logop);
  struct ncvisual_options vopts = {
    .n = n,
    .scaling = NCSCALE_STRETCH,
    .blitter = NCBLIT_PIXEL,
  };
  if(ncvisual_blit(nc, ncv, &vopts) == NULL){
    ncplane_destroy(n);
    return err;
  }
  ncvisual_destroy(ncv);
  clock_gettime(CLOCK_MONOTONIC, &cur);
  l = legend(nc, "partially-transparent image");
  deadlinens = timespec_to_ns(&cur) + 2 * timespec_to_ns(&demodelay);
  if( (err = slideitslideit(nc, n, deadlinens, &vely, &velx)) ){
    ncplane_destroy(n);
    ncplane_destroy(l);
    return err;
  }
  ncplane_destroy(l);

  return ncplane_destroy(n);
}

// draws a border along the perimeter, then fills the inside with position
// markers, each a slightly different color. the goal is to make sure we can
// have a great many colors, that they progress reasonably through the space,
// and that we can write to every coordinate.
int trans_demo(struct notcurses* nc, uint64_t startns){
  (void)startns;
  unsigned maxx, maxy;
  struct ncplane* n = notcurses_stddim_yx(nc, &maxy, &maxx);
  ncplane_set_fg_rgb8(n, 255, 255, 255);
  uint64_t channels = 0;
  ncchannels_set_fg_rgb8(&channels, 0, 128, 128);
  ncchannels_set_bg_rgb8(&channels, 90, 0, 90);
  unsigned y = 1, x = 0;
  ncplane_cursor_move_yx(n, y, x);
  if(ncplane_rounded_box_sized(n, 0, channels, maxy - 1, maxx, 0)){
    return -1;
  }
  uint32_t rgb = 0;
  while(++y < maxy - 1){
    x = 1;
    if(ncplane_cursor_move_yx(n, y, x)){
      return -1;
    }
    while(x < maxx - 1){
      ncplane_set_fg_rgb8(n, (rgb & 0xff0000) >> 16u, (rgb & 0xff00) >> 8u, rgb & 0xff);
      ncplane_set_bg_rgb8(n, 0, 10, 0);
      ncplane_putchar(n, x % 10 + '0');
      grow_rgb8(&rgb);
      ++x;
    }
  }
  if(notcurses_canfade(nc)){
    struct ncplane* l = legend(nc, "what say we explore transparency together?");
    DEMO_RENDER(nc);
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    int err;
    if( (err = ncplane_pulse(l, &demodelay, pulser, &now)) != 2){
      return err;
    }
    ncplane_destroy(l);
  }
  return slidepanel(nc, n);
}
