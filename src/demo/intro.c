#include "demo.h"

int intro(struct notcurses* nc){
  struct ncplane* ncp;
  if((ncp = notcurses_stdplane(nc)) == NULL){
    return -1;
  }
  cell c = CELL_TRIVIAL_INITIALIZER;
  cell_set_bg_rgb(&c, 0x20, 0x20, 0x20);
  ncplane_set_base(ncp, &c);
  int x, y, rows, cols;
  ncplane_dim_yx(ncp, &rows, &cols);
  cell ul = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
  cell ll = CELL_TRIVIAL_INITIALIZER, lr = CELL_TRIVIAL_INITIALIZER;
  cell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
  if(ncplane_cursor_move_yx(ncp, 0, 0)){
    return -1;
  }
  if(cells_rounded_box(ncp, CELL_STYLE_BOLD, 0, &ul, &ur, &ll, &lr, &hl, &vl)){
    return -1;
  }
  cell_set_fg(&ul, 0xff0000);
  cell_set_bg(&ul, 0x002000);
  cell_set_fg(&ur, 0x00ff00);
  cell_set_bg(&ur, 0x002000);
  cell_set_fg(&ll, 0x0000ff);
  cell_set_bg(&ll, 0x002000);
  cell_set_fg(&lr, 0xffffff);
  cell_set_bg(&lr, 0x002000);
  if(ncplane_box_sized(ncp, &ul, &ur, &ll, &lr, &hl, &vl, rows, cols,
                       NCBOXGRAD_TOP | NCBOXGRAD_BOTTOM |
                        NCBOXGRAD_RIGHT | NCBOXGRAD_LEFT)){
    return -1;
  }
  cell_release(ncp, &ul); cell_release(ncp, &ur);
  cell_release(ncp, &ll); cell_release(ncp, &lr);
  cell_release(ncp, &hl); cell_release(ncp, &vl);
  const char* cstr = "Δ";
  cell_load(ncp, &c, cstr);
  cell_set_fg_rgb(&c, 200, 0, 200);
  int ys = 200 / (rows - 2);
  for(y = 5 ; y < rows - 6 ; ++y){
    cell_set_bg_rgb(&c, 0, y * ys  , 0);
    for(x = 5 ; x < cols - 6 ; ++x){
      if(ncplane_putc_yx(ncp, y, x, &c) <= 0){
        return -1;
      }
    }
  }
  cell_release(ncp, &c);
  uint64_t channels = 0;
  channels_set_fg_rgb(&channels, 90, 0, 90);
  channels_set_bg_rgb(&channels, 0, 0, 180);
  if(ncplane_cursor_move_yx(ncp, 4, 4)){
    return -1;
  }
  if(ncplane_rounded_box(ncp, 0, channels, rows - 6, cols - 6, 0)){
    return -1;
  }
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
  ncplane_styles_on(ncp, CELL_STYLE_ITALIC | CELL_STYLE_BOLD);
  if(ncplane_putstr_aligned(ncp, rows / 2, NCALIGN_CENTER, str) != (int)strlen(str)){
    return -1;
  }
  ncplane_styles_off(ncp, CELL_STYLE_ITALIC);
  ncplane_set_fg_rgb(ncp, 0xff, 0xff, 0xff);
  if(ncplane_printf_aligned(ncp, rows - 3, NCALIGN_CENTER, "notcurses %s. press 'q' to quit.", notcurses_version()) < 0){
    return -1;
  }
  ncplane_styles_off(ncp, CELL_STYLE_BOLD);
  const wchar_t wstr[] = L"▏▁ ▂ ▃ ▄ ▅ ▆ ▇ █ █ ▇ ▆ ▅ ▄ ▃ ▂ ▁▕";
  if(ncplane_putwstr_aligned(ncp, rows / 2 - 5, NCALIGN_CENTER, wstr) < 0){
    return -1;
  }
  if(rows < 45){
    ncplane_set_fg_rgb(ncp, 0xc0, 0, 0x80);
    ncplane_set_bg_rgb(ncp, 0x20, 0x20, 0x20);
    ncplane_styles_on(ncp, CELL_STYLE_BLINK); // heh FIXME replace with pulse
    if(ncplane_putstr_aligned(ncp, 2, NCALIGN_CENTER, "demo runs best with at least 45 lines") < 0){
      return -1;
    }
    ncplane_styles_off(ncp, CELL_STYLE_BLINK); // heh FIXME replace with pulse
  }
  if(demo_render(nc)){
    return -1;
  }
  nanosleep(&demodelay, NULL);
  struct timespec fade = demodelay;
  ncplane_fadeout(ncp, &fade, demo_fader, NULL);
  return 0;
}
