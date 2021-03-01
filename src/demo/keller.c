#include "demo.h"

// show it with each blitter, with a legend
static int
visualize(struct notcurses* nc, struct ncvisual* ncv){
  ncblitter_e bs[] = {
    NCBLIT_1x1,
    NCBLIT_2x1,
    NCBLIT_2x2,
    NCBLIT_3x2,
    NCBLIT_BRAILLE,
    NCBLIT_PIXEL,
  };
  for(size_t i = 0 ; i < sizeof(bs) / sizeof(*bs) ; ++i){
    if(bs[i] == NCBLIT_PIXEL && !notcurses_canpixel(nc)){
      // FIXME throw up an indicator?
      continue;
    }
    struct ncvisual_options vopts = {
      .scaling = NCSCALE_STRETCH,
      .blitter = bs[i],
      .n = notcurses_stdplane(nc),
      .y = 1,
    };
    if(ncvisual_render(nc, ncv, &vopts) == NULL){
      return -1;
    }
    ncplane_set_fg_rgb(vopts.n, 0xffffff);
    ncplane_set_bg_rgb(vopts.n, 0);
    const char* name = notcurses_str_blitter(bs[i]);
    int scalex, scaley, truey, truex;
    ncvisual_geom(nc, ncv, &vopts, &truey, &truex, &scaley, &scalex);
    ncplane_putstr_aligned(vopts.n, ncplane_dim_y(vopts.n) / 2 - 3, NCALIGN_CENTER, name);
    ncplane_printf_aligned(vopts.n, ncplane_dim_y(vopts.n) / 2 - 1, NCALIGN_CENTER,
                           "%03dx%03d", truex, truey);
    ncplane_printf_aligned(vopts.n, ncplane_dim_y(vopts.n) / 2 + 1, NCALIGN_CENTER,
                           "%d:%d pixels -> cell", scalex, scaley);
    int ret = demo_render(nc);
    if(ret){
      return ret;
    }
    ret = demo_nanosleep(nc, &demodelay);
    if(ret){
      return ret;
    }
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
