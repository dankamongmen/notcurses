#include "demo.h"
#include <pthread.h>

// we provide a heads-up display throughout the demo, detailing the demos we're
// about to run, running, and just runned. the user can move this HUD with
// their mouse. it should always be on the top of the z-stack, unless hidden.
struct ncplane* hud = NULL;
static struct elem* elems;   // tracks the last n demos

static bool hud_hidden;
static bool plot_hidden;
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

#define FPSHZ 2

// how many columns for runtime?
#define HUD_ROWS (3 + 2) // 2 for borders
static const int HUD_COLS = 23 + 2; // 2 for borders

typedef struct elem {
  char* name;
  uint64_t startns;
  uint64_t totalns;
  unsigned frames;
  struct elem* next;
} elem;

static struct ncmenu* menu;
static struct ncplane* about; // "about" modal popup
static struct ncplane* debug; // "debug info" modal popup

#define MENUSTR_TOGGLE_HUD "Toggle HUD"
#define MENUSTR_TOGGLE_PLOT "Toggle FPS plot"
#define MENUSTR_RESTART "Restart"
#define MENUSTR_ABOUT "About"
#define MENUSTR_DEBUG "Debug info"
#define MENUSTR_QUIT "Quit"

static int
hud_standard_bg_rgb(struct ncplane* n){
  uint64_t channels = 0;
  channels_set_fg_alpha(&channels, CELL_ALPHA_BLEND);
  channels_set_fg_rgb8(&channels, 0x80, 0x80, 0x80);
  channels_set_bg_alpha(&channels, CELL_ALPHA_BLEND);
  channels_set_bg_rgb8(&channels, 0x80, 0x80, 0x80);
  if(ncplane_set_base(n, "", 0, channels) >= 0){
    return -1;
  }
  return 0;
}

static int
count_debug_lines(const char* output, size_t outputlen){
  int lines = 0;
  for(size_t i = 0 ; i < outputlen ; ++i){
    if(output[i] == '\n'){
      ++lines;
    }
  }
  return lines;
}

static void
debug_toggle(struct notcurses* nc){
  ncmenu_rollup(menu);
  if(debug){
    ncplane_destroy(debug);
    debug = NULL;
    return;
  }
  int dimy, dimx;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  char* output = NULL;
  size_t outputlen = 0;
  FILE* mstream = open_memstream(&output, &outputlen);
  if(mstream == NULL){
    return;
  }
  notcurses_debug(nc, mstream);
  if(fclose(mstream)){
    return;
  }
  ncplane_options nopts = {
    .y = 3,
    .x = NCALIGN_CENTER,
    .rows = count_debug_lines(output, outputlen) + 1,
    // 1 plus the max len of debug output so it can print against right border
    .cols = 81,
    .flags = NCPLANE_OPTION_HORALIGNED,
  };
  struct ncplane* n = ncplane_create(notcurses_stdplane(nc), &nopts);
  if(n == NULL){
    free(output);
    return;
  }
  uint64_t channels = 0;
  channels_set_fg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
  channels_set_bg_rgb(&channels, 0xffffe5);
  ncplane_set_base(n, " ", 0, channels);
  ncplane_set_scrolling(n, true);
  ncplane_set_fg_rgb(n, 0x0a0a0a);
  ncplane_set_bg_rgb(n, 0xffffe5);
  if(ncplane_puttext(n, 0, NCALIGN_LEFT, output, &outputlen) < 0){
    free(output);
    ncplane_destroy(n);
    return;
  }
  ncplane_putstr_aligned(n, ncplane_dim_y(n) - 1, NCALIGN_CENTER, "Press Alt+d to hide this window");
  free(output);
  debug = n;
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
  ncplane_options nopts = {
    .y = 3,
    .x = NCALIGN_CENTER,
    .rows = ABOUT_ROWS,
    .cols = ABOUT_COLS,
    .flags = NCPLANE_OPTION_HORALIGNED,
  };
  struct ncplane* n = ncplane_create(notcurses_stdplane(nc), &nopts);
  // let the glyphs below show through, but only dimly
  uint64_t channels = 0;
  channels_set_fg_alpha(&channels, CELL_ALPHA_BLEND);
  channels_set_fg_rgb8(&channels, 0x0, 0x0, 0x0);
  channels_set_bg_alpha(&channels, CELL_ALPHA_BLEND);
  channels_set_bg_rgb8(&channels, 0x0, 0x0, 0x0);
  if(ncplane_set_base(n, "", 0, channels) >= 0){
    ncplane_set_fg_rgb(n, 0x11ffff);
    ncplane_set_bg_rgb(n, 0);
    ncplane_set_bg_alpha(n, CELL_ALPHA_BLEND);
    ncplane_printf_aligned(n, 1, NCALIGN_CENTER, "notcurses-demo %s", notcurses_version());
    ncplane_printf_aligned(n, 3, NCALIGN_LEFT, "  P toggle plot");
    ncplane_printf_aligned(n, 3, NCALIGN_RIGHT, "toggle help Ctrl+U  ");
    ncplane_printf_aligned(n, 4, NCALIGN_LEFT, "  H toggle HUD");
    ncplane_printf_aligned(n, 4, NCALIGN_RIGHT, "restart Ctrl+R  ");
    ncplane_printf_aligned(n, 5, NCALIGN_CENTER, "q quit");
    ncplane_putstr_aligned(n, 7, NCALIGN_CENTER, "\u00a9 nick black <nickblack@linux.com>");
    nccell ul = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
    nccell lr = CELL_TRIVIAL_INITIALIZER, ll = CELL_TRIVIAL_INITIALIZER;
    nccell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
    channels = 0;
    channels_set_fg_rgb(&channels, 0xc020c0);
    channels_set_bg_rgb(&channels, 0);
    if(cells_rounded_box(n, NCSTYLE_NONE, channels, &ul, &ur, &ll, &lr, &hl, &vl) == 0){
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

void about_destroy(struct notcurses* nc){
  if(about){
    about_toggle(nc);
  }
}

static void
hud_toggle(struct notcurses* nc){
  ncmenu_rollup(menu);
  if(!hud){
    return;
  }
  hud_hidden = !hud_hidden;
  if(hud_hidden){
    ncplane_reparent(hud, hud);
  }else{
    ncplane_reparent(hud, notcurses_stdplane(nc));
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
    ncplane_reparent(ncuplot_plane(plot), ncuplot_plane(plot));
  }else{
    ncplane_reparent(ncuplot_plane(plot), notcurses_stdplane(nc));
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
  }else if(menu && ni->id == NCKEY_RELEASE){
    const char* sel = ncmenu_mouse_selected(menu, ni, &tmpni);
    if(sel == NULL){
      memcpy(&tmpni, ni, sizeof(tmpni));
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
  if(tmpni.id == 'd' && tmpni.alt && !tmpni.ctrl){
    debug_toggle(nc);
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
    { .desc = MENUSTR_DEBUG, .shortcut = { .id = 'd', .alt = true, }, },
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
  channels_set_fg_rgb(&sectionchannels, 0xffffff);
  channels_set_bg_rgb(&sectionchannels, 0x000000);
  channels_set_fg_alpha(&sectionchannels, CELL_ALPHA_HIGHCONTRAST);
  channels_set_bg_alpha(&sectionchannels, CELL_ALPHA_BLEND);
  channels_set_fg_rgb(&headerchannels, 0xffffff);
  channels_set_bg_rgb(&headerchannels, 0x7f347f);
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

static int
hud_refresh(struct ncplane* n){
  ncplane_erase(n);
  nccell ul = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
  nccell lr = CELL_TRIVIAL_INITIALIZER, ll = CELL_TRIVIAL_INITIALIZER;
  nccell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
  if(cells_rounded_box(n, NCSTYLE_NONE, 0, &ul, &ur, &ll, &lr, &hl, &vl)){
    return -1;
  }
  ul.channels = CHANNELS_RGB_INITIALIZER(0xf0, 0xc0, 0xc0, 0, 0, 0);
  ur.channels = CHANNELS_RGB_INITIALIZER(0xf0, 0xc0, 0xc0, 0, 0, 0);
  ll.channels = CHANNELS_RGB_INITIALIZER(0xf0, 0xc0, 0xc0, 0, 0, 0);
  lr.channels = CHANNELS_RGB_INITIALIZER(0xf0, 0xc0, 0xc0, 0, 0, 0);
  hl.channels = CHANNELS_RGB_INITIALIZER(0xf0, 0xc0, 0xc0, 0, 0, 0);
  vl.channels = CHANNELS_RGB_INITIALIZER(0xf0, 0xc0, 0xc0, 0, 0, 0);
  cell_set_bg_alpha(&ul, CELL_ALPHA_BLEND);
  cell_set_bg_alpha(&ur, CELL_ALPHA_BLEND);
  cell_set_bg_alpha(&ll, CELL_ALPHA_BLEND);
  cell_set_bg_alpha(&lr, CELL_ALPHA_BLEND);
  cell_set_bg_alpha(&hl, CELL_ALPHA_BLEND);
  cell_set_bg_alpha(&vl, CELL_ALPHA_BLEND);
  if(ncplane_perimeter(n, &ul, &ur, &ll, &lr, &hl, &vl, 0)){
    cell_release(n, &ul); cell_release(n, &ur); cell_release(n, &hl);
    cell_release(n, &ll); cell_release(n, &lr); cell_release(n, &vl);
    return -1;
  }
  cell_release(n, &ul); cell_release(n, &ur); cell_release(n, &hl);
  cell_release(n, &ll); cell_release(n, &lr); cell_release(n, &vl);
  return 0;
}

static int
hud_print_finished(elem* list){
  elem* e = list;
  if(hud){
    hud_refresh(hud);
  }
  int line = 0;
  while(e){
    if(++line == HUD_ROWS - 1){
      if(e->next){
        free(e->next->name);
        free(e->next);
        e->next = NULL;
      }
      break;
    }
    if(hud){
      nccell c = CELL_TRIVIAL_INITIALIZER;
      ncplane_base(hud, &c);
      ncplane_set_bg_rgb(hud, cell_bg_rgb(&c));
      ncplane_set_bg_alpha(hud, CELL_ALPHA_BLEND);
      ncplane_set_fg_rgb(hud, 0xffffff);
      ncplane_set_fg_alpha(hud, CELL_ALPHA_OPAQUE);
      cell_release(hud, &c);
      if(ncplane_printf_yx(hud, line, 1, "%d", e->frames) < 0){
        return -1;
      }
      if(ncplane_printf_yx(hud, line, 7, "%ju.%03jus", e->totalns / NANOSECS_IN_SEC,
                           (e->totalns % NANOSECS_IN_SEC) / 1000000) < 0){
        return -1;
      }
      if(ncplane_putstr_yx(hud, line, 16, e->name) < 0){
        return -1;
      }
    }
    e = e->next;
  }
  return 0;
}

struct ncplane* hud_create(struct notcurses* nc){
  if(hud){
    return NULL;
  }
  int dimx, dimy;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  int yoffset = dimy - HUD_ROWS;
  struct ncplane_options nopts = {
    .y = yoffset,
    // we want it to start tucked right up underneath the title of the FPS
    // graph. graph is min(80, dimx) wide, centered, and we want 6 in.
    .x = (dimx - (dimx > 80 ? 80 : dimx)) / 2 + 6,
    .rows = HUD_ROWS,
    .cols = HUD_COLS,
    .userptr = NULL,
    .name = "hud",
    .resizecb = NULL,
    .flags = 0,
  };
  struct ncplane* n = ncplane_create(notcurses_stdplane(nc), &nopts);
  if(n == NULL){
    return NULL;
  }
  hud_standard_bg_rgb(n);
  hud_refresh(n);
  ncplane_set_fg_rgb(n, 0xffffff);
  ncplane_set_bg_rgb(n, 0);
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
  return hud_standard_bg_rgb(hud);
}

int fpsplot_release(void){
  if(plot == NULL){
    return -1;
  }
  if(plot_grab_y < 0){
    return -1;
  }
  plot_grab_y = -1;
  return hud_standard_bg_rgb(hud);
}

// currently running demo is always at y = HUD_ROWS-2
int hud_completion_notify(const demoresult* result){
  if(elems){
    elems->totalns = result->timens;
    elems->frames = result->stats.renders;
  }
  return 0;
}

// inform the HUD of an upcoming demo
int hud_schedule(const char* demoname){
  elem* cure = malloc(sizeof(*cure));
  if(!cure){
    return -1;
  }
  cure->next = elems;
  cure->name = strdup(demoname);
  cure->totalns = 0;
  cure->frames = 0;
  elems = cure;
  struct timespec cur;
  clock_gettime(CLOCK_MONOTONIC, &cur);
  cure->startns = timespec_to_ns(&cur);
  return hud_print_finished(elems);
}

// wake up every 10ms and render a frame so the HUD doesn't appear locked up.
// provide an absolute deadline calculated via CLOCK_MONOTONIC.
static int
demo_nanosleep_abstime_ns(struct notcurses* nc, uint64_t deadline){
  struct timespec fsleep;
  struct timespec now;

  clock_gettime(CLOCK_MONOTONIC, &now);
  while(deadline > timespec_to_ns(&now)){
    fsleep.tv_sec = 0;
    fsleep.tv_nsec = MAXSLEEP;
    if(deadline - timespec_to_ns(&now) < NANOSECS_IN_SEC / 100){
      fsleep.tv_nsec = deadline - timespec_to_ns(&now);
    }
    ncinput ni;
    // throw away any input we receive. if it was for the menu or HUD, it was
    // already dispatched internally to demo_getc(). we need to ensure input
    // is being procesed, however, to drive the demo elements.
    demo_getc(nc, &fsleep, &ni);
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
  if(debug){
    ncplane_move_top(debug);
  }
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  if(plot){
    if(!plot_hidden){
      ncplane_move_top(ncuplot_plane(plot));
    }
    uint64_t ns = (timespec_to_ns(&ts) - plottimestart) / (NANOSECS_IN_SEC / FPSHZ);
    ncuplot_add_sample(plot, ns, 1);
  }
  if(menu){
    ncplane_move_top(ncmenu_plane(menu));
  }
  if(hud){
    if(!hud_hidden){
      ncplane_move_top(hud);
    }
    uint64_t ns = timespec_to_ns(&ts) - elems->startns;
    ++elems->frames;
    nccell c = CELL_TRIVIAL_INITIALIZER;
    ncplane_base(hud, &c);
    ncplane_set_bg_rgb(hud, cell_bg_rgb(&c));
    ncplane_set_bg_alpha(hud, CELL_ALPHA_BLEND);
    ncplane_set_fg_rgb(hud, 0x80d0ff);
    ncplane_set_fg_alpha(hud, CELL_ALPHA_OPAQUE);
    cell_release(hud, &c);
    ncplane_on_styles(hud, NCSTYLE_BOLD);
    if(ncplane_printf_yx(hud, 1, 1, "%d", elems->frames) < 0){
      return -1;
    }
    if(ncplane_printf_yx(hud, 1, 7, "%ju.%03jus", ns / NANOSECS_IN_SEC,
                         (ns % NANOSECS_IN_SEC) / 1000000) < 0){
      return -1;
    }
    if(ncplane_putstr_yx(hud, 1, 16, elems->name) < 0){
      return -1;
    }
    ncplane_off_styles(hud, NCSTYLE_BOLD);
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
  struct ncplane* stdn = notcurses_stddim_yx(nc, &dimy, &dimx);
  ncplane_options nopts = {
    .y = dimy - PLOTHEIGHT,
    .x = NCALIGN_CENTER,
    .rows = PLOTHEIGHT,
    .cols = dimx > 80 ? 80 : dimx,
    .userptr = NULL,
    .name = "fps",
    .resizecb = ncplane_resize_realign,
    .flags = NCPLANE_OPTION_HORALIGNED,
  };
  struct ncplane* newp = ncplane_create(stdn, &nopts);
  if(newp == NULL){
    return -1;
  }
  uint32_t style = 0;
  uint64_t channels = 0;
  channels_set_fg_alpha(&channels, CELL_ALPHA_BLEND);
  channels_set_fg_rgb(&channels, 0x201020);
  channels_set_bg_alpha(&channels, CELL_ALPHA_BLEND);
  channels_set_bg_rgb(&channels, 0x201020);
  ncplane_set_base(newp, "", style, channels);
  ncplot_options opts;
  memset(&opts, 0, sizeof(opts));
  opts.flags = NCPLOT_OPTION_LABELTICKSD |
               NCPLOT_OPTION_EXPONENTIALD |
               NCPLOT_OPTION_PRINTSAMPLE;
  opts.gridtype = NCBLIT_BRAILLE;
  opts.legendstyle = NCSTYLE_ITALIC | NCSTYLE_BOLD;
  opts.title = "frames per semisecond";
  channels_set_fg_rgb8(&opts.minchannels, 0x80, 0x80, 0xff);
  channels_set_bg_rgb(&opts.minchannels, 0x201020);
  channels_set_bg_alpha(&opts.minchannels, CELL_ALPHA_BLEND);
  channels_set_fg_rgb8(&opts.maxchannels, 0x80, 0xff, 0x80);
  channels_set_bg_rgb(&opts.maxchannels, 0x201020);
  channels_set_bg_alpha(&opts.maxchannels, CELL_ALPHA_BLEND);
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

int fpsgraph_stop(void){
  if(plot){
    ncuplot_destroy(plot);
    plot = NULL;
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
    ret = ncplane_move_yx(ncuplot_plane(plot), plot_pos_y + delty, ncplane_x(ncuplot_plane(plot)));
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
