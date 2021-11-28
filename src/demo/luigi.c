#include "demo.h"

//0 = trans
//1 = white
//2 = yellow
//3 = green

static const char* luigis[] = {
  "0000000000000000"
  "0000000000000000"
  "0000000111110000"
  "0000011111120000"
  "0000111111220000"
  "0000111111111110"
  "0000333223222000"
  "0003223223322220"
  "0003223322222222"
  "0033223322232222"
  "0033222223333330"
  "0003322222333330"
  "0000032222222200"
  "0000311122200000"
  "0003133313000000"
  "0003133331300000"
  "0033133333112200"
  "0031133333332222"
  "0031113333332222"
  "0001113333333222"
  "0001111333333222"
  "0001111113331000"
  "0001111111111000"
  "0001111111113000"
  "3333111111131100"
  "3333111113311100"
  "3333111131111000"
  "3333111001111000"
  "3333000003333000"
  "3300000003333000"
  "3000000003333330"
  "0000000003333330",

  "0000000000000000"
  "0000001111100000"
  "0000111111200000"
  "0001111112200000"
  "0001111111111100"
  "0003332232220000"
  "0032232233222200"
  "0032233222222220"
  "0332233222322220"
  "0332222233333300"
  "0033222223333300"
  "0003322222222000"
  "0000111122000000"
  "0003133113300000"
  "0031333311300000"
  "0031333311330000"
  "0031333311130000"
  "0031333332230000"
  "0031333322220000"
  "0011133322221000"
  "0011133322221100"
  "0011113322211100"
  "0011111133111100"
  "0001111133311000"
  "0000111333333000"
  "0000113333330000"
  "0000011333300000"
  "0000031113330000"
  "0000033330330000"
  "0000333330000000"
  "0000333333300000"
  "0000003333300000",

  "0000001111100000"
  "0000111111200000"
  "0001111112200000"
  "0001111111111100"
  "0003332232220000"
  "0032232233222200"
  "0032233222222220"
  "0332233222322220"
  "0332222233333300"
  "0333222223333300"
  "0003322222222000"
  "0000033322000000"
  "0000111133100020"
  "0003333113310222"
  "0033333311313222"
  "0333333311331222"
  "0333333311331323"
  "0333333111331330"
  "3333331112132300"
  "3333111111111000"
  "2222211111111000"
  "2222211111111003"
  "2222111111111033"
  "0222111111133333"
  "0001311111133333"
  "0031131111133333"
  "3331113311133333"
  "3333111100033333"
  "3333310000000000"
  "0333000000000000"
  "0333000000000000"
  "0033300000000000",

  NULL
};


static int
draw_luigi(struct ncplane* n, const char* sprite){
  uint64_t channels = 0;
  ncchannels_set_fg_alpha(&channels, NCALPHA_TRANSPARENT);
  ncchannels_set_bg_alpha(&channels, NCALPHA_TRANSPARENT);
  ncplane_set_base(n, "", 0, channels);
  size_t s;
  // optimization so we can elide more color changes, see README's "#perf"
  ncplane_set_bg_rgb8(n, 0x00, 0x00, 0x00);
  for(s = 0 ; sprite[s] ; ++s){
    switch(sprite[s]){
      case '0':
        break;
      case '1':
        ncplane_set_bg_rgb8(n, 0xff, 0xff, 0xff);
        break;
      case '2':
        ncplane_set_bg_rgb8(n, 0xe3, 0x9d, 0x25);
        break;
      case '3':
        ncplane_set_bg_rgb8(n, 0x3a, 0x84, 0x00);
        break;
    }
    if(sprite[s] != '0'){
      if(ncplane_putegc_yx(n, s / 16, s % 16, " ", NULL) != 1){
        return -1;
      }
    }
  }
  return 0;
}

int luigi_demo(struct notcurses* nc, uint64_t startns){
  (void)startns;
  if(!notcurses_canopen_images(nc)){
    return 0;
  }
  unsigned rows, cols;
  char* map = find_data("megaman2.bmp");
  struct ncvisual* nv = ncvisual_from_file(map);
  free(map);
  if(nv == NULL){
    return -1;
  }
  struct ncvisual_options vopts = {
    .n = notcurses_stddim_yx(nc, &rows, &cols),
    .scaling = NCSCALE_STRETCH,
    .flags = NCVISUAL_OPTION_NOINTERPOLATE,
  };
  if(ncvisual_blit(nc, nv, &vopts) == NULL){
    return -1;
  }
  assert(1 == ncvisual_decode(nv));
  // he should be walking on the platform ~4/5 of the way down
  const int height = 32;
  int yoff = rows * 4 / 5 - height + 1; // tuned
  struct ncplane* lns[3];
  unsigned i;
  struct ncplane* lastseen = NULL;
  for(i = 0 ; i < 3 ; ++i){
    struct ncplane_options nopts = {
      .y = yoff,
      .rows = height,
      .cols = 16,
    };
    lns[i] = ncplane_create(notcurses_stdplane(nc), &nopts);
    if(lns[i] == NULL){
      while(i--){
        ncplane_destroy(lns[i]);
      }
      return -1;
    }
    lastseen = lns[i];
    draw_luigi(lns[i], luigis[i]);
    ncplane_move_bottom(lastseen); // all start hidden underneath stdplane
  }
  struct timespec stepdelay;
  ns_to_timespec(timespec_to_ns(&demodelay) / (cols - 16 - 1), &stepdelay);
  struct ncvisual* wmncv = NULL;
  char* fname = find_data("warmech.bmp");
  if(fname == NULL){
    return -1;
  }
  wmncv = ncvisual_from_file(fname);
  free(fname);
  if(wmncv == NULL){
    return -1;
  }
  uint64_t channels = 0;
  ncchannels_set_fg_alpha(&channels, NCALPHA_TRANSPARENT);
  ncchannels_set_bg_alpha(&channels, NCALPHA_TRANSPARENT);
  struct ncvisual_options wvopts = {
    .n = notcurses_stdplane(nc),
    .flags = NCVISUAL_OPTION_CHILDPLANE,
  };
  struct ncplane* wmplane = ncvisual_blit(nc, wmncv, &wvopts);
  if(wmplane == NULL){
    ncvisual_destroy(wmncv);
    return -1;
  }
  ncplane_set_base(wmplane, "", 0, channels);
  for(i = 0 ; i < cols - 16 - 1 + 50 ; ++i){
    if(i + 16 >= cols - 16 - 1){
      --yoff;
    }else{
      ncplane_move_bottom(lastseen); // hide the previous sprite
      lastseen = lns[i % 3];
      ncplane_move_top(lastseen);
    }
    ncplane_move_yx(lastseen, yoff, i);
    ncvgeom geom;
    ncvisual_geom(nc, wmncv, NULL, &geom);
    geom.pixy /= geom.scaley;
    // FIXME what the fuck is this
    ncplane_move_yx(wmplane, rows * 4 / 5 - geom.pixy + 1 + (i % 2), i - 60);
    DEMO_RENDER(nc);
    demo_nanosleep(nc, &stepdelay);
  }
  for(i = 0 ; i < 3 ; ++i){
    ncplane_destroy(lns[i]);
  }
  ncplane_destroy(wmplane);
  ncvisual_destroy(nv);
  ncvisual_destroy(wmncv);
  return 0;
}
