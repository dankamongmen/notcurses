#include "demo.h"

// 0: transparent
// 1: white
// 2: grey
// 3: black
const char eagle1[] =
"0000003333333000"
"0000331113112300"
"0003111111113130"
"0031111113133330"
"3311311111333030"
"2131111111313000"
"3331111111311300"
"0311111111331330"
"0311311111131130"
"0323111111133130"
"0323111111133113"
"0033213111113313"
"0003213211111333"
"0000332321321233"
"0000003312332223"
"0000000033003330"
;

static struct ncplane*
zoom_map(struct notcurses* nc, const char* map, int* ret){
  *ret = -1;
  // determine size that will be represented on screen at once, and how
  // large that section has been rendered in the outzoomed map. take the map
  // and begin opening it on larger and larger planes that fit on the screen
  // less and less. eventually, reach our natural NCSCALE_NONE size and begin
  // scrolling through the map, whooooooooosh.
  struct ncvisual* ncv = ncvisual_from_file(map);
  if(ncv == NULL){
    return NULL;
  }
  int vheight, yscale;
  int vwidth, xscale;
  // first we want to get the true size, so don't supply NCSSCALE_STRETCH yet
  struct ncvisual_options vopts = {
    .y = 1,
    .blitter = NCBLIT_2x2,
  };
  if(ncvisual_geom(nc, ncv, &vopts, &vheight, &vwidth, &yscale, &xscale)){
    ncvisual_destroy(ncv);
    return NULL;
  }
//fprintf(stderr, "VHEIGHT: %d VWIDTH: %d scale: %d/%d\n", vheight, vwidth, yscale, xscale);
  // we start at the lower left corner of the outzoomed map
  int placey, placex; // dimensions of true display
  notcurses_term_dim_yx(nc, &placey, &placex);
  float delty = 3;
  float deltx = 3;
  float truey = placey;
  float truex = placex;
  if(truey > truex){
    delty *= truey / truex;
  }else{
    deltx *= truex / truey;
  }
  // to zoom in on the map, we're going to scale the full image to a plane
  // which grows on each iteration. it starts at the standard plane size,
  // and each time gets bigger (and is moved, so that the same area stays
  // on the screen, growing).
  struct ncplane* zncp = ncplane_dup(notcurses_stdplane(nc), NULL);
  if(zncp == NULL){
    ncvisual_destroy(ncv);
    return NULL;
  }
  vheight /= yscale;
  vwidth /= xscale;
  vopts.n = zncp;
  vopts.scaling = NCSCALE_STRETCH;
  if(ncvisual_render(nc, ncv, &vopts) == NULL || (*ret = demo_render(nc))){
    ncvisual_destroy(ncv);
    ncplane_destroy(zncp);
    return NULL;
  }
  while(vheight > truey || vwidth > truex){
    *ret = -1;
    ncplane_destroy(zncp);
    if((zncp = ncplane_new(nc, truey, truex, 0, 0, NULL)) == NULL){
      ncvisual_destroy(ncv);
      return NULL;
    }
    vopts.n = zncp;
    if((truey += delty) > vheight){
      truey = vheight;
    }
    if((truex += deltx) > vwidth){
      truex = vwidth;
    }
    if(ncvisual_render(nc, ncv, &vopts) == NULL){
      ncvisual_destroy(ncv);
      ncplane_destroy(zncp);
      return NULL;
    }
    if( (*ret = demo_render(nc)) ){
      ncvisual_destroy(ncv);
      ncplane_destroy(zncp);
      return NULL;
    }
  }
  ncvisual_destroy(ncv);
  return zncp;
}

static int
draw_eagle(struct ncplane* n, const char* sprite){
  cell bgc = CELL_TRIVIAL_INITIALIZER;
  cell_set_fg_alpha(&bgc, CELL_ALPHA_TRANSPARENT);
  cell_set_bg_alpha(&bgc, CELL_ALPHA_TRANSPARENT);
  ncplane_set_base_cell(n, &bgc);
  cell_release(n, &bgc);
  size_t s;
  int sbytes;
  for(s = 0 ; sprite[s] ; ++s){
    switch(sprite[s]){
      case '0':
        break;
      case '1':
        ncplane_set_fg_rgb(n, 0xff, 0xff, 0xff);
        break;
      case '2':
        ncplane_set_fg_rgb(n, 0xe3, 0x9d, 0x25);
        break;
      case '3':
        ncplane_set_fg_rgb(n, 0x3a, 0x84, 0x00);
        break;
    }
    if(sprite[s] != '0'){
      unsigned f, b;
      f = ncplane_fg(n);
      b = ncplane_bg(n);
      ncplane_set_fg(n, b);
      ncplane_set_bg(n, f);
      if(ncplane_putegc_yx(n, s / 16, s % 16, " ", &sbytes) != 1){
        return -1;
      }
      ncplane_set_fg(n, f);
      ncplane_set_bg(n, b);
    }
  }
  return 0;
}

static int
eagles(struct notcurses* nc){
  int ret = 0;
  int truex, truey; // dimensions of true display
  notcurses_term_dim_yx(nc, &truey, &truex);
  struct timespec flapiter;
  timespec_div(&demodelay, truex / 2, &flapiter);
  const int height = 16;
  const int width = 16;
  struct eagle {
    int yoff, xoff;
    struct ncplane* n;
  } e[3];
  for(size_t i = 0 ; i < sizeof(e) / sizeof(*e) ; ++i){
    e[i].xoff = 0;
    e[i].yoff = random() % ((truey - height) / 2);
    e[i].n = ncplane_new(nc, height, width, e[i].yoff, e[i].xoff, NULL);
    if(e[i].n == NULL){
      return -1;
    }
    if(draw_eagle(e[i].n, eagle1)){
      return -1;
    }
  }
  int eaglesmoved;
  do{
    eaglesmoved = 0;
    if( (ret = demo_render(nc)) ){
      break;
    }
    for(size_t i = 0 ; i < sizeof(e) / sizeof(*e) ; ++i){
      if(e[i].xoff >= truex){
        continue;
      }
      e[i].yoff += random() % (2 + i) - 1;
      if(e[i].yoff < 0){
        e[i].yoff = 0;
      }else if(e[i].yoff + height >= truey){
        e[i].yoff = truey - height - 1;
      }
      int scale = truex >= 80 ? truex / 80 : 1;
      e[i].xoff += (random() % scale) + 1;
      ncplane_move_yx(e[i].n, e[i].yoff, e[i].xoff);
      ++eaglesmoved;
    }
    demo_nanosleep(nc, &flapiter);
  }while(eaglesmoved);
  for(size_t i = 0 ; i < sizeof(e) / sizeof(*e) ; ++i){
    ncplane_destroy(e[i].n);
  }
  return ret;
}

// motherfucking eagles!
int eagle_demo(struct notcurses* nc){
  struct ncplane* zncp = NULL;
  int err;
  if(notcurses_canopen_images(nc)){
    char* map = find_data("eagles.png");
    if((zncp = zoom_map(nc, map, &err)) == NULL){
      free(map);
      return err;
    }
    free(map);
  }
  err = eagles(nc);
  ncplane_destroy(zncp);
  return err;
}
