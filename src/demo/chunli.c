#include "demo.h"

#define CHUNS 8 // 8-part sprite

typedef struct chunli {
  char* path;
  struct ncvisual* ncv;
  struct ncplane* n;
} chunli;

static int
chunli_draw(struct notcurses* nc, const char* ext, int count, const nccell* b){
  chunli chuns[CHUNS];
  char file[20];
  int dimx, dimy;
  struct timespec iterdelay;
  timespec_div(&demodelay, 10, &iterdelay);
  for(int i = 0 ; i < count ; ++i){
    notcurses_refresh(nc, &dimy, &dimx);
    snprintf(file, sizeof(file), "chunli%d.%s", i + 1, ext);
    chuns[i].path = find_data(file);
    chuns[i].ncv = ncvisual_from_file(chuns[i].path);
    if(chuns[i].ncv == NULL){
      return -1;
    }
    if((chuns[i].n = ncvisual_render(nc, chuns[i].ncv, NULL)) == NULL){
      return -1;
    }
    ncplane_set_base_cell(chuns[i].n, b);
    int thisx, thisy;
    ncplane_dim_yx(chuns[i].n, &thisy, &thisx);
    if(ncplane_move_yx(chuns[i].n, (dimy - thisy) / 2, (dimx - thisx) / 2)){
      return -1;
    }
    // xoff += thisx;
    DEMO_RENDER(nc);
    demo_nanosleep(nc, &iterdelay);
    ncvisual_destroy(chuns[i].ncv);
    ncplane_destroy(chuns[i].n);
    free(chuns[i].path);
  }
  return 0;
}

// test of sprites from files
int chunli_demo(struct notcurses* nc){
  if(!notcurses_canopen_images(nc)){
    return 0;
  }
  struct timespec iterdelay;
  timespec_div(&demodelay, 10, &iterdelay);
  int ret, dimx, dimy;
  notcurses_refresh(nc, &dimy, &dimx);
  nccell b = CELL_TRIVIAL_INITIALIZER;
  nccell_set_fg_alpha(&b, NCALPHA_TRANSPARENT);
  nccell_set_bg_alpha(&b, NCALPHA_TRANSPARENT);
  if( (ret = chunli_draw(nc, "bmp", CHUNS, &b)) ){
    return ret;
  }
  char file[20];
  for(int i = 1 ; i < 100 ; ++i){
    snprintf(file, sizeof(file), "chunli%02d.png", i);
    char* path = find_data(file);
    struct ncvisual* ncv = ncvisual_from_file(path);
    if(ncv == NULL){
      free(path);
      break;
    }
    free(path);
    struct ncplane* ncp;
    if((ncp = ncvisual_render(nc, ncv, NULL)) == NULL){
      return -1;
    }
    ncplane_set_base_cell(ncp, &b);
    int thisx, thisy;
    ncplane_dim_yx(ncp, &thisy, &thisx);
    if(ncplane_move_yx(ncp, (dimy - thisy) / 2, (dimx - thisx) / 2)){
      return -1;
    }
    DEMO_RENDER(nc);
    demo_nanosleep(nc, &iterdelay);
    ncvisual_destroy(ncv);
    ncplane_destroy(ncp);
  }
  return 0;
}
