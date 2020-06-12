#include "demo.h"
#include <pthread.h>

// we provide a heads-up display throughout the demo, detailing the demos we're
// about to run, running, and just runned. the user can move this HUD with
// their mouse. it should always be on the top of the z-stack, unless hidden.
struct ncplane* hud = NULL;

static bool hud_hidden = true;
static bool plot_hidden = true;
static struct ncuplot* plot;
static uint64_t plottimestart;

// while the HUD is grabbed by the mouse, these are set to the position where
// the grab started. they are reset once the HUD is released.
static int hud_grab_x = -1;
static int hud_grab_y = -1;
// position of the HUD *when grab started*
static int hud_pos_x;
static int hud_pos_y;

// while the plot is grabbed by the mouse, these are set to the position where
// the grab started. they are reset once the plot is released.
static int plot_grab_y = -1;
// position of the plot *when grab started*
static int plot_pos_y;

// how many columns for runtime?
static const int NSLEN = 9;
static const int HUD_ROWS = 3 + 2; // 2 for borders
static const int HUD_COLS = 30 + 2; // 2 for borders
static const int PLEN = HUD_COLS - 11 - NSLEN;

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
static int writeline = HUD_ROWS - 2;

#define MENUSTR_TOGGLE_HUD "Toggle HUD"
#define MENUSTR_TOGGLE_PLOT "Toggle FPS plot"
#define MENUSTR_RESTART "Restart"
#define MENUSTR_ABOUT "About"
#define MENUSTR_QUIT "Quit"

static int
hud_standard_bg(struct ncplane* n){
  uint64_t channels = 0;
  channels_set_fg_alpha(&channels, CELL_ALPHA_BLEND);
  channels_set_fg_rgb(&channels, 0x0, 0x0, 0x0);
  channels_set_bg_alpha(&channels, CELL_ALPHA_BLEND);
  channels_set_bg_rgb(&channels, 0x0, 0x0, 0x0);
  if(ncplane_set_base(n, " ", 0, channels) >= 0){
    return -1;
  }
  return 0;
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
  struct ncplane* n = ncplane_aligned(notcurses_stdplane(nc), ABOUT_ROWS,
                                      ABOUT_COLS, 3, NCALIGN_CENTER, NULL);
  // let the glyphs below show through, but only dimly
  uint64_t channels = 0;
  channels_set_fg_alpha(&channels, CELL_ALPHA_BLEND);
  channels_set_fg_rgb(&channels, 0x0, 0x0, 0x0);
  channels_set_bg_alpha(&channels, CELL_ALPHA_BLEND);
  channels_set_bg_rgb(&channels, 0x0, 0x0, 0x0);
  if(ncplane_set_base(n, "", 0, channels) >= 0){
    ncplane_set_fg(n, 0x11ffff);
    ncplane_set_bg(n, 0);
    ncplane_set_bg_alpha(n, CELL_ALPHA_BLEND);
    ncplane_printf_aligned(n, 1, NCALIGN_CENTER, "notcurses-demo %s", notcurses_version());
    ncplane_printf_aligned(n, 3, NCALIGN_LEFT, "  P toggle plot");
    ncplane_printf_aligned(n, 3, NCALIGN_RIGHT, "toggle help Ctrl+U  ");
    ncplane_printf_aligned(n, 4, NCALIGN_LEFT, "  H toggle HUD");
    ncplane_printf_aligned(n, 4, NCALIGN_RIGHT, "restart Ctrl+R  ");
    ncplane_printf_aligned(n, 5, NCALIGN_CENTER, "q quit");
    ncplane_putstr_aligned(n, 7, NCALIGN_CENTER, "\u00a9 nick black <nickblack@linux.com>");
    cell ul = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
    cell lr = CELL_TRIVIAL_INITIALIZER, ll = CELL_TRIVIAL_INITIALIZER;
    cell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
    channels = 0;
    channels_set_fg(&channels, 0xc020c0);
    channels_set_bg(&channels, 0);
    if(cells_double_box(n, 0, channels, &ul, &ur, &ll, &lr, &hl, &vl) == 0){
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

static void
hud_toggle(struct notcurses* nc){
  ncmenu_rollup(menu);
  if(!hud){
    return;
  }
  hud_hidden = !hud_hidden;
  if(hud_hidden){
    ncplane_move_bottom(hud);
  }else{
    ncplane_move_top(hud);
  }
  demo_render(nc);
}

static int
fpsplot_toggle(struct notcurses* nc){
  ncmenu_rollup(menu);
  if(!plot){
    return 0;
  }
  plot_hidden = !plot_hidden;
  if(plot_hidden){
    ncplane_move_bottom(ncuplot_plane(plot));
  }else{
    ncplane_move_top(ncuplot_plane(plot));
  }
  return demo_render(nc);
}

// returns true if the input was handled by the menu/HUD. 'q' is passed through
// (we return false) so that it can interrupt a demo blocking on input.
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
  if(tmpni.id == 'P' && !tmpni.alt && !tmpni.ctrl){
    fpsplot_toggle(nc);
    return true;
  }
  if(tmpni.id == 'U' && !tmpni.alt && tmpni.ctrl){
    about_toggle(nc);
    return true;
  }
  if(tmpni.id == 'R' && !tmpni.alt && tmpni.ctrl){
    if(menu){
      ncmenu_rollup(menu);
    }
    interrupt_and_restart_demos();
    return true;
  }
  if(tmpni.id == 'q' && !tmpni.alt && !tmpni.ctrl){
    if(menu){
      ncmenu_rollup(menu);
    }
    interrupt_demo();
    return false; // see comment above
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
    { .desc = MENUSTR_TOGGLE_PLOT, .shortcut = { .id = 'P', }, },
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
  channels_set_fg(&sectionchannels, 0xffffff);
  channels_set_bg(&sectionchannels, 0x000000);
  channels_set_fg_alpha(&sectionchannels, CELL_ALPHA_HIGHCONTRAST);
  channels_set_bg_alpha(&sectionchannels, CELL_ALPHA_BLEND);
  channels_set_fg(&headerchannels, 0xffffff);
  channels_set_bg(&headerchannels, 0x7f347f);
  channels_set_bg_alpha(&headerchannels, CELL_ALPHA_BLEND);
  const ncmenu_options mopts = {
    .sections = sections,
    .sectioncount = sizeof(sections) / sizeof(*sections),
    .headerchannels = headerchannels,
    .sectionchannels = sectionchannels,
    .flags = 0,
  };
  menu = ncmenu_create(notcurses_stdplane(nc), &mopts);
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
      ncplane_set_fg(hud, 0xffffff);
      cell_release(hud, &c);
      if(ncplane_printf_yx(hud, *line, 1, "%-6d %*ju.%02jus %-*.*s", e->frames,
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
  int yoffset = 13;
  struct ncplane* n = ncplane_new(nc, HUD_ROWS, HUD_COLS, yoffset, 1, NULL);
  if(n == NULL){
    return NULL;
  }
  hud_standard_bg(n);
  cell ul = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
  cell lr = CELL_TRIVIAL_INITIALIZER, ll = CELL_TRIVIAL_INITIALIZER;
  cell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
  if(cells_double_box(n, 0, 0, &ul, &ur, &ll, &lr, &hl, &vl)){
    ncplane_destroy(n);
    return NULL;
  }
  cell_set_fg(&ul, 0xc0f0c0);
  cell_set_fg(&ur, 0xc0f0c0);
  cell_set_fg(&ll, 0xc0f0c0);
  cell_set_fg(&lr, 0xc0f0c0);
  cell_set_fg(&hl, 0xc0f0c0);
  cell_set_fg(&vl, 0xc0f0c0);
  cell_set_bg(&ul, 0);
  cell_set_bg(&ur, 0);
  cell_set_bg(&ll, 0);
  cell_set_bg(&lr, 0);
  cell_set_bg(&hl, 0);
  cell_set_bg(&vl, 0);
  if(ncplane_perimeter(n, &ul, &ur, &ll, &lr, &hl, &vl, 0)){
    cell_release(n, &ul); cell_release(n, &ur); cell_release(n, &hl);
    cell_release(n, &ll); cell_release(n, &lr); cell_release(n, &vl);
    ncplane_destroy(n);
    return NULL;
  }
  cell_release(n, &ul); cell_release(n, &ur); cell_release(n, &hl);
  cell_release(n, &ll); cell_release(n, &lr); cell_release(n, &vl);
  ncplane_set_fg(n, 0xffffff);
  ncplane_set_bg(n, 0);
  ncplane_set_bg_alpha(n, CELL_ALPHA_BLEND);
  if(hud_hidden){
    ncplane_move_bottom(n);
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
  if(hud == NULL || hud_hidden){
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
    int ty = y, tx = x;
    // first, though, verify that we're clicking within the hud
    if(!ncplane_translate_abs(hud, &ty, &tx)){
      return -1;
    }
    hud_grab_x = x;
    hud_grab_y = y;
    ncplane_yx(hud, &hud_pos_y, &hud_pos_x);
    ret = 0;
  }
  return ret;
}

int hud_release(void){
  if(hud == NULL){
    return -1;
  }
  if(hud_grab_x < 0 && hud_grab_y < 0){
    return -1;
  }
  hud_grab_x = -1;
  hud_grab_y = -1;
  return hud_standard_bg(hud);
}

int fpsplot_release(void){
  if(plot == NULL){
    return -1;
  }
  if(plot_grab_y < 0){
    return -1;
  }
  plot_grab_y = -1;
  return hud_standard_bg(hud);
}

// currently running demo is always at y = HUD_ROWS-2
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
  //  * cure is ready to write to, and print at y = HUD_ROWS - 2
  //  * hooks is ready to enqueue cure to
  //  * reused entries have been printed, if any exist
  if(line <= 0){
    cure = elems;
    elems = cure->next;
    line = 1;
    free(cure->name);
  }else{
    --writeline;
    cure = malloc(sizeof(*cure));
  }
  elem** hook = hud_print_finished(&line);
  if(hook == NULL){
    free(cure);
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
    ncplane_set_fg_alpha(hud, CELL_ALPHA_BLEND);
    ncplane_set_fg(hud, 0);
    ncplane_set_bg_alpha(hud, CELL_ALPHA_BLEND);
    ncplane_set_bg(hud, 0);
    if(ncplane_printf_yx(hud, line, 1, "%-6d %*ju.%02jus %-*.*s", cure->frames,
                        NSLEN - 3, cure->totalns / GIG,
                        (cure->totalns % GIG) / (GIG / 100),
                        PLEN, PLEN, cure->name) < 0){
      return -1;
    }
  }
  return 0;
}

// wake up every 100ms and render a frame so the HUD doesn't appear locked up.
// provide an absolute deadline calculated via CLOCK_MONOTONIC.
static int
demo_nanosleep_abstime_ns(struct notcurses* nc, uint64_t deadline){
  struct timespec fsleep;
  struct timespec now;

  clock_gettime(CLOCK_MONOTONIC, &now);
  while(deadline > timespec_to_ns(&now)){
    fsleep.tv_sec = 0;
    fsleep.tv_nsec = GIG / 10;
    if(deadline - timespec_to_ns(&now) < GIG / 10){
      fsleep.tv_nsec = deadline - timespec_to_ns(&now);
    }
    ncinput ni;
    // throw away any input we receive. if it was for the menu or HUD, it was
    // already dispatched internally to demo_getc().
    char32_t id;
    if((id = demo_getc(nc, &fsleep, &ni)) > 0){
      return -1;
    }
    if(hud){
      int r = demo_render(nc);
      if(r){
        return r;
      }
    }
    clock_gettime(CLOCK_MONOTONIC, &now);
  }
  return 0;
}

int demo_nanosleep(struct notcurses* nc, const struct timespec *ts){
  uint64_t deadline;
  struct timespec now;
  uint64_t nstotal = timespec_to_ns(ts);
  clock_gettime(CLOCK_MONOTONIC, &now);
  deadline = timespec_to_ns(&now) + nstotal;
  return demo_nanosleep_abstime_ns(nc, deadline);
}

int demo_nanosleep_abstime(struct notcurses* nc, const struct timespec* abstime){
  return demo_nanosleep_abstime_ns(nc, timespec_to_ns(abstime));
}

// FIXME needs to pass back any ncinput read, if requested...hrmmm
int demo_render(struct notcurses* nc){
  if(interrupted){
    return 1;
  }
  if(about){
    ncplane_move_top(about);
  }
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  if(plot){
    if(!plot_hidden){
      ncplane_move_top(ncuplot_plane(plot));
    }
    uint64_t ns = (timespec_to_ns(&ts) - plottimestart) / GIG;
    ncuplot_add_sample(plot, ns, 1);
  }
  if(menu){
    ncplane_move_top(ncmenu_plane(menu));
  }
  if(hud){
    const int plen = HUD_COLS - 12 - NSLEN;
    if(!hud_hidden){
      ncplane_move_top(hud);
    }
    uint64_t ns = timespec_to_ns(&ts) - running->startns;
    ++running->frames;
    cell c = CELL_TRIVIAL_INITIALIZER;
    ncplane_base(hud, &c);
    ncplane_set_bg(hud, cell_bg(&c));
    ncplane_set_bg_alpha(hud, CELL_ALPHA_BLEND);
    ncplane_set_fg(hud, 0xffffff);
    cell_release(hud, &c);
    if(ncplane_printf_yx(hud, HUD_ROWS - 2, 1, "%-6d %*ju.%02jus %-*.*s",
                         running->frames,
                         NSLEN - 3, ns / GIG, (ns % GIG) / (GIG / 100),
                         plen, plen, running->name) < 0){
      return -1;
    }
  }
  ncinput ni;
  char32_t id;
  id = demo_getc_nblock(nc, &ni);
  int ret = notcurses_render(nc);
  if(ret){
    return ret;
  }
  if(id == 'q'){
    return 1;
  }
  return 0;
}

int fpsgraph_init(struct notcurses* nc){
  const int PLOTHEIGHT = 6;
  int dimy, dimx;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  struct ncplane* newp = ncplane_new(nc, PLOTHEIGHT, dimx, dimy - PLOTHEIGHT, 0, NULL);
  uint32_t attrword = 0;
  uint64_t channels = 0;
  channels_set_fg_alpha(&channels, CELL_ALPHA_BLEND);
  channels_set_fg(&channels, 0x201020);
  channels_set_bg_alpha(&channels, CELL_ALPHA_BLEND);
  channels_set_bg(&channels, 0x201020);
  ncplane_set_base(newp, "", attrword, channels);
  ncplot_options opts;
  memset(&opts, 0, sizeof(opts));
  opts.flags = NCPLOT_OPTION_LABELTICKSD | NCPLOT_OPTION_EXPONENTIALD;
  channels_set_fg_rgb(&opts.minchannel, 0xff, 0x00, 0xff);
  channels_set_bg(&opts.minchannel, 0x201020);
  channels_set_bg_alpha(&opts.minchannel, CELL_ALPHA_BLEND);
  channels_set_fg_rgb(&opts.maxchannel, 0x00, 0xff, 0x00);
  channels_set_bg(&opts.maxchannel, 0x201020);
  channels_set_bg_alpha(&opts.maxchannel, CELL_ALPHA_BLEND);
  struct ncuplot* fpsplot = ncuplot_create(newp, &opts, 0, 0);
  if(!fpsplot){
    ncplane_destroy(newp);
    return EXIT_FAILURE;
  }
  plot = fpsplot;
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  plottimestart = timespec_to_ns(&ts);
  if(plot_hidden){
    ncplane_move_bottom(newp);
  }
  return 0;
}

int fpsgraph_stop(struct notcurses* nc){
  if(plot){
    ncuplot_destroy(plot);
    plot = NULL;
    notcurses_render(nc);
  }
  return 0;
}

// mouse has maybe pressed on the plot. the caller is responsible for rerendering.
int fpsplot_grab(int y){
  int ret;
  if(plot == NULL || plot_hidden){
    return -1;
  }
  // are we in the middle of a grab?
  if(plot_grab_y >= 0){
    int delty = y - plot_grab_y;
    ret = ncplane_move_yx(ncuplot_plane(plot), plot_pos_y + delty, 0);
  }else{
    // new grab. stash point of original grab, and location of plot at original
    // grab. any delta while grabbed (relative to the original grab point)
    // will see the plot moved by delta (relative to the original plot location).
    int ty = y;
    // first, though, verify that we're clicking within the plot
    if(!ncplane_translate_abs(ncuplot_plane(plot), &ty, NULL)){
      return -1;
    }
    plot_grab_y = y;
    ncplane_yx(ncuplot_plane(plot), &plot_pos_y, NULL);
    ret = 0;
  }
  return ret;
}
