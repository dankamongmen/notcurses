#include "demo.h"

#define CHUNS 8 // 8-part sprite

typedef struct chunli {
  char* path;
  struct ncvisual* ncv;
  struct ncplane* n;
} chunli;

static int
chunli_draw(struct notcurses* nc, const char* ext, int count, const cell* b){
  chunli chuns[CHUNS];
  char file[PATH_MAX];
  int dimx, dimy;
  struct timespec iterdelay;
  timespec_div(&demodelay, 10, &iterdelay);
  for(int i = 0 ; i < count ; ++i){
    int averr;
    notcurses_resize(nc, &dimy, &dimx);
    snprintf(file, sizeof(file), "chunli%d.%s", i + 1, ext);
    chuns[i].path = find_data(file);
    chuns[i].ncv = ncvisual_open_plane(nc, chuns[i].path, &averr, 0, 0, NCSCALE_NONE);
    if(chuns[i].ncv == NULL){
      return -1;
    }
    if(ncvisual_decode(chuns[i].ncv, &averr) == NULL){
      return -1;
    }
    if(ncvisual_render(chuns[i].ncv, 0, 0, 0, 0)){
      return -1;
    }
    chuns[i].n = ncvisual_plane(chuns[i].ncv);
    ncplane_set_base(chuns[i].n, b);
    int thisx, thisy;
    ncplane_dim_yx(chuns[i].n, &thisy, &thisx);
    if(ncplane_move_yx(chuns[i].n, (dimy - thisy) / 2, (dimx - thisx) / 2)){
      return -1;
    }
    // xoff += thisx;
    if(demo_render(nc)){
      return -1;
    }
    nanosleep(&iterdelay, NULL);
    ncvisual_destroy(chuns[i].ncv);
    free(chuns[i].path);
  }
  return 0;
}

// test of sprites from files
int chunli_demo(struct notcurses* nc){
  struct timespec iterdelay;
  timespec_div(&demodelay, 10, &iterdelay);
  int averr, dimy, dimx;
  cell b = CELL_TRIVIAL_INITIALIZER;
  cell_set_fg_alpha(&b, CELL_ALPHA_TRANSPARENT);
  cell_set_bg_alpha(&b, CELL_ALPHA_TRANSPARENT);
  chunli_draw(nc, "bmp", CHUNS, &b);
  chunli_draw(nc, "png", 7, &b);
  char* victory = find_data("chunlivictory.png");
  struct ncvisual* ncv = ncvisual_open_plane(nc, victory, &averr, 0, 0, NCSCALE_NONE);
  if(ncv == NULL){
    return -1;
  }
  if(ncvisual_decode(ncv, &averr) == NULL){
    return -1;
  }
  struct ncplane* ncp = ncvisual_plane(ncv);
  ncplane_set_base(ncp, &b);
  const int offsets[] = {
    0, 50, 100, 154, 208, 260, 312, 368, 420, 479, 538, 588, 638, 688, 736, 786, 836, 888, 942
  };
  for(size_t i = 0u ; i < sizeof(offsets) / sizeof(*offsets) - 1 ; ++i){
    notcurses_resize(nc, &dimy, &dimx);
    ncplane_erase(ncvisual_plane(ncv));
    if(ncvisual_render(ncv, 0, offsets[i], 0, offsets[i + 1] - offsets[i] + 1)){
      return -1;
    }
    if(demo_render(nc)){
      return -1;
    }
    nanosleep(&iterdelay, NULL);
  }
  ncvisual_destroy(ncv);
  return 0;
}
