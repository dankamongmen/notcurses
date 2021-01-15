#include "demo.h"

// we have a set of cyclic glyphs, with each cycle composed of some number N_c
// of glyphs. our string is made up of each cycle, with each occupying N_c
// cells, each iterating through the N_c states in a round. the string emerges
// from the center of the screen, moving in a spiral. it then does a loop around
// the columns, and returns back to its hole.
//
// so, each iteration, start at the head of the chain, and move one forward.
// work back along the path, moving back through the string. if we reach the
// end of the string, clear the cell behind it. eventually, we'll clear the
// entirety of the string, and we're done.
//
// path: we know what phase we're in based on the location of the head. within
// the side columns, we're spiraling. beyond them, we're circling. the geometry
// of the circle is dictated by being one beyond the columns everywhere. the
// geometry of the spiral is dictated by distance from the center.
//
// we'll start with just one cycle to prove the path, then add the rest in.
static const char* cycles[] = {
  "🞯🞰🞱🞲🞳🞴",   // 6 five-point asterisks
  "🞵🞶🞷🞸🞹🞺",   // 6 six-point asterisks
  "🞻🞼🞽🞾🞿",    // 5 eight-point asterisks
  "◧◩⬒⬔◨◪⬓⬕", // 8 half-black squares
  "◐◓◑◒",     // 4 half-black circles
  "◢◣◤◥",     // 4 black triangles
  "◰◳◲◱",     // 4 white squares with quadrants
  "◴◷◶◵",     // 4 white circles with quadrants
  "🞅🞆🞇🞈🞉🞊",   // 6 white circles
  "🞎🞏🞐🞑🞒🞓",   // 6 white squares
  "▤▥▦▧▨▩",   // 6 squares with fill
  "⯁⯂⯃⯄",     // 4 regular black polyhedra
  "⌌⌍⌎⌏",     // 4 crops
  NULL,
};

typedef enum {
  PHASE_SPIRAL,
  PHASE_CIRCLE,
  PHASE_CLOSE,
} phase_e;

// get the new head position, given the old head position
static void
get_next_head(struct ncplane* std, struct ncplane* left, struct ncplane* right,
              int* heady, int* headx, phase_e* phase){
  int lrcol; // left column of right progbar
  int rlcol; // right column of left progbar
  int trow, brow; // top and bottom
  if(*heady == -1 && *headx == -1){
    *headx = ncplane_dim_x(std) / 2;
    *heady = ncplane_dim_y(std) / 2;
    *phase = PHASE_SPIRAL;
    return;
  }
  ncplane_abs_yx(right, &trow, &lrcol);
  ncplane_abs_yx(left, &brow, &rlcol);
  rlcol += ncplane_dim_x(left) - 1;
  brow += ncplane_dim_y(left) - 1;
  // if we're ending, we either remain where we are, or move down
  if(*phase == PHASE_CLOSE){
    if(*heady < ncplane_dim_y(std) / 2){
      ++*heady;
    }else if(*headx > ncplane_dim_x(std) / 2){
      --*headx; // done
    }
    return;
  }
  // we're above the columns. head left, unless we're at top left, in which
  // case we head down, or if we're in PHASE_CIRCLE and at the top center,
  // in which case move into PHASE_CLOSE.
  if(*heady == trow - 2){
    if(*headx == ncplane_dim_x(std) / 2 + 1 && *phase == PHASE_CIRCLE){
      *phase = PHASE_CLOSE;
      ++*heady;
      return;
    }
    if(*headx == rlcol - ncplane_dim_x(left) - 2){
      ++*heady; // turn the corner
      return;
    }
    if(*headx == ncplane_dim_x(std) / 2){
      *phase = PHASE_CIRCLE;
    }
    --*headx;
  // we're to the left of the columns. head down, until bottom left.
  }else if(*headx == rlcol - ncplane_dim_x(left) - 2){
    if(*heady == brow + 2){
      ++*headx;
    }else{
      ++*heady;
    }
  // we're below the columns. head right, until bottom right.
  }else if(*heady == brow + 2){
    if(*headx == lrcol + ncplane_dim_x(right) + 1){
      --*heady;
    }else{
      ++*headx;
    }
  // we're to the right of the columns. head up, until top right.
  }else if(*headx == lrcol + ncplane_dim_x(right) + 2){
    if(*heady == trow - 2){
      --*headx;
    }else{
      --*heady;
    }
  }else{
    // in the spiral cycle. it's a counterclockwise spiral, come out the bottom,
    // calculate distances from the center in both directions. if the absolute
    // values are equal, turn counterclockwise, *unless* xdist is positive and
    // ydist is negative. in that case, we're coming down the left side, and
    // need go down one further, only then turning right. that case is xdist is
    // positive, ydist is negative, and xdist + ydist == -1. otherwise, continue
    // moving counterclockwise (right if |ydist|>|xdist| and negative y, left
    // if |ydist|>|xdist| and positive y, etc.)
    int ydist = ncplane_dim_y(std) / 2 - *heady;
    int xdist = ncplane_dim_x(std) / 2 - *headx;
    if(ydist == 0 && xdist == 0){
      ++*heady; // move down
    }else if(abs(ydist) == abs(xdist)){ // corner
      if(ydist < 0 && xdist > 0){ // lower-left, move down
        ++*heady; // move down
      }else if(ydist < 0 && xdist < 0){ // lower-right, move up
        --*heady;
      }else if(xdist > 0){ // upper-left, move down
        ++*heady;
      }else{ // upper-right, love left
        --*headx;
      }
    }else if(ydist < 0 && xdist > 0 && ydist + xdist == -1){ // new iteration
      ++*headx;
    }else{
      if(abs(ydist) > abs(xdist)){
        if(ydist < 0){
          ++*headx;
        }else{
          --*headx;
        }
      }else{
        if(xdist < 0){
          --*heady;
        }else{
          ++*heady;
        }
      }
    }
  }
}

static void
get_next_end(struct ncplane* std, struct ncplane* left, struct ncplane* right,
             int* endy, int* endx, phase_e* endphase){
  get_next_head(std, left, right, endy, endx, endphase);
}

// determine the total number of moves we will make
static int
determine_totalmoves(struct ncplane* left, struct ncplane* right){
  int lrcol; // left column of right progbar
  int rlcol; // right column of left progbar
  int trow, brow; // top and bottom
  ncplane_abs_yx(right, &trow, &lrcol);
  ncplane_abs_yx(left, &brow, &rlcol);
  rlcol += ncplane_dim_x(left) - 1;
  brow += ncplane_dim_y(left) - 1;
  return ((brow - trow + 3) * (brow - trow + 3) +
          ncplane_dim_x(left) * 2 + ncplane_dim_y(left) * 2);
}

static int
animate(struct notcurses* nc, struct ncprogbar* left, struct ncprogbar* right){
  int dimy, dimx;
  struct ncplane* std = notcurses_stddim_yx(nc, &dimy, &dimx);
  int headx = -1;
  int heady = -1;
  int endy = -1;
  int endx = -1;
  int length = 0;
  int totallength = 0;
  for(const char** c = cycles ; *c ; ++c){
    totallength += ncstrwidth(*c);
  }
  const int totalmoves = determine_totalmoves(ncprogbar_plane(left), ncprogbar_plane(right));
  // headx and heady will not return to their starting location until the
  // string begins to disappear. endx and endy won't equal heady/headx until
  // the entire string has been consumed.
  struct timespec delay;
  uint64_t iterns = (timespec_to_ns(&demodelay) * 5) / totalmoves;
  phase_e headphase, endphase;
  int moves = 0;
  struct timespec expected;
  clock_gettime(CLOCK_MONOTONIC, &expected);
  uint64_t expect_ns = timespec_to_ns(&expected);
  do{
    uint64_t channels;
    free(ncplane_at_yx(std, heady, headx, NULL, &channels));
    ncplane_set_bg_rgb(std, channels_bg_rgb(channels));
    ncplane_set_fg_rgb(std, 0xffffff);
    ncplane_putchar_yx(std, heady, headx, 'X');
    get_next_head(std, ncprogbar_plane(left), ncprogbar_plane(right),
                  &heady, &headx, &headphase);
    // FIXME need to iterate each character through its cycle
    if(length < totallength){
      ++length;
    }else{
      ncplane_putchar_yx(std, endy, endx, '\0');
      get_next_end(std, ncprogbar_plane(left), ncprogbar_plane(right),
                  &endy, &endx, &endphase);
    }
    ++moves;
    ncprogbar_set_progress(left, ((float)moves) / totalmoves);
    ncprogbar_set_progress(right, ((float)moves) / totalmoves);
    DEMO_RENDER(nc);
    struct timespec now;
    expect_ns += iterns;
    clock_gettime(CLOCK_MONOTONIC, &now);
    uint64_t nowns = timespec_to_ns(&now);
    if(nowns < expect_ns){
      ns_to_timespec(expect_ns - nowns, &delay);
      demo_nanosleep(nc, &delay);
    }
  }while(endy != heady || endx != headx);
  ncprogbar_set_progress(left, 1);
  ncprogbar_set_progress(right, 1);
  DEMO_RENDER(nc);
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
  ncplane_destroy(column);
  int r = animate(nc, pbarleft, pbarright);
  // reflash the gradient to eliminate the counter, setting stage for next demo
  ncplane_cursor_move_yx(n, 1, 0);
  if(ncplane_highgradient(n, tl, tr, bl, br, dimy - 1, dimx - 1) < 0){
    return -1;
  }
  ncprogbar_destroy(pbarleft);
  ncprogbar_destroy(pbarright);
  return r;
}
