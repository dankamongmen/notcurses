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

// how many columns for runtime?
static const int NSLEN = 9;
static const int HUD_ROWS = 3;
static const int HUD_COLS = 30;

typedef struct elem {
  char* name;
  uint64_t startns;
  uint64_t totalns;
  unsigned frames;
  struct elem* next;
} elem;

static struct elem* elems;
static struct elem* running;
// which line we're writing the next entry to. once this becomes -1, we stop decrementing
// it, and throw away the oldest entry each time.
static int writeline = HUD_ROWS - 1;

int demo_fader(struct notcurses* nc, struct ncplane* ncp, void* curry){
  (void)ncp;
  (void)curry;
  return demo_render(nc);
}

static int
hud_standard_bg(struct ncplane* n){
  cell c = CELL_SIMPLE_INITIALIZER(' ');
  cell_set_bg_rgb(&c, 0xc0, 0xf0, 0xc0);
  cell_set_bg_alpha(&c, CELL_ALPHA_BLEND);
  ncplane_set_base_cell(n, &c);
  cell_release(n, &c);
  return 0;
}

static int
hud_grabbed_bg(struct ncplane* n){
  cell c = CELL_SIMPLE_INITIALIZER(' ');
  cell_set_bg_rgb(&c, 0x40, 0x90, 0x40);
  ncplane_set_base_cell(n, &c);
  cell_release(n, &c);
  return 0;
}

struct ncmenu* menu_create(struct notcurses* nc){
  struct ncmenu_item demo_items[] = {
    { .desc = "Restart", .shortcut = { .id = 'r', .ctrl = true, }, },
  };
  struct ncmenu_item help_items[] = {
    { .desc = "About", .shortcut = { .id = 'u', .ctrl = true, }, },
  };
  struct ncmenu_section sections[] = {
    { .name = "notcurses-demo", .items = demo_items,
      .itemcount = sizeof(demo_items) / sizeof(*demo_items),
      .shortcut = { .id = 'o', .alt = true, }, },
    { .name = NULL, .items = NULL, .itemcount = 0, },
    { .name = "help", .items = help_items,
      .itemcount = sizeof(help_items) / sizeof(*help_items),
      .shortcut = { .id = 'h', .alt = true, }, },
  };
  uint64_t headerchannels = 0;
  uint64_t sectionchannels = 0;
  channels_set_fg(&headerchannels, 0x00ff00);
  channels_set_bg(&headerchannels, 0x440000);
  channels_set_fg_alpha(&sectionchannels, CELL_ALPHA_TRANSPARENT);
  channels_set_bg_alpha(&sectionchannels, CELL_ALPHA_TRANSPARENT);
  const ncmenu_options mopts = {
    .bottom = false,
    .hiding = false,
    .sections = sections,
    .sectioncount = sizeof(sections) / sizeof(*sections),
    .headerchannels = headerchannels,
    .sectionchannels = sectionchannels,
  };
  return ncmenu_create(nc, &mopts);
}

struct ncplane* hud_create(struct notcurses* nc){
  int dimx, dimy;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  int yoffset = dimy - HUD_ROWS - 1;
  struct ncplane* n = ncplane_new(nc, HUD_ROWS, HUD_COLS, yoffset, 1, NULL);
  if(n == NULL){
    return NULL;
  }
  hud_standard_bg(n);
  ncplane_set_fg(n, 0xffffff);
  ncplane_set_bg(n, 0x409040);
  if(ncplane_putegc_yx(n, 0, HUD_COLS - 1, "\u2612", NULL) < 0){
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
    running->frames = result->stats.renders;
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
  int plen = HUD_COLS - 10 - NSLEN;
  while(e){
    hook = &e->next;
    if(ncplane_printf_yx(hud, line, 0, "%-6d %*ju.%02jus %-*.*s", e->frames,
                          NSLEN - 3, e->totalns / GIG,
                          (e->totalns % GIG) / (GIG / 100),
                          plen, plen, e->name) < 0){
      return -1;
    }
    ++line;
    e = e->next;
  }
  *hook = cure;
  cure->name = strdup(demoname);
  cure->next = NULL;
  cure->totalns = 0;
  cure->frames = 0;
  struct timespec cur;
  clock_gettime(CLOCK_MONOTONIC, &cur);
  cure->startns = timespec_to_ns(&cur);
  running = cure;
  if(ncplane_printf_yx(hud, line, 0, "%-6d %*ju.%02jus %-*.*s", cure->frames,
                        NSLEN - 3, cure->totalns / GIG,
                        (cure->totalns % GIG) / (GIG / 100),
                        plen, plen, cure->name) < 0){
    return -1;
  }
  return 0;
}

// wake up every 100ms and render a frame so the HUD doesn't appear locked up
int demo_nanosleep(struct notcurses* nc, const struct timespec *ts){
  struct timespec fsleep;
  if(hud){
    uint64_t nstotal = timespec_to_ns(ts);
    uint64_t deadline;
    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    deadline = timespec_to_ns(&now) + nstotal;
    while(deadline - timespec_to_ns(&now) > GIG / 10){
      fsleep.tv_sec = 0;
      fsleep.tv_nsec = GIG / 10;
      nanosleep(&fsleep, NULL);
      demo_render(nc);
      clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    }
    ns_to_timespec(deadline - timespec_to_ns(&now), &fsleep);
  }else{
    fsleep = *ts;
  }
  nanosleep(&fsleep, NULL);
  return 0;
}

int demo_render(struct notcurses* nc){
  if(hud){
    int plen = HUD_COLS - 4 - NSLEN;
    ncplane_move_top(hud);
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t ns = timespec_to_ns(&ts) - running->startns;
    ++running->frames;
    if(ncplane_printf_yx(hud, HUD_ROWS - 1, 0, "%-6d %*ju.%02jus %-*.*s",
                         running->frames,
                         NSLEN - 3, ns / GIG, (ns % GIG) / (GIG / 100),
                         plen, plen, running->name) < 0){
      return -1;
    }
  }
  return notcurses_render(nc);
}
