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

// display the level map scaled to fit entirely within the visual area
static int
outzoomed_map(struct notcurses* nc, const char* map){
  nc_err_e ncerr;
  struct ncvisual* ncv = ncvisual_from_file(map, &ncerr);
  if(ncv == NULL){
    return -1;
  }
  if((ncerr = ncvisual_decode(ncv)) != NCERR_SUCCESS){
    return -1;
  }
  struct ncvisual_options opts = {
    .scaling = NCSCALE_SCALE,
    .n = notcurses_stdplane(nc),
    .y = 1,
  };
  if(ncvisual_render(nc, ncv, &opts) == NULL){
    return -1;
  }
  DEMO_RENDER(nc);
  ncvisual_destroy(ncv);
  return 0;
}

static struct ncplane*
zoom_map(struct notcurses* nc, const char* map){
  nc_err_e ncerr;
  // determine size that will be represented on screen at once, and how
  // large that section has been rendered in the outzoomed map. take the map
  // and begin opening it on larger and larger planes that fit on the screen
  // less and less. eventually, reach our natural NCSCALE_NONE size and begin
  // scrolling through the map, whooooooooosh.
  struct ncvisual* ncv = ncvisual_from_file(map, &ncerr);
  if(ncv == NULL){
    return NULL;
  }
  int vheight, yscale;
  int vwidth, xscale;
  if((ncerr = ncvisual_decode(ncv)) != NCERR_SUCCESS ||
     ncvisual_geom(nc, ncv, NCBLIT_DEFAULT, &vheight, &vwidth, &yscale, &xscale)){
    ncvisual_destroy(ncv);
    return NULL;
  }
//fprintf(stderr, "VHEIGHT: %d VWIDTH: %d scale: %d/%d\n", vheight, vwidth, yscale, xscale);
  ncvisual_destroy(ncv);
  // we start at the lower left corner of the outzoomed map
  int truex, truey; // dimensions of true display
  notcurses_term_dim_yx(nc, &truey, &truex);
  int zoomy = vheight;
  int zoomx = vwidth;
  struct ncplane* zncp = notcurses_stdplane(nc);
  int delty = yscale;
  int deltx = xscale;
  if(truey > truex){
    ++delty;
  }else if(truex > truey * 2){
    ++deltx;
  }
  truey *= yscale;
  truex *= xscale;
  while(zoomy > truey && zoomx > truex){
    if((zoomy -= delty) < truey){
      zoomy = truey;
    }
    if((zoomx -= deltx) < truex){
      zoomx = truex;
    }
    struct ncvisual* zncv = ncvisual_from_file(map, &ncerr);
    if(zncv == NULL){
      ncvisual_destroy(ncv);
      return NULL;
    }
    if((ncerr = ncvisual_decode(zncv)) != NCERR_SUCCESS){
      ncvisual_destroy(zncv);
      return NULL;
    }
//fprintf(stderr, "render! begy: %d lenx: %d\n", vheight - zoomy, zoomx);
    struct ncvisual_options vopts = {
      .lenx = zoomx,
      .begx = 0,
      .begy = vheight - zoomy,
      .y = 1,
      .n = zncp,
    };
    if(ncvisual_render(nc, zncv, &vopts) == NULL){
      ncvisual_destroy(zncv);
      return NULL;
    }
    if(demo_render(nc)){
      ncvisual_destroy(zncv);
      return NULL;
    }
    ncvisual_destroy(zncv);
  }
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
    demo_render(nc);
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
  return 0;
}

// motherfucking eagles!
int eagle_demo(struct notcurses* nc){
  if(!notcurses_canopen_images(nc)){
    return 0;
  }
  char* map = find_data("eagles.png");
  int err;
  if( (err = outzoomed_map(nc, map)) ){
    free(map);
    return err;
  }
  // FIXME propagate out err vs abort
  if(zoom_map(nc, map) == NULL){
    free(map);
    return -1;
  }
  if(eagles(nc)){
    return -1;
  }
  free(map);
  return 0;
}
