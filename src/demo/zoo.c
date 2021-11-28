#include "demo.h"

// open up changes.jpg, stretch it to fill, drop it to greyscale
static int
draw_background(struct notcurses* nc, struct ncplane** bgp){
  *bgp = NULL;
  if(!notcurses_canopen_images(nc)){
    return 0;
  }
  struct ncplane* n = notcurses_stdplane(nc);
  char* path = find_data("changes.jpg");
  struct ncvisual* ncv = ncvisual_from_file(path);
  free(path);
  if(!ncv){
    return -1;
  }
  struct ncvisual_options vopts = {
    .scaling = NCSCALE_STRETCH,
    .n = n,
    .flags = NCVISUAL_OPTION_CHILDPLANE,
  };
  if((*bgp = ncvisual_blit(nc, ncv, &vopts)) == NULL){
    ncvisual_destroy(ncv);
    return -1;
  }
  ncplane_greyscale(n);
  ncvisual_destroy(ncv);
  return 0;
}

// we list all distributions on which notcurses is known to exist
static struct ncselector_item select_items[] = {
#define SITEM(short, long) { short, long, }
  SITEM("fbsd", "FreeBSD"),
  SITEM("deb", "Debian Unstable Linux"),
  SITEM("rpm", "Fedora Rawhide Linux"),
  SITEM("pac", "Arch Linux"),
  SITEM("apk", "Alpine Edge Linux"),
  SITEM(NULL, NULL),
#undef SITEM
};

static struct ncmselector_item mselect_items[] = {
  { "Pa231", "Protactinium-231 (162kg)", .selected = false, },
  { "U233", "Uranium-233 (15kg)", .selected = false, },
  { "U235", "Uranium-235 (50kg)", .selected = false, },
  { "Np236", "Neptunium-236 (7kg)", .selected = false, },
  { "Np237", "Neptunium-237 (60kg)", .selected = false, },
  { "Pu238", "Plutonium-238 (10kg)", .selected = false, },
  { "Pu239", "Plutonium-239 (10kg)", .selected = false, },
  { "Pu240", "Plutonium-240 (40kg)", .selected = false, },
  { "Pu241", "Plutonium-241 (13kg)", .selected = false, },
  { "Am241", "Americium-241 (100kg)", .selected = false, },
  { "Pu242", "Plutonium-242 (100kg)", .selected = false, },
  { "Am242", "Americium-242 (18kg)", .selected = false, },
  { "Am243", "Americium-243 (155kg)", .selected = false, },
  { "Cm243", "Curium-243 (10kg)", .selected = false, },
  { "Cm244", "Curium-244 (30kg)", .selected = false, },
  { "Cm245", "Curium-245 (13kg)", .selected = false, },
  { "Cm246", "Curium-246 (84kg)", .selected = false, },
  { "Cm247", "Curium-247 (7kg)", .selected = false, },
  { "Bk247", "Berkelium-247 (10kg)", .selected = false, },
  { "Cf249", "Californium-249 (6kg)", .selected = false, },
  { "Cf251", "Californium-251 (9kg)", .selected = false, },
  { NULL, NULL, .selected = false, },
};

static struct ncmultiselector*
multiselector_demo(struct ncplane* n, struct ncplane* under, int y){
  ncmultiselector_options mopts = {
    .maxdisplay = 8,
    .title = "multi-item selector",
    .items = mselect_items,
    .boxchannels = NCCHANNELS_INITIALIZER(0x20, 0xe0, 0xe0, 0x20, 0, 0),
    .opchannels = NCCHANNELS_INITIALIZER(0xe0, 0x80, 0x40, 0, 0, 0),
    .descchannels = NCCHANNELS_INITIALIZER(0x80, 0xe0, 0x40, 0, 0, 0),
    .footchannels = NCCHANNELS_INITIALIZER(0xe0, 0, 0x40, 0x20, 0x20, 0),
    .titlechannels = NCCHANNELS_INITIALIZER(0x80, 0x80, 0xff, 0, 0, 0x20),
  };
  uint64_t bgchannels = NCCHANNELS_INITIALIZER(0, 0x40, 0, 0, 0x40, 0);
  ncchannels_set_fg_alpha(&bgchannels, NCALPHA_BLEND);
  ncchannels_set_bg_alpha(&bgchannels, NCALPHA_BLEND);
  struct ncplane_options nopts = {
    .y = y,
    .x = 0,
    .rows = 1,
    .cols = 1,
    NULL,
    .name = "msel",
    NULL, 0,
  };
  struct ncplane* mseln = ncplane_create(n, &nopts);
  if(mseln == NULL){
    return NULL;
  }
  ncplane_set_base(mseln, "", 0, bgchannels);
  struct ncmultiselector* mselect = ncmultiselector_create(mseln, &mopts);
  if(mselect == NULL){
    return NULL;
  }
  struct ncplane* mplane = ncmultiselector_plane(mselect);
  ncplane_move_below(mplane, under);
  return mselect;
}

static struct ncselector*
selector_demo(struct ncplane* n, struct ncplane* under, int dimx, int y){
  ncselector_options sopts = {
    .title = "single-item selector",
    .items = select_items,
    .defidx = 4,
    .maxdisplay = 3,
    .boxchannels = NCCHANNELS_INITIALIZER(0xe0, 0x20, 0x40, 0x20, 0x20, 0x20),
    .opchannels = NCCHANNELS_INITIALIZER(0xe0, 0x80, 0x40, 0, 0, 0),
    .descchannels = NCCHANNELS_INITIALIZER(0x80, 0xe0, 0x40, 0, 0, 0),
    .footchannels = NCCHANNELS_INITIALIZER(0xe0, 0, 0x40, 0x20, 0, 0),
    .titlechannels = NCCHANNELS_INITIALIZER(0xff, 0xff, 0x80, 0, 0, 0x20),
  };
  uint64_t bgchannels = NCCHANNELS_INITIALIZER(0, 0, 0x40, 0, 0, 0x40);
  ncchannels_set_fg_alpha(&bgchannels, NCALPHA_BLEND);
  ncchannels_set_bg_alpha(&bgchannels, NCALPHA_BLEND);
  struct ncplane_options nopts = {
    .y = y,
    .x = dimx,
    .rows = 1,
    .cols = 1,
    NULL,
    .name = "sel",
    NULL, 0,
  };
  struct ncplane* seln = ncplane_create(n, &nopts);
  if(seln == NULL){
    return NULL;
  }
  ncplane_set_base(seln, "", 0, bgchannels);
  struct ncselector* selector = ncselector_create(seln, &sopts);
  if(selector == NULL){
    return NULL;
  }
  ncplane_move_below(seln, under);
  return selector;
}

// wait one demodelay period, offering input to the multiselector, then fade
// out both widgets (if supported).
static int
reader_post(struct notcurses* nc, struct ncselector* selector, struct ncmultiselector* mselector){
  int ret;
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  uint64_t cur = timespec_to_ns(&ts);
  uint64_t targ = cur + timespec_to_ns(&demodelay);
  do{
    struct timespec rel;
    ns_to_timespec(targ - cur, &rel);
    ncinput ni;
    uint32_t wc = demo_getc(nc, &rel, &ni);
    if(wc == (uint32_t)-1){
      return -1;
    }else if(wc){
      ncmultiselector_offer_input(mselector, &ni);
    }
    clock_gettime(CLOCK_MONOTONIC, &ts);
    cur = timespec_to_ns(&ts);
    if( (ret = demo_render(nc)) ){
      return ret;
    }
  }while(cur < targ);
  struct timespec fadedelay;
  ns_to_timespec(timespec_to_ns(&demodelay) * 2, &fadedelay);
  if(notcurses_canfade(nc)){
    if(ncplane_fadeout(ncselector_plane(selector), &fadedelay, demo_fader, NULL)){
      return -1;
    }
    if(ncplane_fadeout(ncmultiselector_plane(mselector), &fadedelay, demo_fader, NULL)){
      return -1;
    }
  }
  return 0;
}

static int
layout_next_text(struct ncplane* p, const char* text, size_t* textpos){
  size_t towrite = strcspn(text + *textpos, " \t\n");
  towrite += strspn(text + *textpos + towrite, " \t\n");
  if(towrite){
    char* duped = strndup(text + *textpos, towrite);
    size_t bytes;
    if(ncplane_puttext(p, -1, NCALIGN_LEFT, duped, &bytes) < 0 || bytes != strlen(duped)){
      free(duped);
      return -1;
    }
    free(duped);
    *textpos += towrite;
  }
  return 0;
}

static int
run_out_text(struct ncplane* p, const char* text, size_t* textpos,
             const struct timespec* iterdelay){
  while(text[*textpos]){
    int ret = layout_next_text(p, text, textpos);
    if(ret){
      return ret;
    }
    demo_nanosleep(ncplane_notcurses(p), iterdelay);
  }
  return 0;
}

static int
get_word_count(const char* text){
  bool inspace = true;
  int words = 0;
  while(*text){
    if(isspace(*text)){
      if(!inspace){
        ++words;
        inspace = true;
      }
    }else{
      inspace = false;
    }
    ++text;
  }
  return words;
}

// selector moves across to the left; plane moves up halfway to the center
static int
selector_run(struct notcurses* nc, struct ncplane* p, struct ncselector* selector){
  const char text[] =
    "Notcurses provides several widgets to quickly build vivid TUIs.\n"
    "This NCReader widget facilitates free-form text entry complete with readline-style bindings.\n"
    "NCSelector allows a single option to be selected from a list.\n"
    "NCFdplane streams a file descriptor, while NCSubproc spawns a subprocess and streams its output.\n";
  int titers = get_word_count(text);
  int ret = 0;
  unsigned dimy, dimx;
  ncplane_dim_yx(notcurses_stdplane(nc), &dimy, &dimx);
  const int centery = (dimy - ncplane_dim_y(p)) / 2;
  int ry, rx, sy, sx;
  ncplane_yx(p, &ry, &rx);
  ncplane_yx(ncselector_plane(selector), &sy, &sx);
  const int xiters = sx - 2;
  const int yiters = (ry - centery) / 2;
  int iters = yiters > xiters ? yiters : xiters;
  if(titers > iters){
    iters = titers;
  }
  const double eachy = (double)iters / yiters;
  const double eachx = (double)iters / xiters;
  const double eacht = (double)iters / titers;
  int xi = 1;
  int yi = 1;
  int ti = 1;
  struct timespec iterdelay, start;
  timespec_div(&demodelay, iters / 4, &iterdelay);
  size_t textpos = 0;
  clock_gettime(CLOCK_MONOTONIC, &start);
  for(int i = 0 ; i < iters ; ++i){
    if(i == (int)(xi * eachx)){
      if(ncplane_move_yx(ncselector_plane(selector), sy, --sx)){
        return -1;
      }
      ++xi;
    }
    if(i == (int)(yi * eachy)){
      if(ncplane_move_yx(p, --ry, rx)){
        return -1;
      }
      ++yi;
    }
    if(i == (int)(ti * eacht)){
      if( (ret = layout_next_text(p, text, &textpos)) ){
        return ret;
      }
      ++ti;
    }
    struct timespec targettime, now;
    timespec_mul(&iterdelay, i + 1, &targettime);
    const uint64_t deadline_ns = timespec_to_ns(&start) + timespec_to_ns(&targettime);
    clock_gettime(CLOCK_MONOTONIC, &now);
    while(timespec_to_ns(&now) < deadline_ns){
      if( (ret = demo_render(nc)) ){
        return ret;
      }
      struct ncinput ni;
      struct timespec inputtime;
      ns_to_timespec(deadline_ns - timespec_to_ns(&now), &inputtime);
      uint32_t wc = demo_getc(nc, &inputtime, &ni);
      if(wc == (uint32_t)-1){
        return -1;
      }else if(wc){
        ncselector_offer_input(selector, &ni);
      }
      clock_gettime(CLOCK_MONOTONIC, &now);
    }
  }
  return run_out_text(p, text, &textpos, &iterdelay);
}

// selector moves across to the right; plane moves up halfway to the center
static int
mselector_run(struct notcurses* nc, struct ncplane* p, struct ncmultiselector* mselector){
  const char text[] =
    "NCMultiselector allows 0..n options to be selected from a list of n items.\n"
    "A variety of plots are supported. "
    "Menus can be placed along the top and/or bottom of any plane. "
    "Widgets can be controlled with the keyboard and/or mouse. "
    "They are implemented atop ncplanes, and these planes can be manipulated like all others.";
  const int titers = get_word_count(text);
  int ret = 0;
  unsigned dimy, dimx;
  ncplane_dim_yx(notcurses_stdplane(nc), &dimy, &dimx);
  const int centery = (dimy - ncplane_dim_y(p)) / 2;
  int ry, rx, sy, sx;
  ncplane_yx(p, &ry, &rx);
  ncplane_yx(ncmultiselector_plane(mselector), &sy, &sx);
  const int xiters = dimx - ncplane_dim_x(ncmultiselector_plane(mselector));
  const int yiters = ry - centery;
  int iters = yiters > xiters ? yiters : xiters;
  if(titers > iters){
    iters = titers;
  }
  const double eachy = (double)iters / yiters;
  const double eachx = (double)iters / xiters;
  const double eacht = (double)iters / titers;
  int xi = 1;
  int yi = 1;
  int ti = 1;
  struct timespec iterdelay, start;
  timespec_div(&demodelay, iters / 4, &iterdelay);
  clock_gettime(CLOCK_MONOTONIC, &start);
  size_t textpos = 0;
  for(int i = 0 ; i < iters ; ++i){
    if(i == (int)(ti * eacht)){
      if( (ret = layout_next_text(p, text, &textpos)) ){
        return ret;
      }
      ++ti;
    }
    if(i == (int)(xi * eachx)){
      if(ncplane_move_yx(ncmultiselector_plane(mselector), sy, ++sx)){
        return -1;
      }
      ++xi;
    }
    if(i == (int)(yi * eachy)){
      if(ncplane_move_yx(p, --ry, rx)){
        return -1;
      }
      ++yi;
    }
    struct timespec targettime, now;
    timespec_mul(&iterdelay, i + 1, &targettime);
    const uint64_t deadline_ns = timespec_to_ns(&start) + timespec_to_ns(&targettime);
    clock_gettime(CLOCK_MONOTONIC, &now);
    while(timespec_to_ns(&now) < deadline_ns){
      if( (ret = demo_render(nc)) ){
        return ret;
      }
      struct ncinput ni;
      struct timespec inputtime;
      ns_to_timespec(deadline_ns - timespec_to_ns(&now), &inputtime);
      uint32_t wc = demo_getc(nc, &inputtime, &ni);
      if(wc == (uint32_t)-1){
        return -1;
      }else if(wc){
        ncmultiselector_offer_input(mselector, &ni);
      }
      clock_gettime(CLOCK_MONOTONIC, &now);
    }
  }
  return run_out_text(p, text, &textpos, &iterdelay);
}

// creates a plane, an ncselector, and a ncmultiselector, and moves them into
// place. the latter two are then faded out. all three are then destroyed.
static int
reader_demo(struct notcurses* nc){
  int ret = -1;
  unsigned dimy, dimx;
  struct ncplane* std = notcurses_stddim_yx(nc, &dimy, &dimx);
  const int READER_COLS = 64;
  const int READER_ROWS = 8;
  const int x = ncplane_halign(std, NCALIGN_CENTER, READER_COLS);
  struct ncselector* selector = NULL;
  struct ncmultiselector* mselector = NULL;
  struct ncplane_options nopts = {
    .y = dimy,
    .x = x,
    .rows = READER_ROWS,
    .cols = READER_COLS,
    NULL, "read", NULL, 0,
  };
  struct ncplane* rp = ncplane_create(std, &nopts);
  if(rp == NULL){
    goto done;
  }
  ncplane_set_fg_rgb8(rp, 0x20, 0xe0, 0xe0);
  uint64_t echannels = 0;
  ncchannels_set_fg_alpha(&echannels, NCALPHA_BLEND);
  ncchannels_set_bg_alpha(&echannels, NCALPHA_BLEND);
  ncplane_set_base(rp, "", 0, echannels);
  ncplane_set_scrolling(rp, true);
  // Bring the selector left across the top, while raising the exposition
  // halfway to its target height.
  selector = selector_demo(std, rp, dimx, 2);
  if(selector == NULL){
    goto done;
  }
  if( (ret = selector_run(nc, rp, selector)) ){
    goto done;
  }
  // Bring the multiselector right across the top, while raising the exposition
  // the remainder of its path to the center of the screen.
  mselector = multiselector_demo(std, rp, 8);
  if(mselector == NULL){
    goto done;
  }
  if( (ret = mselector_run(nc, rp, mselector)) ){
    goto done;
  }
  // Delay and fade
  if( (ret = reader_post(nc, selector, mselector)) ){
    goto done;
  }

done:
  ncselector_destroy(selector, NULL);
  ncmultiselector_destroy(mselector);
  ncplane_destroy(rp);
  return ret;
}


// a plane with exposition text rises from the bottom to the center of the
// screen. as it does so, two widgets (selector and multiselector) come in
// from the left and right, respectively. they then fade out.
int zoo_demo(struct notcurses* nc, uint64_t startns){
  (void)startns;
  struct ncplane* bgp;
  if(draw_background(nc, &bgp)){
    return -1;
  }
  int ret = reader_demo(nc);
  ncplane_destroy(bgp);
  return ret;
}
