#include "demo.h"

static bool truefxn(const struct notcurses* nc){ (void)nc; return true; }

// show it with each blitter, with a legend
static int
visualize(struct notcurses* nc, struct ncvisual* ncv){
  struct timespec kdelay;
  timespec_div(&demodelay, 2, &kdelay);
  const struct {
    ncblitter_e b;
    bool (*f)(const struct notcurses*);
  } bs[] = {
    { NCBLIT_BRAILLE, notcurses_canbraille, },
    { NCBLIT_1x1, truefxn, }, // everyone can do spaces
    { NCBLIT_2x1, notcurses_canhalfblock, },
    { NCBLIT_2x2, notcurses_canquadrant, },
    { NCBLIT_3x2, notcurses_cansextant, },
    { NCBLIT_PIXEL, notcurses_canpixel, },
  };
  struct ncplane* stdn = notcurses_stdplane(nc);
  ncplane_set_fg_rgb(stdn, 0xffffff);
  ncplane_set_bg_rgb(stdn, 0);
  ncplane_set_styles(stdn, NCSTYLE_BOLD);
  uint64_t channels = 0;
  ncchannels_set_fg_alpha(&channels, NCALPHA_TRANSPARENT);
  ncchannels_set_bg_alpha(&channels, NCALPHA_TRANSPARENT);
  ncplane_set_base(stdn, "", 0, channels);
  for(size_t i = 0 ; i < sizeof(bs) / sizeof(*bs) ; ++i){
    struct ncvisual_options vopts = {
      .n = notcurses_stdplane(nc),
      .scaling = NCSCALE_SCALE_HIRES,
      .blitter = bs[i].b,
      .flags = NCVISUAL_OPTION_NODEGRADE
                | NCVISUAL_OPTION_HORALIGNED
                | NCVISUAL_OPTION_VERALIGNED
                | NCVISUAL_OPTION_CHILDPLANE,
    };
    vopts.x = NCALIGN_CENTER;
    vopts.y = NCALIGN_CENTER;
    ncplane_erase(stdn); // to clear out old text
    struct ncplane* n = NULL;
    ncvgeom geom;
    if(ncvisual_geom(nc, ncv, &vopts, &geom) == 0){
      if( (n = ncvisual_blit(nc, ncv, &vopts)) ){
        ncplane_move_below(n, stdn);
        ncplane_printf_aligned(stdn, ncplane_dim_y(stdn) / 2 - 1, NCALIGN_CENTER,
                              "%03dx%03d", geom.pixx, geom.pixy);
        ncplane_printf_aligned(stdn, ncplane_dim_y(stdn) / 2 + 1, NCALIGN_CENTER,
                              "%d:%d pixels -> cell", geom.scalex, geom.scaley);
      }
    }
//fprintf(stderr, "X: %d truex: %d scalex: %d\n", vopts.x, truex, scalex);
    if(!n){
      if(bs[i].f(nc)){
        return -1;
      }
      ncplane_on_styles(stdn, NCSTYLE_ITALIC);
      ncplane_printf_aligned(stdn, ncplane_dim_y(stdn) / 2 - 1, NCALIGN_CENTER, "not available");
      ncplane_off_styles(stdn, NCSTYLE_ITALIC);
    }
    const char* name = notcurses_str_blitter(bs[i].b);
    ncplane_printf_aligned(stdn, ncplane_dim_y(stdn) / 2 - 3, NCALIGN_CENTER, "%sblitter", name);
    int ret = demo_render(nc);
    if(ret){
      ncplane_destroy(n);
      return ret;
    }
    ret = demo_nanosleep(nc, &kdelay);
    if(ret){
      ncplane_destroy(n);
      return ret;
    }
    ncplane_destroy(n);
  }
  return 0;
}

int keller_demo(struct notcurses* nc, uint64_t startns){
  (void)startns;
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
