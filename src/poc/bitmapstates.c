#include <unistd.h>
#include <notcurses/notcurses.h>

static void
emit(struct ncplane* n, const char* str){
  fprintf(stderr, "\n\n\n%s\n", str);
  ncplane_erase_region(n, 6, 0, INT_MAX, 0);
  ncplane_putstr_yx(n, 6, 0, str);
}

// draw a bitmap of 6x6 cells with a cell left out
static int
wipebitmap(struct notcurses* nc){
  unsigned cellpxy, cellpxx;
  ncplane_pixel_geom(notcurses_stdplane(nc), NULL, NULL,
                     &cellpxy, &cellpxx, NULL, NULL);
  int pixy = cellpxy * 6;
  int pixx = cellpxx * 6;
  uint32_t* pixels = malloc(sizeof(*pixels) * pixx * pixy);
  memset(pixels, 0xff, sizeof(*pixels) * pixx * pixy);
  struct ncvisual* ncv = ncvisual_from_rgba(pixels,
                                            6 * cellpxy,
                                            6 * cellpxx * 4,
                                            6 * cellpxx);
  if(ncv == NULL){
    free(pixels);
    return -1;
  }
  struct ncvisual_options vopts = {
    .blitter = NCBLIT_PIXEL,
    .n = notcurses_stdplane(nc),
    .flags = NCVISUAL_OPTION_CHILDPLANE,
  };
  struct ncplane* n = ncvisual_blit(nc, ncv, &vopts);
  if(n == NULL){
    free(pixels);
    return -1;
  }
  emit(notcurses_stdplane(nc), "Ought see full square");
  notcurses_debug(nc, stderr);
  notcurses_render(nc);
  sleep(2);

  ncplane_erase(notcurses_stdplane(nc));
  for(int y = 1 ; y < 5 ; ++y){
    for(int x = 1 ; x < 5 ; ++x){
      ncplane_putchar_yx(notcurses_stdplane(nc), y, x, '*');
    }
  }
  uint64_t channels = 0;
  ncchannels_set_bg_alpha(&channels, NCALPHA_TRANSPARENT);
  ncchannels_set_fg_alpha(&channels, NCALPHA_TRANSPARENT);
  ncplane_set_base(notcurses_stdplane(nc), "", 0, channels);
  ncplane_move_top(notcurses_stdplane(nc));
  emit(notcurses_stdplane(nc), "Ought see 16 *s");
  notcurses_render(nc);
  sleep(2);

  ncplane_erase(notcurses_stdplane(nc));
  emit(notcurses_stdplane(nc), "Ought see full square");
  notcurses_debug(nc, stderr);
  notcurses_render(nc);
  sleep(2);

  ncplane_erase(notcurses_stdplane(nc));
  for(int y = 1 ; y < 5 ; ++y){
    for(int x = 1 ; x < 5 ; ++x){
      ncplane_putchar_yx(notcurses_stdplane(nc), y, x, ' ');
    }
  }
  emit(notcurses_stdplane(nc), "Ought see 16 spaces");
  notcurses_debug(nc, stderr);
  notcurses_render(nc);
  sleep(2);

  ncplane_erase(notcurses_stdplane(nc));
  ncplane_destroy(n);
  emit(notcurses_stdplane(nc), "Ought see nothing");
  notcurses_debug(nc, stderr);
  notcurses_render(nc);
  sleep(2);

  ncplane_erase(notcurses_stdplane(nc));
  for(unsigned i = cellpxy ; i < 5 * cellpxy ; ++i){
    memset(pixels + (i * 6 * cellpxx + cellpxx), 0, cellpxx * 4 * sizeof(*pixels));
  }
  struct ncvisual* ncve = ncvisual_from_rgba(pixels, 6 * cellpxy, 6 * cellpxx * 4, 6 * cellpxx);
  if(ncve == NULL){
    free(pixels);
    return -1;
  }
  if((n = ncvisual_blit(nc, ncve, &vopts)) == NULL){
    free(pixels);
    ncvisual_destroy(ncve);
    return -1;
  }
  emit(notcurses_stdplane(nc), "Ought see empty square");
  notcurses_debug(nc, stderr);
  notcurses_render(nc);
  sleep(2);
  vopts.n = n;
  ncplane_move_top(notcurses_stdplane(nc));

  // now, actually wipe the middle with *s, and then ensure that a new render
  // gets wiped out before being displayed
  if(ncvisual_blit(nc, ncv, &vopts) == NULL){
    return -1;
  }
  emit(notcurses_stdplane(nc), "Ought see full square");
  notcurses_debug(nc, stderr);
  notcurses_render(nc);
  sleep(2);

  for(int y = 1 ; y < 5 ; ++y){
    for(int x = 1 ; x < 5 ; ++x){
      ncplane_putchar_yx(notcurses_stdplane(nc), y, x, '*');
    }
  }
  emit(notcurses_stdplane(nc), "Ought see 16 *s");
  notcurses_debug(nc, stderr);
  notcurses_render(nc);
  sleep(2);

  if(ncvisual_blit(nc, ncv, &vopts) == NULL){
    ncvisual_destroy(ncve);
    free(pixels);
    return -1;
  }
  emit(notcurses_stdplane(nc), "Ought *still* see 16 *s");
  notcurses_debug(nc, stderr);
  notcurses_render(nc);
  sleep(2);

  ncplane_move_yx(n, 0, 7);
  emit(notcurses_stdplane(nc), "Full square on right");
  notcurses_debug(nc, stderr);
  notcurses_render(nc);
  sleep(2);

  ncplane_move_yx(n, 0, 0);
  emit(notcurses_stdplane(nc), "Ought see 16 *s");
  notcurses_debug(nc, stderr);
  notcurses_render(nc);
  sleep(2);

  ncplane_move_yx(n, 0, 7);
  emit(notcurses_stdplane(nc), "Full square on right");
  notcurses_debug(nc, stderr);
  notcurses_render(nc);
  sleep(2);

  ncvisual_destroy(ncve);
  ncvisual_destroy(ncv);
  ncplane_destroy(n);
  free(pixels);
  return 0;
}

int main(void){
  struct notcurses_options opts = {
    .loglevel = NCLOGLEVEL_TRACE,
    .flags = NCOPTION_DRAIN_INPUT,
  };
  struct notcurses* nc = notcurses_core_init(&opts, NULL);
  if(notcurses_check_pixel_support(nc) < 1){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  fprintf(stderr, "               stderr ought be redirected\n");
  int r = wipebitmap(nc);
  r |= notcurses_stop(nc);
  return r;
}
