#include <unistd.h>
#include <notcurses/notcurses.h>

// draw a bitmap of 6x6 cells with a cell left out
static int
wipebitmap(struct notcurses* nc){
  int cellpxy, cellpxx;
  ncplane_pixelgeom(notcurses_stdplane(nc), NULL, NULL,
                    &cellpxy, &cellpxx, NULL, NULL);
  int pixy = cellpxy * 6;
  int pixx = cellpxx * 6;
  uint32_t pixels[pixx * pixy];
  memset(pixels, 0xff, sizeof(pixels));
  struct ncvisual* ncv = ncvisual_from_rgba(pixels,
                                            6 * cellpxy,
                                            6 * cellpxx * 4,
                                            6 * cellpxx);
  if(ncv == NULL){
    return -1;
  }
  struct ncvisual_options vopts = {
    .blitter = NCBLIT_PIXEL,
  };
  struct ncplane* n = ncvisual_render(nc, ncv, &vopts);
  if(n == NULL){
    return -1;
  }
  ncvisual_destroy(ncv);
  ncplane_putstr_yx(notcurses_stdplane(nc), 6, 0, "Ought see full square");
  notcurses_render(nc);
  sleep(3);

  ncplane_erase(notcurses_stdplane(nc));
  for(int y = 1 ; y < 5 ; ++y){
    for(int x = 1 ; x < 5 ; ++x){
      ncplane_putchar_yx(notcurses_stdplane(nc), y, x, '*');
    }
  }
  uint64_t channels = 0;
  ncchannels_set_bg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
  ncchannels_set_fg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
  ncplane_set_base(notcurses_stdplane(nc), "", 0, channels);
  ncplane_move_top(notcurses_stdplane(nc));
  ncplane_putstr_yx(notcurses_stdplane(nc), 6, 0, "Ought see 16 *s");
  notcurses_render(nc);
  sleep(3);

  ncplane_erase(notcurses_stdplane(nc));
  ncplane_putstr_yx(notcurses_stdplane(nc), 6, 0, "Ought see full square");
  notcurses_render(nc);
  sleep(3);

  ncplane_erase(notcurses_stdplane(nc));
  for(int y = 1 ; y < 5 ; ++y){
    for(int x = 1 ; x < 5 ; ++x){
      ncplane_putchar_yx(notcurses_stdplane(nc), y, x, ' ');
    }
  }
  ncplane_putstr_yx(notcurses_stdplane(nc), 6, 0, "Ought see 16 spaces");
  notcurses_render(nc);
  sleep(3);

  ncplane_erase(notcurses_stdplane(nc));
  ncplane_destroy(n);
  ncplane_putstr_yx(notcurses_stdplane(nc), 6, 0, "Ought see nothing");
  notcurses_render(nc);
  sleep(3);

  ncplane_erase(notcurses_stdplane(nc));
  for(int i = cellpxy ; i < 5 * cellpxy ; ++i){
    memset(pixels + (i * 6 * cellpxx + cellpxx), 0, cellpxx * 4 * sizeof(*pixels));
  }
  ncv = ncvisual_from_rgba(pixels, 6 * cellpxy, 6 * cellpxx * 4, 6 * cellpxx);
  if(ncv == NULL){
    return -1;
  }
  if((n = ncvisual_render(nc, ncv, &vopts)) == NULL){
    return -1;
  }
  ncplane_putstr_yx(notcurses_stdplane(nc), 6, 0, "Ought see empty square");
  ncvisual_destroy(ncv);
  notcurses_render(nc);
  ncplane_destroy(n);
  sleep(3);
  return 0;
}

int main(void){
  struct notcurses_options opts = {
  };
  struct notcurses* nc = notcurses_core_init(&opts, NULL);
  if(notcurses_check_pixel_support(nc) < 1){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  int r = wipebitmap(nc);
  r |= notcurses_stop(nc);
  return r;
}
