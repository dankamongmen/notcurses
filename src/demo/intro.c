#include "demo.h"

static int centercols;

static int
animate(struct notcurses* nc, struct ncplane* ncp, void* curry){
  int* flipmode = curry;
  unsigned rows, cols;
  ncplane_dim_yx(ncp, &rows, &cols);
  const bool smallscreen = rows < 26;
  const int row1 = rows - 10 + smallscreen;
  const int row2 = 6 - smallscreen; // box always starts on 4; don't stomp riser
  int startx = (cols - (centercols - 2)) / 2;
  ncplane_set_fg_rgb8(ncp, 0xd0, 0xf0, 0xd0);
  for(int x = startx ; x < startx + centercols - 2 ; ++x){
    if(ncplane_putwc_yx(ncp, row1, x, x % 2 != *flipmode % 2 ? L'◪' : L'◩') <= 0){
      // don't fail out
    }
    if(ncplane_putwc_yx(ncp, row2, x, x % 2 == *flipmode % 2 ? L'◪' : L'◩') <= 0){
      // don't fail out
    }
  }
  ++*flipmode;
  int err;
  if( (err = demo_render(nc)) ){
    return err;
  }
  return 0;
}

static struct ncplane*
orcashow(struct notcurses* nc, unsigned dimy, unsigned dimx){
  char* path = find_data("natasha-blur.png");
  if(path == NULL){
    return NULL;
  }
  struct ncvisual* ncv = ncvisual_from_file(path);
  free(path);
  if(ncv == NULL){
    return NULL;
  }
  struct ncvisual_options vopts = {
    .blitter = NCBLIT_PIXEL,
    .scaling = NCSCALE_STRETCH,
  };
  ncvgeom geom;
  ncvisual_geom(nc, ncv, &vopts, &geom);
  struct ncplane_options nopts = {
    .rows = geom.rcelly > dimy / 2 ? dimy / 2 : geom.rcelly,
    .cols = geom.rcellx > dimx / 4 ? dimx / 4 : geom.rcellx,
    .name = "orca",
  };
  nopts.y = dimy - nopts.rows - 1;
  nopts.x = dimx - nopts.cols - 1;
  struct ncplane* n = ncplane_create(notcurses_stdplane(nc), &nopts);
  if(n == NULL){
    ncvisual_destroy(ncv);
    return NULL;
  }
  vopts.n = n;
  if(ncvisual_blit(nc, ncv, &vopts) == NULL){
    ncplane_destroy(n);
    ncvisual_destroy(ncv);
    return NULL;
  }
  ncvisual_destroy(ncv);
  return n;
}

static int
orcaride(struct notcurses* nc, struct ncplane* on, int iterations){
  unsigned odimy, odimx, dimx;
  int oy, ox;
  ncplane_dim_yx(notcurses_stdplane(nc), NULL, &dimx);
  ncplane_yx(on, &oy, &ox);
  ncplane_dim_yx(on, &odimy, &odimx);
  ox -= ncplane_dim_x(notcurses_stdplane(nc)) / iterations;
  if(ox < 1){
    ox = 1;
  }
  if(ncplane_move_yx(on, oy, ox)){
    return -1;
  }
//fprintf(stderr, "ON THE MOVE (%d/%d through %d/%d)\n", ncplane_y(on), ncplane_x(on), ncplane_y(on) + ncplane_dim_y(on) - 1, ncplane_x(on) + ncplane_dim_x(on) - 1);
  DEMO_RENDER(nc);
  return 0;
}

int intro_demo(struct notcurses* nc, uint64_t startns){
  (void)startns;
  if(!notcurses_canutf8(nc)){
    return 0;
  }
  unsigned rows, cols;
  struct ncplane* ncp = notcurses_stddim_yx(nc, &rows, &cols);
  uint32_t ccul, ccur, ccll, cclr;
  ccul = ccur = ccll = cclr = 0;
  ncchannel_set_rgb8(&ccul, 0, 0xff, 0xff);
  ncchannel_set_rgb8(&ccur, 0xff, 0xff, 0);
  ncchannel_set_rgb8(&ccll, 0xff, 0, 0);
  ncchannel_set_rgb8(&cclr, 0, 0, 0xff);
  // we use full block rather+fg than space+bg to conflict less with the menu
  ncplane_cursor_move_yx(ncp, 2, 1);
  if(ncplane_gradient2x1(ncp, -1, -1, rows - 3, cols - 2, ccul, ccur, ccll, cclr) <= 0){
    return -1;
  }
  nccell c = NCCELL_TRIVIAL_INITIALIZER;
  nccell_set_bg_rgb8(&c, 0x20, 0x20, 0x20);
  ncplane_set_base_cell(ncp, &c);
  nccell ul = NCCELL_TRIVIAL_INITIALIZER, ur = NCCELL_TRIVIAL_INITIALIZER;
  nccell ll = NCCELL_TRIVIAL_INITIALIZER, lr = NCCELL_TRIVIAL_INITIALIZER;
  nccell hl = NCCELL_TRIVIAL_INITIALIZER, vl = NCCELL_TRIVIAL_INITIALIZER;
  if(ncplane_cursor_move_yx(ncp, 1, 0)){
    return -1;
  }
  if(nccells_rounded_box(ncp, NCSTYLE_BOLD, 0, &ul, &ur, &ll, &lr, &hl, &vl)){
    return -1;
  }
  nccell_set_fg_rgb(&ul, 0xff0000); nccell_set_bg_rgb(&ul, 0x002000);
  nccell_set_fg_rgb(&ur, 0x00ff00); nccell_set_bg_rgb(&ur, 0x002000);
  nccell_set_fg_rgb(&ll, 0x0000ff); nccell_set_bg_rgb(&ll, 0x002000);
  nccell_set_fg_rgb(&lr, 0xffffff); nccell_set_bg_rgb(&lr, 0x002000);
  if(ncplane_box_sized(ncp, &ul, &ur, &ll, &lr, &hl, &vl, rows - 1, cols,
                       NCBOXGRAD_TOP | NCBOXGRAD_BOTTOM |
                        NCBOXGRAD_RIGHT | NCBOXGRAD_LEFT)){
    return -1;
  }
  uint64_t cul, cur, cll, clr;
  cul = cur = cll = clr = 0;
  ncchannels_set_fg_rgb8(&cul, 200, 0, 200); ncchannels_set_bg_rgb8(&cul, 0, 64, 0);
  ncchannels_set_fg_rgb8(&cur, 200, 0, 200); ncchannels_set_bg_rgb8(&cur, 0, 64, 0);
  ncchannels_set_fg_rgb8(&cll, 200, 0, 200); ncchannels_set_bg_rgb8(&cll, 0, 128, 0);
  ncchannels_set_fg_rgb8(&clr, 200, 0, 200); ncchannels_set_bg_rgb8(&clr, 0, 128, 0);
  centercols = cols > 80 ? 72 : cols - 8;
  if(ncplane_cursor_move_yx(ncp, 5, (cols - centercols) / 2 + 1)){
    return -1;
  }
  if(ncplane_gradient(ncp, -1, -1, rows - 8 - 5,
                      cols / 2 + centercols / 2 - 1 - ((cols - centercols) / 2 + 1),
                      "Δ", 0, cul, cur, cll, clr) <= 0){
    return -1;
  }
  nccell_set_fg_rgb(&lr, 0xff0000); nccell_set_bg_rgb(&lr, 0x002000);
  nccell_set_fg_rgb(&ll, 0x00ff00); nccell_set_bg_rgb(&ll, 0x002000);
  nccell_set_fg_rgb(&ur, 0x0000ff); nccell_set_bg_rgb(&ur, 0x002000);
  nccell_set_fg_rgb(&ul, 0xffffff); nccell_set_bg_rgb(&ul, 0x002000);
  if(ncplane_cursor_move_yx(ncp, 4, (cols - centercols) / 2)){
    return -1;
  }
  if(ncplane_box_sized(ncp, &ul, &ur, &ll, &lr, &hl, &vl, rows - 11, centercols,
                       NCBOXGRAD_TOP | NCBOXGRAD_BOTTOM | NCBOXGRAD_RIGHT | NCBOXGRAD_LEFT)){
    return -1;
  }
  nccell_release(ncp, &ul); nccell_release(ncp, &ur);
  nccell_release(ncp, &ll); nccell_release(ncp, &lr);
  nccell_release(ncp, &hl); nccell_release(ncp, &vl);
  const char s1[] = " Die Welt ist alles, was der Fall ist. ";
  const char str[] = " Wovon man nicht sprechen kann, darüber muss man schweigen. ";
  if(ncplane_set_fg_rgb8(ncp, 192, 192, 192)){
    return -1;
  }
  if(ncplane_set_bg_rgb8(ncp, 0, 40, 0)){
    return -1;
  }
  if(ncplane_putstr_aligned(ncp, rows / 2 - 4, NCALIGN_CENTER, s1) < 0){
    return -1;
  }
  ncplane_on_styles(ncp, NCSTYLE_ITALIC | NCSTYLE_BOLD);
  if(ncplane_putstr_aligned(ncp, rows / 2 - 3, NCALIGN_CENTER, str) < 0){
    return -1;
  }
  ncplane_off_styles(ncp, NCSTYLE_ITALIC);
  ncplane_set_fg_rgb8(ncp, 0xff, 0xff, 0xff);
  int major, minor, patch, tweak;
  notcurses_version_components(&major, &minor, &patch, &tweak);
  if(tweak){
    if(ncplane_printf_aligned(ncp, rows / 2 - 1, NCALIGN_CENTER, "notcurses %d.%d.%d.%d. press 'q' to quit.",
                              major, minor, patch, tweak) < 0){
      return -1;
    }
  }else{
    if(ncplane_printf_aligned(ncp, rows / 2 - 1, NCALIGN_CENTER, "notcurses %d.%d.%d. press 'q' to quit.",
                              major, minor, patch) < 0){
      return -1;
    }
  }
  ncplane_off_styles(ncp, NCSTYLE_BOLD);
  const wchar_t nwstr[] = L"▏▁ ▂ ▃ ▄ ▅ ▆ ▇ █ █ ▇ ▆ ▅ ▄ ▃ ▂ ▁▕";
  if(ncplane_putwstr_aligned(ncp, rows / 2 - 6, NCALIGN_CENTER, nwstr) < 0){
    // don't fail out
  }
  const wchar_t* iwstr;
  if(notcurses_cansextant(nc)){
    iwstr = L"▏▔ 🮂 🮃 ▀ 🮄 🮅 🮆 █ █ 🮆 🮅 🮄 ▀ 🮃 🮂 ▔▕";
  }else{
    iwstr = L"▏█ ▇ ▆ ▅ ▄ ▃ ▂ ▁ ▁ ▂ ▃ ▄ ▅ ▆ ▇ █▕";
  }
  if(ncplane_putwstr_aligned(ncp, rows / 2 + 1, NCALIGN_CENTER, iwstr) < 0){
    // don't fail out
  }
  if(rows < 45){
    ncplane_set_fg_rgb8(ncp, 0xc0, 0x80, 0x80);
    ncplane_set_bg_rgb8(ncp, 0x20, 0x20, 0x20);
    ncplane_on_styles(ncp, NCSTYLE_BOLD); // FIXME maybe use pulse?
    if(ncplane_putstr_aligned(ncp, 2, NCALIGN_CENTER, "demo runs best with at least 45 lines") < 0){
      return -1;
    }
    ncplane_off_styles(ncp, NCSTYLE_BOLD);
  }
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  // there ought be 20 iterations
  const int expected_iter = 20;
  uint64_t deadline = timespec_to_ns(&now) + timespec_to_ns(&demodelay) * 2;
  struct timespec iter;
  timespec_div(&demodelay, 10, &iter);
  int flipmode = 0;
  struct ncplane* on = NULL;
  do{
    demo_nanosleep(nc, &iter);
    clock_gettime(CLOCK_MONOTONIC, &now);
    int err;
    if(!on){
      if(notcurses_canopen_images(nc)){
        if((on = orcashow(nc, rows, cols)) == NULL){
          return -1;
        }
      }
    }else{
      if((err = orcaride(nc, on, expected_iter))){
        return err;
      }
    }
    if( (err = animate(nc, ncp, &flipmode)) ){
      return err;
    }
  }while(timespec_to_ns(&now) < deadline || flipmode < expected_iter);
  ncplane_destroy(on);
  if(notcurses_canfade(nc)){
    struct timespec fade = demodelay;
    int err;
    if( (err = ncplane_fadeout(ncp, &fade, demo_fader, NULL)) ){
      return err;
    }
  }
  return 0;
}
