#include "demo.h"

// show it with each blitter, with a legend
static int
visualize(struct notcurses* nc, struct ncvisual* ncv){
  for(ncblitter_e b = NCBLIT_DEFAULT + 1 ; b < NCBLIT_SIXEL ; ++b){
    struct timespec ts = { .tv_sec = 0, .tv_nsec = GIG / 2, };
    struct ncvisual_options vopts = {
      .scaling = NCSCALE_STRETCH,
      .blitter = b,
    };
    struct ncplane* n = ncvisual_render(nc, ncv, &vopts);
    if(n == NULL){
      return -1;
    }
    const char* name = notcurses_str_blitter(b);
    ncplane_set_bg_rgb(n, 0);
    int scalex, scaley, truey, truex;
    ncvisual_geom(nc, ncv, &vopts, &truey, &truex, &scaley, &scalex);
    ncplane_printf_aligned(n, ncplane_dim_y(n) / 2 - 1, NCALIGN_CENTER,
                           "%dx%d", truex, truey);
    ncplane_putstr_aligned(n, ncplane_dim_y(n) / 2, NCALIGN_CENTER, name);
    ncplane_printf_aligned(n, ncplane_dim_y(n) / 2 + 1, NCALIGN_CENTER,
                           "%d:%d pixels -> cell", scalex, scaley);
    int ret = demo_render(nc);
    demo_nanosleep(nc, &ts);
    ncplane_destroy(n);
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
  const char* files[] = { "covid19.jpg", "warmech.bmp", NULL, };
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
