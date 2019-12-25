#include "demo.h"

#define CHUNS 8 // 8-part sprite
#define HEIGHT (95 / 2)

typedef struct chunli {
  char* path;
  struct ncvisual* ncv;
  struct ncplane* n;
} chunli;

// test of sprites from files
int chunli_demo(struct notcurses* nc){
  struct ncplane* stdn = notcurses_stdplane(nc);
  cell c = CELL_SIMPLE_INITIALIZER(' ');
  cell_set_bg(&c, 0x804080);
  ncplane_set_default(stdn, &c);
  chunli chuns[CHUNS];
  char file[PATH_MAX];
  int xoff = 0;
  for(int i = 0 ; i < CHUNS ; ++i){
    snprintf(file, sizeof(file), "chunli%d.bmp", i + 1);
    chuns[i].path = find_data(file);
    int averr;
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
    int thisx, thisy;
    ncplane_dim_yx(chuns[i].n, &thisy, &thisx);
    if(ncplane_move_yx(chuns[i].n, (HEIGHT - thisy) / 2, xoff)){
      return -1;
    }
    xoff += thisx;
fprintf(stderr, "WIDTH: %d AT: %d UP: %d\n", thisx, xoff, (HEIGHT - thisy) / 2);
  }
  if(notcurses_render(nc)){
    return -1;
  }
  nanosleep(&demodelay, NULL);
  for(int i = 0 ; i < CHUNS ; ++i){
    ncvisual_destroy(chuns[i].ncv);
    free(chuns[i].path);
  }
  return 0;
}
