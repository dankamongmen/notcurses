#include "demo.h"

// we provide a heads-up display throughout the demo, detailing the demos we're
// about to run, running, and just runned. the user can move this HUD with
// their mouse. it should always be on the top of the z-stack.
struct ncplane* hud = NULL;

// while the HUD is grabbed by the mouse, these are set to the position where
// the grab started. they are reset once the HUD is released.
static int hud_grab_x = -1;
static int hud_grab_y = -1;
// position of the HUD *when grab started*
static int hud_pos_x;
static int hud_pos_y;

static const int HUD_ROWS = 3;
static const int HUD_COLS = 54;

static int
hud_standard_bg(struct ncplane* n){
  cell c = CELL_SIMPLE_INITIALIZER(' ');
  cell_set_bg_rgb(&c, 0xc0, 0xf0, 0xc0);
  ncplane_set_default(n, &c);
  cell_release(n, &c);
  return 0;
}

static int
hud_grabbed_bg(struct ncplane* n){
  cell c = CELL_SIMPLE_INITIALIZER(' ');
  cell_set_bg_rgb(&c, 0x40, 0x90, 0x40);
  ncplane_set_default(n, &c);
  cell_release(n, &c);
  return 0;
}

struct ncplane* hud_create(struct notcurses* nc){
  int dimx, dimy;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  int xoffset = (dimx - HUD_COLS) / 2;
  //int yoffset = (dimy - HUD_ROWS);
  int yoffset = 0;
  struct ncplane* n = notcurses_newplane(nc, HUD_ROWS, HUD_COLS, yoffset, xoffset, NULL);
  if(n == NULL){
    return NULL;
  }
  hud_standard_bg(n);
  uint64_t channels;
  channels_set_fg(&channels, 0xffffff);
  if(ncplane_putegc_yx(n, 0, HUD_COLS - 1, "\u274e", 0, channels, NULL) < 0){
    ncplane_destroy(n);
    return NULL;
  }
  return (hud = n);
}

int hud_destroy(struct ncplane* h){
  hud = NULL;
  return ncplane_destroy(h);
}

// mouse has been pressed on the hud. the caller is responsible for rerendering.
int hud_grab(int y, int x){
  int ret;
  if(hud == NULL){
    return -1;
  }
  // are we in the middle of a grab?
  if(hud_grab_x >= 0 && hud_grab_y >= 0){
    int delty = y - hud_grab_y;
    int deltx = x - hud_grab_x;
    ret = ncplane_move_yx(hud, hud_pos_y + delty, hud_pos_x + deltx);
  }else{
    // new grab. stash point of original grab, and location of HUD at original
    // grab. any delta while grabbed (relative to the original grab point)
    // will see the HUD moved by delta (relative to the original HUD location).
    hud_grab_x = x;
    hud_grab_y = y;
    ncplane_yx(hud, &hud_pos_y, &hud_pos_x);
    if(x == hud_pos_x + HUD_COLS - 1 && y == hud_pos_y){
      return hud_destroy(hud);
    }
    ret = hud_grabbed_bg(hud);
  }
  return ret;
}

int hud_release(void){
  if(hud == NULL){
    return -1;
  }
  hud_grab_x = -1;
  hud_grab_y = -1;
  return hud_standard_bg(hud);
}

int hud_completion_notify(int idx, const demoresult* result){
  // FIXME
  return 0;
}

// inform the HUD of an upcoming demo
int hud_schedule(const char* demoname){
  // FIXME
  return 0;
}

int demo_render(struct notcurses* nc){
  if(hud){
    ncplane_move_top(hud);
  }
  return notcurses_render(nc);
}
