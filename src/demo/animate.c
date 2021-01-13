#include "demo.h"

static int
animate(struct notcurses* nc, struct ncplane* column, int legendy,
        struct ncprogbar* left, struct ncprogbar* right){
  (void)nc; // FIXME
  (void)column;
  (void)legendy;
  (void)left;
  (void)right;
  return 0;
}

static int
make_pbars(struct ncplane* column, struct ncprogbar** left, struct ncprogbar** right){
  int dimy, dimx, coly, colx, colposy, colposx;
  struct notcurses* nc = ncplane_notcurses(column);
  notcurses_stddim_yx(nc, &dimy, &dimx);
  ncplane_dim_yx(column, &coly, &colx);
  ncplane_yx(column, &colposy, &colposx);
  ncplane_options opts = {
    .x = colposx / 4 * -3,
    .rows = coly,
    .cols = (dimx - colx) / 4,
  };
  struct ncplane* leftp = ncplane_create(column, &opts);
  if(leftp == NULL){
    return -1;
  }
  ncplane_set_base(leftp, " ", 0, CHANNELS_RGB_INITIALIZER(0xdd, 0xdd, 0xdd, 0x1b, 0x1b, 0x1b));
  ncprogbar_options popts = { };
  channel_set_rgb8(&popts.brchannel, 0, 0, 0);
  channel_set_rgb8(&popts.blchannel, 0, 0xff, 0);
  channel_set_rgb8(&popts.urchannel, 0, 0, 0xff);
  channel_set_rgb8(&popts.ulchannel, 0, 0xff, 0xff);
  *left = ncprogbar_create(leftp, &popts);
  if(*left == NULL){
    return -1;
  }
  opts.x = colx + colposx / 4;
  struct ncplane* rightp = ncplane_create(column, &opts);
  if(rightp == NULL){
    return -1;
  }
  ncplane_set_base(rightp, " ", 0, CHANNELS_RGB_INITIALIZER(0xdd, 0xdd, 0xdd, 0x1b, 0x1b, 0x1b));
  popts.flags = NCPROGBAR_OPTION_RETROGRADE;
  *right = ncprogbar_create(rightp, &popts);
  if(*right == NULL){
    ncprogbar_destroy(*left);
    return -1;
  }
  return 0;
}

int animate_demo(struct notcurses* nc){
  if(!notcurses_canutf8(nc)){
    return 0;
  }
  int dimy, dimx;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  ncplane_erase(n);
  ncplane_home(n);
  uint32_t tl = 0, tr = 0, bl = 0, br = 0;
  channel_set_rgb8(&tl, 0, 0, 0);
  channel_set_rgb8(&tr, 0, 0xff, 0);
  channel_set_rgb8(&bl, 0, 0, 0xff);
  channel_set_rgb8(&br, 0, 0xff, 0xff);
  if(ncplane_highgradient(n, tl, tr, bl, br, dimy - 1, dimx - 1) < 0){
    return -1;
  }
  ncplane_set_fg_rgb(n, 0xf0f0a0);
  ncplane_set_bg_rgb(n, 0);
  int width = 40;
  if(width > dimx - 8){
    if((width = dimx - 8) <= 0){
      return -1;
    }
  }
  int height = 40;
  if(height >= dimy - 4){
    if((height = dimy - 5) <= 0){
      return -1;
    }
  }
  const int planey = (dimy - height) / 2 + 1;
  ncplane_options nopts = {
    .y = planey,
    .x = NCALIGN_CENTER,
    .rows = height,
    .cols = width,
    .flags = NCPLANE_OPTION_HORALIGNED,
  };
  struct ncplane* column = ncplane_create(n, &nopts);
  if(column == NULL){
    return -1;
  }
  struct ncprogbar *pbarleft, *pbarright;
  if(make_pbars(column, &pbarleft, &pbarright)){
    ncplane_destroy(column);
    return -1;
  }
  ncplane_set_scrolling(column, true);
  int r = animate(nc, column, planey - 2, pbarleft, pbarright);
  ncplane_destroy(column);
  // reflash the gradient to eliminate the counter, setting stage for next demo
  ncplane_cursor_move_yx(n, 1, 0);
  if(ncplane_highgradient(n, tl, tr, bl, br, dimy - 1, dimx - 1) < 0){
    return -1;
  }
  ncprogbar_destroy(pbarleft);
  ncprogbar_destroy(pbarright);
  return r;
}
