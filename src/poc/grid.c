#include <notcurses/notcurses.h>

struct ncvisual*
draw_grid(struct ncplane* stdn){
  unsigned maxby, maxbx;
  unsigned cellpxy, cellpxx;
  ncplane_pixel_geom(stdn, NULL, NULL, &cellpxy, &cellpxx, &maxby, &maxbx);
  if(cellpxy <= 1 || cellpxx <= 1){
    fprintf(stderr, "cell-pixel geometry: %d %d\n", cellpxy, cellpxx);
    return NULL;
  }
  uint32_t* rgba = malloc(maxby * maxbx * sizeof(*rgba));
  if(rgba == NULL){
    return NULL;
  }
  // we want an inlay on each cell, so if they're 10 wide, we want pixels
  // at 0, 9, 10, 19, 20, 29, etc.
  for(unsigned y = 0 ; y < maxby ; ++y){
    uint32_t* row = rgba + y * maxbx;
    for(unsigned x = 0 ; x < maxbx ; ++x){
      uint32_t* px = row + x;
      ncpixel_set_a(px, 255);
      ncpixel_set_r(px, 0x39);
      ncpixel_set_g(px, 0xff);
      ncpixel_set_b(px, 0xa0);
    }
    for(unsigned yi = 0 ; yi < cellpxy - 2 ; ++yi){
      ++y;
      if(y < maxby){
        row = rgba + y * maxbx;
        for(unsigned x = 0 ; x < maxbx ; ++x){
          uint32_t* px = row + x;
          ncpixel_set_a(px, 255);
          ncpixel_set_r(px, 0x39);
          ncpixel_set_g(px, 0xff);
          ncpixel_set_b(px, 0xa0);
          if((x += cellpxx - 1) < maxbx){
            px = row + x;
            ncpixel_set_a(px, 255);
            ncpixel_set_r(px, 0x39);
            ncpixel_set_g(px, 0xff);
            ncpixel_set_b(px, 0xa0);
          }
        }
      }
    }
    ++y;
    row = rgba + y * maxbx;
    if(y < maxby){
      for(unsigned x = 0 ; x < maxbx ; ++x){
        uint32_t* px = row + x;
        ncpixel_set_a(px, 255);
        ncpixel_set_r(px, 0x39);
        ncpixel_set_g(px, 0xff);
        ncpixel_set_b(px, 0xa0);
      }
    }
  }
  struct ncvisual* ncv = ncvisual_from_rgba(rgba, maxby, maxbx * sizeof(*rgba), maxbx);
  free(rgba);
  return ncv;
}

int main(void){
  struct notcurses_options opts = {
    .flags = NCOPTION_CLI_MODE |
             NCOPTION_SUPPRESS_BANNERS |
             NCOPTION_DRAIN_INPUT,
  //  .loglevel = NCLOGLEVEL_TRACE,
  };
  struct notcurses* nc = notcurses_init(&opts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  unsigned dimy, dimx;
  struct ncplane* stdn = notcurses_stddim_yx(nc, &dimy, &dimx);
  struct ncvisual* ncv = draw_grid(stdn);
  if(ncv == NULL){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  struct ncvisual_options vopts = {
    .n = stdn,
    .blitter = NCBLIT_PIXEL,
    .flags = NCVISUAL_OPTION_NODEGRADE | NCVISUAL_OPTION_CHILDPLANE,
  };
  struct ncplane* bitn = ncvisual_blit(nc, ncv, &vopts);
  ncvisual_destroy(ncv);
  if(bitn == NULL){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  // FIXME render it behind some text
  notcurses_render(nc);
  notcurses_stop(nc);
  return EXIT_SUCCESS;
}
