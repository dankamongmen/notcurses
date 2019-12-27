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
static struct ncvisual*
outzoomed_map(struct notcurses* nc, const char* map){
  int averr;
  struct ncvisual* ncv = ncvisual_open_plane(nc, map, &averr, 0, 0, NCSCALE_SCALE);
  if(ncv == NULL){
    return NULL;
  }
  if(ncvisual_decode(ncv, &averr) == NULL){
    return NULL;
  }
  if(ncvisual_render(ncv, 0, 0, 0, 0)){
    return NULL;
  }
  if(demo_render(nc)){
    return NULL;
  }
  return ncv;
}

static struct ncplane*
zoom_map(struct notcurses* nc, const char* map){
  int averr;
  // determine size that will be represented on screen at once, and how
  // large that section has been rendered in the outzoomed map. take the map
  // and begin opening it on larger and larger planes that fit on the screen
  // less and less. eventually, reach our natural NCSCALE_NONE size and begin
  // scrolling through the map, whooooooooosh.
  struct ncvisual* ncv = ncvisual_open_plane(nc, map, &averr, 0, 0, NCSCALE_NONE);
  if(ncv == NULL){
    return NULL;
  }
  struct AVFrame* frame;
  if((frame = ncvisual_decode(ncv, &averr)) == NULL){
    ncvisual_destroy(ncv);
    return NULL;
  }
  int vheight;
  int vwidth;
  ncplane_dim_yx(ncvisual_plane(ncv), &vheight, &vwidth);
  ncvisual_destroy(ncv);
  // we start at the lower left corner of the outzoomed map
  int truex, truey; // dimensions of true display
  notcurses_term_dim_yx(nc, &truey, &truex);
  int vx = vwidth;
  vheight /= 2;
  int vy = vheight;
  int zoomy = truey;
  int zoomx = truex;
  struct ncplane* zncp = NULL;
  int delty = 2;
  int deltx = 2;
  if(truey > truex){
    ++delty;
  }else if(truex > truey * 2){
    ++deltx;
  }
  while(zoomy < vy && zoomx < vx){
    ncplane_destroy(zncp);
    zoomy += delty;
    zoomx += deltx;
    zncp = notcurses_newplane(nc, zoomy, zoomx, 0, 0, NULL);
    struct ncvisual* zncv = ncplane_visual_open(zncp, map, &averr);
    if(zncv == NULL){
      ncvisual_destroy(ncv);
      ncplane_destroy(zncp);
      return NULL;
    }
    if(ncvisual_decode(zncv, &averr) == NULL){
      ncvisual_destroy(zncv);
      ncplane_destroy(zncp);
      return NULL;
    }
    if(ncvisual_render(zncv, (zoomy - truey) * 2, 0, 0, ((float)truex / zoomx) * zoomx)){
      ncvisual_destroy(zncv);
      ncplane_destroy(zncp);
      return NULL;
    }
    if(demo_render(nc)){
      ncvisual_destroy(zncv);
      ncplane_destroy(zncp);
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
  ncplane_set_default(n, &bgc);
  cell_release(n, &bgc);
  size_t s;
  int sbytes;
  uint64_t channels = 0;
  // optimization so we can elide more color changes, see README's "#perf"
  channels_set_bg_rgb(&channels, 0x00, 0x00, 0x00);
  ncplane_cursor_move_yx(n, 0, 0);
  for(s = 0 ; sprite[s] ; ++s){
    switch(sprite[s]){
      case '0':
        ncplane_cursor_move_yx(n, (s + 1) / 16, (s + 1) % 16);
        break;
      case '1':
        channels_set_fg_rgb(&channels, 0xff, 0xff, 0xff);
        break;
      case '2':
        channels_set_fg_rgb(&channels, 0xe3, 0x9d, 0x25);
        break;
      case '3':
        channels_set_fg_rgb(&channels, 0x3a, 0x84, 0x00);
        break;
    }
    if(sprite[s] != '0'){
      if(ncplane_putegc(n, "\u2588", 0, channels, &sbytes) != 1){
        return -1;
      }
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
    e[i].n = notcurses_newplane(nc, height, width, e[i].yoff, e[i].xoff, NULL);
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
      e[i].xoff += (random() % (truex / 80)) + 1;
      ncplane_move_yx(e[i].n, e[i].yoff, e[i].xoff);
      ++eaglesmoved;
    }
    nanosleep(&flapiter, NULL);
  }while(eaglesmoved);
  for(size_t i = 0 ; i < sizeof(e) / sizeof(*e) ; ++i){
    ncplane_destroy(e[i].n);
  }
  return 0;
}

// motherfucking eagles!
int eagle_demo(struct notcurses* nc){
  char* map = find_data("eagles.png");
  struct ncvisual* zo;
  if((zo = outzoomed_map(nc, map)) == NULL){
    free(map);
    return -1;
  }
  ncvisual_destroy(zo);
  struct ncplane* zncp = zoom_map(nc, map);
  if(zncp == NULL){
    free(map);
    return -1;
  }
  if(eagles(nc)){
    ncplane_destroy(zncp);
    return -1;
  }
  ncplane_destroy(zncp);
  free(map);
  return 0;
}
