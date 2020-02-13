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
static const int PLEN = HUD_COLS - 9 - NSLEN;

typedef struct elem {
  char* name;
  uint64_t startns;
  uint64_t totalns;
  unsigned frames;
  struct elem* next;
} elem;

static bool menu_unrolled;
static struct ncmenu* menu;
static struct ncplane* about; // "about" modal popup

static struct elem* elems;
static struct elem* running;
// which line we're writing the next entry to. once this becomes -1, we stop decrementing
// it, and throw away the oldest entry each time.
static int writeline = HUD_ROWS - 1;

#define MENUSTR_TOGGLE_HUD "Toggle HUD"
#define MENUSTR_RESTART "Restart"
#define MENUSTR_ABOUT "About"
#define MENUSTR_QUIT "Quit"

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

static void
hud_toggle(struct notcurses* nc){
  ncmenu_rollup(menu);
  if(hud){
    hud_destroy();
  }else{
    hud_create(nc);
  }
}

static void
about_toggle(struct notcurses* nc){
  ncmenu_rollup(menu);
  if(about){
    ncplane_destroy(about);
    about = NULL;
    return;
  }
  const int ABOUT_ROWS = 9;
  const int ABOUT_COLS = 40;
  int dimy;
  notcurses_term_dim_yx(nc, &dimy, NULL);
  struct ncplane* n = ncplane_aligned(notcurses_stdplane(nc), ABOUT_ROWS, ABOUT_COLS,
                                      dimy - (ABOUT_ROWS + 2), NCALIGN_CENTER, NULL);
  // let the glyphs below show through, but only dimly
  uint64_t channels = 0;
  channels_set_fg_alpha(&channels, CELL_ALPHA_BLEND);
  channels_set_fg_rgb(&channels, 0x0, 0x0, 0x0);
  channels_set_bg_alpha(&channels, CELL_ALPHA_BLEND);
  channels_set_bg_rgb(&channels, 0x0, 0x0, 0x0);
  if(ncplane_set_base(n, channels, 0, "") >= 0){
    ncplane_set_fg(n, 0xc0f0c0);
    ncplane_set_bg(n, 0);
    ncplane_set_bg_alpha(n, CELL_ALPHA_BLEND);
    ncplane_printf_aligned(n, 1, NCALIGN_CENTER, "notcurses-demo %s", notcurses_version());
    ncplane_printf_aligned(n, 3, NCALIGN_LEFT, "  q quit");
    ncplane_printf_aligned(n, 3, NCALIGN_RIGHT, "restart Ctrl+R  ");
    ncplane_printf_aligned(n, 4, NCALIGN_LEFT, "  H toggle HUD");
    ncplane_printf_aligned(n, 4, NCALIGN_RIGHT, "toggle help Ctrl+U  ");
    ncplane_putstr_aligned(n, 7, NCALIGN_CENTER, "\u00a9 nick black <nickblack@linux.com>");
    cell ul = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
    cell lr = CELL_TRIVIAL_INITIALIZER, ll = CELL_TRIVIAL_INITIALIZER;
    cell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
    if(cells_double_box(n, 0, 0, &ul, &ur, &ll, &lr, &hl, &vl) == 0){
      if(ncplane_perimeter(n, &ul, &ur, &ll, &lr, &hl, &vl, 0) == 0){
        cell_release(n, &ul); cell_release(n, &ur); cell_release(n, &hl);
        cell_release(n, &ll); cell_release(n, &lr); cell_release(n, &vl);
        about = n;
        return;
      }
      cell_release(n, &ul); cell_release(n, &ur); cell_release(n, &hl);
      cell_release(n, &ll); cell_release(n, &lr); cell_release(n, &vl);
    }
  }
  ncplane_destroy(n);
}

// returns true if the input was handled by the menu/HUD
bool menu_or_hud_key(struct notcurses *nc, const struct ncinput *ni){
  struct ncinput tmpni;
  if(menu && ni->id == NCKEY_ENTER){
    const char* sel = ncmenu_selected(menu, &tmpni);
    if(sel == NULL){
      return false;
    }
  }else{
    memcpy(&tmpni, ni, sizeof(tmpni));
  }
  // toggle the HUD
  if(tmpni.id == 'H' && !tmpni.alt && !tmpni.ctrl){
    hud_toggle(nc);
    return true;
  }
  if(tmpni.id == 'U' && !tmpni.alt && tmpni.ctrl){
    about_toggle(nc);
    return true;
  }
  if(tmpni.id == 'R' && !tmpni.alt && tmpni.ctrl){
    ncmenu_rollup(menu);
    interrupt_and_restart_demos();
    return true;
  }
  if(tmpni.id == 'q' && !tmpni.alt && !tmpni.ctrl){
    ncmenu_rollup(menu);
    interrupt_demo();
    return true;
  }
  if(!menu){
    return false;
  }
  if(ncmenu_offer_input(menu, ni)){
    return true;
  }else if(ni->id == 'o' && ni->alt && !ni->ctrl){
    if(ncmenu_unroll(menu, 0) == 0){
      menu_unrolled = true;
    }
    return true;
  }else if(ni->id == 'h' && ni->alt && !ni->ctrl){
    if(ncmenu_unroll(menu, 2) == 0){
      menu_unrolled = true;
    }
    return true;
  }else if(ni->id == '\x1b'){
    if(menu_unrolled){
      ncmenu_rollup(menu);
      menu_unrolled = false;
      return true;
    }
  }
  return false;
}

struct ncmenu* menu_create(struct notcurses* nc){
  struct ncmenu_item demo_items[] = {
    { .desc = MENUSTR_TOGGLE_HUD, .shortcut = { .id = 'H', }, },
    { .desc = NULL, },
    { .desc = MENUSTR_RESTART, .shortcut = { .id = 'R', .ctrl = true, }, },
    { .desc = MENUSTR_QUIT, .shortcut = { .id = 'q', }, },
  };
  struct ncmenu_item help_items[] = {
    { .desc = MENUSTR_ABOUT, .shortcut = { .id = 'U', .ctrl = true, }, },
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
  channels_set_fg(&headerchannels, 0xffffff);
  channels_set_bg(&headerchannels, 0x7f347f);
  channels_set_bg_alpha(&headerchannels, CELL_ALPHA_BLEND);
  const ncmenu_options mopts = {
    .bottom = false,
    .hiding = false,
    .sections = sections,
    .sectioncount = sizeof(sections) / sizeof(*sections),
    .headerchannels = headerchannels,
    .sectionchannels = sectionchannels,
  };
  menu = ncmenu_create(nc, &mopts);
  return menu;
}

static elem**
hud_print_finished(int* line){
  elem** hook = &elems;
  elem* e = elems;
  while(e){
    hook = &e->next;
    if(hud){
      cell c = CELL_TRIVIAL_INITIALIZER;
      ncplane_base(hud, &c);
      ncplane_set_bg(hud, cell_bg(&c));
      ncplane_set_bg_alpha(hud, CELL_ALPHA_BLEND);
      ncplane_set_fg(hud, 0);
      cell_release(hud, &c);
      if(ncplane_printf_yx(hud, *line, 0, "%-6d %*ju.%02jus %-*.*s", e->frames,
                          NSLEN - 3, e->totalns / GIG,
                          (e->totalns % GIG) / (GIG / 100),
                          PLEN, PLEN, e->name) < 0){
        return NULL;
      }
    }
    ++*line;
    e = e->next;
  }
  return hook;
}

struct ncplane* hud_create(struct notcurses* nc){
  if(hud){
    return NULL;
  }
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
  elem* cure;
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
  elem** hook = hud_print_finished(&line);
  if(hook == NULL){
    return -1;
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
  if(hud){
    if(ncplane_printf_yx(hud, line, 0, "%-6d %*ju.%02jus %-*.*s", cure->frames,
                        NSLEN - 3, cure->totalns / GIG,
                        (cure->totalns % GIG) / (GIG / 100),
                        PLEN, PLEN, cure->name) < 0){
      return -1;
    }
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
  if(interrupted){
    return 1;
  }
  if(about){
    ncplane_move_top(about);
  }
  if(menu){
    ncplane_move_top(ncmenu_plane(menu));
  }
  if(hud){
    int plen = HUD_COLS - 4 - NSLEN;
    ncplane_move_top(hud);
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t ns = timespec_to_ns(&ts) - running->startns;
    ++running->frames;
    cell c = CELL_TRIVIAL_INITIALIZER;
    ncplane_base(hud, &c);
    ncplane_set_bg(hud, cell_bg(&c));
    ncplane_set_bg_alpha(hud, CELL_ALPHA_BLEND);
    ncplane_set_fg(hud, 0);
    cell_release(hud, &c);
    if(ncplane_printf_yx(hud, HUD_ROWS - 1, 0, "%-6d %*ju.%02jus %-*.*s",
                         running->frames,
                         NSLEN - 3, ns / GIG, (ns % GIG) / (GIG / 100),
                         plen, plen, running->name) < 0){
      return -1;
    }
    if(ncplane_putegc_yx(hud, 0, HUD_COLS - 1, "\u2a02", NULL) < 0){
      return -1;
    }
  }
  return notcurses_render(nc);
}
