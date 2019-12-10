#include "demo.h"

//0 = trans
//1 = white
//2 = yellow
//3 = green

static const char luigi1[] = "0000000000000000"
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
"0000000003333330";

static const char luigi2[] = "0000000000000000"
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
"0000003333300000";

static const char luigi3[] = "0000001111100000"
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
"0033300000000000";

static int
draw_luigi(struct ncplane* n, const char* sprite){
  cell bgc = CELL_TRIVIAL_INITIALIZER;
  cell_bg_set_alpha(&bgc, 3);
  ncplane_set_background(n, &bgc);
  cell_release(n, &bgc);
  size_t s;
  int sbytes;
  uint64_t channels = 0;
  for(s = 0 ; s < strlen(sprite) ; ++s){
    switch(sprite[s]){
      case '0':
        ncplane_cursor_move_yx(n, (s + 1) / 16, (s + 1) % 16);
        break;
      case '1':
        notcurses_fg_prep(&channels, 255, 255, 255);
        break;
      case '2':
        notcurses_fg_prep(&channels, 0xe3, 0x9d, 0x25);
        break;
      case '3':
        notcurses_fg_prep(&channels, 0x3a, 0x84, 0x00);
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

int luigi_demo(struct notcurses* nc){
  struct ncplane* n = notcurses_stdplane(nc);
  int averr = 0;
  struct ncvisual* nv = ncplane_visual_open(n, "../tests/megaman2.bmp", &averr);
  if(nv == NULL){
    return -1;
  }
  if(ncvisual_decode(nv, &averr) == NULL){
    return -1;
  }
  if(ncvisual_render(nv)){
    return -1;
  }
  int rows, cols;
  ncplane_dim_yx(n, &rows, &cols);
  // he should be walking on the platform ~4/5 of the way down
  const int height = 32;
  int yoff = rows * 4 / 5 - height + 1; // tuned
  struct ncplane* lns[3];
  int i;
  for(i = 0 ; i < 3 ; ++i){
    lns[i] = notcurses_newplane(nc, height, 16, yoff, -16, NULL);
    if(lns[i] == NULL){
      while(--i){
        ncplane_destroy(lns[i]);
      }
      return -1;
    }
  }
  draw_luigi(lns[0], luigi1);
  draw_luigi(lns[1], luigi2);
  draw_luigi(lns[2], luigi3);
  struct ncplane* lastseen = NULL;
  struct timespec stepdelay;
  ns_to_timespec(timespec_to_ns(&demodelay) / (cols - 16 - 1), &stepdelay);
  for(i = 0 ; i < cols - 16 - 1 ; ++i){
    if(lastseen){ // hide the previous sprite
      ncplane_move_yx(lastseen, yoff, -16);
    }
    if(i % 4 == 3){
      lastseen = lns[1];
    }else{
      lastseen = lns[i % 4];
    }
    ncplane_move_yx(lastseen, yoff, i);
    notcurses_render(nc);
    nanosleep(&stepdelay, NULL);
  }
  for(i = 0 ; i < 3 ; ++i){
    ncplane_destroy(lns[i]);
  }
  ncvisual_destroy(nv);
  return 0;
}
