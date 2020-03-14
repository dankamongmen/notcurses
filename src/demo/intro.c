#include "demo.h"

static int
fader(struct notcurses* nc, struct ncplane* ncp, void* curry){
  int* flipmode = curry;
  int rows, cols;
  ncplane_dim_yx(ncp, &rows, &cols);
  for(int x = 5 ; x < cols - 6 ; ++x){
    ncplane_set_fg_rgb(ncp, 0xd0, 0xf0, 0xd0);
    if(ncplane_putwc_yx(ncp, rows - 3, x, x % 2 == *flipmode % 2 ? L'◪' : L'◩') <= 0){
      return -1;
    }
  }
  ++*flipmode;
  int err;
  if( (err = demo_render(nc)) ){
    return err;
  }
  return 0;
}

int intro(struct notcurses* nc){
  int rows, cols;
  struct ncplane* ncp = notcurses_stddim_yx(nc, &rows, &cols);
  uint32_t ccul, ccur, ccll, cclr;
  ccul = ccur = ccll = cclr = 0;
  channel_set_rgb(&ccul, 0, 0xd0, 0);
  channel_set_rgb(&ccur, 0xff, 0, 0);
  channel_set_rgb(&ccll, 0x88, 0, 0xcc);
  channel_set_rgb(&cclr, 0, 0, 0);
  // we use full block rather+fg than space+bg to conflict less with the menu
  if(ncplane_cursor_move_yx(ncp, 0, 0)){
    return -1;
  }
  if(ncplane_highgradient_sized(ncp, ccul, ccur, ccll, cclr, rows, cols)){
    return -1;
  }
  cell c = CELL_TRIVIAL_INITIALIZER;
  cell_set_bg_rgb(&c, 0x20, 0x20, 0x20);
  ncplane_set_base_cell(ncp, &c);
  cell ul = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
  cell ll = CELL_TRIVIAL_INITIALIZER, lr = CELL_TRIVIAL_INITIALIZER;
  cell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
  if(ncplane_cursor_move_yx(ncp, 1, 0)){
    return -1;
  }
  if(cells_rounded_box(ncp, NCSTYLE_BOLD, 0, &ul, &ur, &ll, &lr, &hl, &vl)){
    return -1;
  }
  cell_set_fg(&ul, 0xff0000); cell_set_bg(&ul, 0x002000);
  cell_set_fg(&ur, 0x00ff00); cell_set_bg(&ur, 0x002000);
  cell_set_fg(&ll, 0x0000ff); cell_set_bg(&ll, 0x002000);
  cell_set_fg(&lr, 0xffffff); cell_set_bg(&lr, 0x002000);
  if(ncplane_box_sized(ncp, &ul, &ur, &ll, &lr, &hl, &vl, rows - 1, cols,
                       NCBOXGRAD_TOP | NCBOXGRAD_BOTTOM |
                        NCBOXGRAD_RIGHT | NCBOXGRAD_LEFT)){
    return -1;
  }
  uint64_t cul, cur, cll, clr;
  cul = cur = cll = clr = 0;
  channels_set_fg_rgb(&cul, 200, 0, 200); channels_set_bg_rgb(&cul, 0, 64, 0);
  channels_set_fg_rgb(&cur, 200, 0, 200); channels_set_bg_rgb(&cur, 0, 64, 0);
  channels_set_fg_rgb(&cll, 200, 0, 200); channels_set_bg_rgb(&cll, 0, 128, 0);
  channels_set_fg_rgb(&clr, 200, 0, 200); channels_set_bg_rgb(&clr, 0, 128, 0);
  int centercols = cols > 80 ? 72 : cols - 8;
  if(ncplane_cursor_move_yx(ncp, 5, (cols - centercols) / 2 + 1)){
    return -1;
  }
  if(ncplane_gradient(ncp, "Δ", 0, cul, cur, cll, clr, rows - 8, cols / 2 + centercols / 2 - 1)){
    return -1;
  }
  cell_set_fg(&lr, 0xff0000); cell_set_bg(&lr, 0x002000);
  cell_set_fg(&ll, 0x00ff00); cell_set_bg(&ll, 0x002000);
  cell_set_fg(&ur, 0x0000ff); cell_set_bg(&ur, 0x002000);
  cell_set_fg(&ul, 0xffffff); cell_set_bg(&ul, 0x002000);
  if(ncplane_cursor_move_yx(ncp, 4, (cols - centercols) / 2)){
    return -1;
  }
  if(ncplane_box_sized(ncp, &ul, &ur, &ll, &lr, &hl, &vl, rows - 11, centercols,
                       NCBOXGRAD_TOP | NCBOXGRAD_BOTTOM | NCBOXGRAD_RIGHT | NCBOXGRAD_LEFT)){
    return -1;
  }
  cell_release(ncp, &ul); cell_release(ncp, &ur);
  cell_release(ncp, &ll); cell_release(ncp, &lr);
  cell_release(ncp, &hl); cell_release(ncp, &vl);
  const char s1[] = " Die Welt ist alles, was der Fall ist. ";
  const char str[] = " Wovon man nicht sprechen kann, darüber muss man schweigen. ";
  if(ncplane_set_fg_rgb(ncp, 192, 192, 192)){
    return -1;
  }
  if(ncplane_set_bg_rgb(ncp, 0, 40, 0)){
    return -1;
  }
  if(ncplane_putstr_aligned(ncp, rows / 2 - 2, NCALIGN_CENTER, s1) != (int)strlen(s1)){
    return -1;
  }
  ncplane_styles_on(ncp, NCSTYLE_ITALIC | NCSTYLE_BOLD);
  if(ncplane_putstr_aligned(ncp, rows / 2, NCALIGN_CENTER, str) != (int)strlen(str)){
    return -1;
  }
  ncplane_styles_off(ncp, NCSTYLE_ITALIC);
  // FIXME don't blow away the background color from the gradient. make this a transplane
  ncplane_set_fg_rgb(ncp, 0xff, 0xff, 0xff);
  if(ncplane_printf_aligned(ncp, rows - 5, NCALIGN_CENTER, "notcurses %s. press 'q' to quit.", notcurses_version()) < 0){
    return -1;
  }
  ncplane_styles_off(ncp, NCSTYLE_BOLD);
  const wchar_t wstr[] = L"▏▁ ▂ ▃ ▄ ▅ ▆ ▇ █ █ ▇ ▆ ▅ ▄ ▃ ▂ ▁▕";
  if(ncplane_putwstr_aligned(ncp, rows / 2 - 5, NCALIGN_CENTER, wstr) < 0){
    return -1;
  }
  if(rows < 45){
    ncplane_set_fg_rgb(ncp, 0xc0, 0, 0x80);
    ncplane_set_bg_rgb(ncp, 0x20, 0x20, 0x20);
    ncplane_styles_on(ncp, NCSTYLE_BLINK); // heh FIXME replace with pulse
    if(ncplane_putstr_aligned(ncp, 2, NCALIGN_CENTER, "demo runs best with at least 45 lines") < 0){
      return -1;
    }
    ncplane_styles_off(ncp, NCSTYLE_BLINK); // heh FIXME replace with pulse
  }
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC_RAW, &now);
  uint64_t deadline = timespec_to_ns(&now) + timespec_to_ns(&demodelay) * 2;
  int flipmode = 0;
  do{
    int err;
    if( (err = fader(nc, ncp, &flipmode)) ){
      return err;
    }
    struct timespec iter;
    timespec_div(&demodelay, 10, &iter);
    demo_nanosleep(nc, &iter);
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
  }while(timespec_to_ns(&now) < deadline);
  struct timespec fade = demodelay;
  return ncplane_fadeout(ncp, &fade, demo_fader, NULL);
}
