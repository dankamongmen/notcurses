#include "demo.h"

// show it with each blitter, with a legend
static int
visualize(struct notcurses* nc, struct ncvisual* ncv){
  struct timespec kdelay;
  timespec_div(&demodelay, 2, &kdelay);
  ncblitter_e bs[] = {
    NCBLIT_BRAILLE,
    NCBLIT_1x1,
    NCBLIT_2x1,
    NCBLIT_2x2,
    NCBLIT_3x2,
    NCBLIT_PIXEL,
  };
  struct ncplane* stdn = notcurses_stdplane(nc);
  ncplane_set_fg_rgb(stdn, 0xffffff);
  ncplane_set_bg_rgb(stdn, 0);
  uint64_t channels = 0;
  channels_set_fg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
  channels_set_bg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
  ncplane_set_base(stdn, "", 0, channels);
  for(size_t i = 0 ; i < sizeof(bs) / sizeof(*bs) ; ++i){
    struct ncvisual_options vopts = {
      .scaling = NCSCALE_SCALE,
      .blitter = bs[i],
      .y = NCALIGN_CENTER,
      .flags = NCVISUAL_OPTION_NODEGRADE | NCVISUAL_OPTION_HORALIGNED
                | NCVISUAL_OPTION_VERALIGNED,
    };
    int scalex, scaley, truey, truex;
    ncvisual_geom(nc, ncv, &vopts, &truey, &truex, &scaley, &scalex);
    vopts.x = NCALIGN_CENTER;
    vopts.y = (ncplane_dim_y(notcurses_stdplane(nc)) - truey / scaley) / 2;
//fprintf(stderr, "X: %d truex: %d sclaex: %d\n", vopts.x, truex, scalex);
    ncplane_erase(stdn);
    // if we're about to blit pixel graphics, render the screen as empty, so
    // that everything is damaged for the printing of the legend.
    if(vopts.blitter == NCBLIT_PIXEL){
      DEMO_RENDER(nc);
    }
    struct ncplane* n;
    if((n = ncvisual_render(nc, ncv, &vopts)) == NULL){
      ncplane_printf_aligned(stdn, ncplane_dim_y(stdn) / 2 - 1, NCALIGN_CENTER, "not available");
    }else{
      // FIXME shouldn't need this once z-axis is united with bitmap graphics
      ncplane_move_below(n, stdn);
      if(vopts.blitter == NCBLIT_PIXEL){
        DEMO_RENDER(nc);
      }
      ncplane_printf_aligned(stdn, ncplane_dim_y(stdn) / 2 - 1, NCALIGN_CENTER,
                            "%03dx%03d", truex, truey);
      ncplane_printf_aligned(stdn, ncplane_dim_y(stdn) / 2 + 1, NCALIGN_CENTER,
                            "%d:%d pixels -> cell", scalex, scaley);
    }
    const char* name = notcurses_str_blitter(bs[i]);
    ncplane_printf_aligned(stdn, ncplane_dim_y(stdn) / 2 - 3, NCALIGN_CENTER, "%sblitter", name);
    int ret = demo_render(nc);
    if(ret){
      return ret;
    }
    ret = demo_nanosleep(nc, &kdelay);
    if(ret){
      return ret;
    }
    ncplane_destroy(n);
  }
  return 0;
}

int keller_demo(struct notcurses* nc){
  if(!notcurses_canopen_images(nc)){
    return 0;
  }
  // don't leave whatever random stuff was up behind covid19 (braille is always
  // to a degree transparent), but *do* leave things up between phases--the
  // combined braille of covid19+atma looks subtly cool.
  ncplane_erase(notcurses_stdplane(nc));
  const char* files[] = { "covid19.jpg", "atma.png", "fonts.jpg", "aidsrobots.jpeg", NULL, };
  for(const char** file = files ; *file ; ++file){
    char* f = find_data(*file);
    if(f == NULL){
      return -1;
    }
    struct ncvisual* ncv = ncvisual_from_file(f);
    free(f);
    if(ncv == NULL){
      return -1;
    }
    int r = visualize(nc, ncv);
    ncvisual_destroy(ncv);
    if(r){
      return r;
    }
  }
  return 0;
}
