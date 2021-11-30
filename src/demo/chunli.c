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
  unsigned dimx, dimy;
  ncplane_dim_yx(notcurses_stdplane_const(nc), &dimy, &dimx);
  struct timespec iterdelay;
  timespec_div(&demodelay, 10, &iterdelay);
  for(int i = 0 ; i < count ; ++i){
    snprintf(file, sizeof(file), "chunli%d.%s", i + 1, ext);
    chuns[i].path = find_data(file);
    chuns[i].ncv = ncvisual_from_file(chuns[i].path);
    if(chuns[i].ncv == NULL){
      return -1;
    }
    struct ncvisual_options vopts = {
      .n = notcurses_stdplane(nc),
      .flags = NCVISUAL_OPTION_CHILDPLANE,
    };
    if((chuns[i].n = ncvisual_blit(nc, chuns[i].ncv, &vopts)) == NULL){
      return -1;
    }
    ncplane_set_base_cell(chuns[i].n, b);
    unsigned thisx, thisy;
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
int chunli_demo(struct notcurses* nc, uint64_t startns){
  (void)startns;
  if(!notcurses_canopen_images(nc)){
    return 0;
  }
  struct timespec iterdelay;
  timespec_div(&demodelay, 10, &iterdelay);
  int ret;
  unsigned dimx, dimy;
  ncplane_dim_yx(notcurses_stdplane_const(nc), &dimy, &dimx);
  nccell b = NCCELL_TRIVIAL_INITIALIZER;
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
    struct ncvisual_options vopts = {
      .x = NCALIGN_CENTER,
      .y = NCALIGN_CENTER,
      .n = notcurses_stdplane(nc),
      .flags = NCVISUAL_OPTION_CHILDPLANE |
               NCVISUAL_OPTION_HORALIGNED |
               NCVISUAL_OPTION_VERALIGNED,
    };
    if((ncp = ncvisual_blit(nc, ncv, &vopts)) == NULL){
      return -1;
    }
    ncplane_set_base_cell(ncp, &b);
    DEMO_RENDER(nc);
    demo_nanosleep(nc, &iterdelay);
    ncvisual_destroy(ncv);
    ncplane_destroy(ncp);
  }
  return 0;
}
