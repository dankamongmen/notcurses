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
static const int HUD_COLS = 30;

typedef struct elem {
  char* name;
  uint64_t startns;
  uint64_t totalns;
  struct elem* next;
} elem;

static struct elem* elems;
static struct elem* running;
// which line we're writing the next entry to. once this becomes -1, we stop decrementing
// it, and throw away the oldest entry each time.
static int writeline = HUD_ROWS - 1;

int demo_fader(struct notcurses* nc, struct ncplane* ncp){
  (void)ncp;
  return demo_render(nc);
}

static int
hud_standard_bg(struct ncplane* n){
  cell c = CELL_SIMPLE_INITIALIZER(' ');
  cell_set_bg_rgb(&c, 0xc0, 0xf0, 0xc0);
  cell_set_bg_alpha(&c, CELL_ALPHA_BLEND);
  ncplane_set_base(n, &c);
  cell_release(n, &c);
  return 0;
}

static int
hud_grabbed_bg(struct ncplane* n){
  cell c = CELL_SIMPLE_INITIALIZER(' ');
  cell_set_bg_rgb(&c, 0x40, 0x90, 0x40);
  ncplane_set_base(n, &c);
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
  uint64_t channels = 0;
  channels_set_fg(&channels, 0xffffff);
  channels_set_bg(&channels, 0xffffff);
  ncplane_set_bg(n, 0x409040);
  if(ncplane_putegc_yx(n, 0, HUD_COLS - 1, "\u2612", 0, channels, NULL) < 0){
    ncplane_destroy(n);
    return NULL;
  }
  return (hud = n);
}

int hud_destroy(void){
  int ret = 0;
  if(hud){
    ret = ncplane_destroy(hud);
    hud = NULL;
  }
  return ret;
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
      return hud_destroy();
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

// currently running demo is always at y = HUD_ROWS-1
int hud_completion_notify(const demoresult* result){
  if(running){
    running->totalns = result->timens;
  }
  return 0;
}

// inform the HUD of an upcoming demo
int hud_schedule(const char* demoname){
  if(hud == NULL){
    return -1;
  }
  cell c = CELL_TRIVIAL_INITIALIZER;
  ncplane_base(hud, &c);
  ncplane_set_bg(hud, cell_bg(&c));
  ncplane_set_bg_alpha(hud, CELL_ALPHA_BLEND);
  ncplane_set_fg(hud, 0);
  elem* cure;
  elem** hook = &elems;
  int line = writeline;
  // once we pass through this conditional:
  //  * cure is ready to write to, and print at y = HUD_ROWS - 1
  //  * hooks is ready to enqueue cure to
  //  * reused entries have been printed, if any exist
  if(line == -1){
    cure = elems;
    elems = cure->next;
    line = 0;
    free(cure->name);
  }else{
    --writeline;
    cure = malloc(sizeof(*cure));
  }
  elem* e = elems;
  int nslen = 14;
  int plen = HUD_COLS - 4 - nslen;
  while(e){
    hook = &e->next;
    if(ncplane_printf_yx(hud, line, 0, "%*luns %*.*s", nslen, e->totalns, plen, plen, e->name) < 0){
      return -1;
    }
    ++line;
    e = e->next;
  }
  *hook = cure;
  cure->name = strdup(demoname);
  cure->next = NULL;
  cure->totalns = 0;
  struct timespec cur;
  clock_gettime(CLOCK_MONOTONIC, &cur);
  cure->startns = timespec_to_ns(&cur);
  running = cure;
  if(ncplane_printf_yx(hud, line, 0, "%*luns %-*.*s", nslen, cure->totalns, plen, plen, cure->name) < 0){
    return -1;
  }
  return 0;
}

int demo_render(struct notcurses* nc){
  if(hud){
    int nslen = 14;
    int plen = HUD_COLS - 4 - nslen;
    ncplane_move_top(hud);
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    if(ncplane_printf_yx(hud, HUD_ROWS - 1, 0, "%*luns %-*.*s", nslen,
                         timespec_to_ns(&ts) - running->startns,
                         plen, plen, running->name) < 0){
      return -1;
    }
  }
  return notcurses_render(nc);
}
