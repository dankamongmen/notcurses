#include <deque>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <clocale>
#include <iostream>
#include <notcurses.h>

static int dimy, dimx;
static struct notcurses* nc;

// return the string version of a special composed key
const char* nckeystr(char32_t spkey){
  switch(spkey){ // FIXME
    case NCKEY_RESIZE:
      notcurses_resize(nc, &dimy, &dimx);
      return "resize event";
    case NCKEY_INVALID: return "invalid";
    case NCKEY_LEFT:    return "left";
    case NCKEY_UP:      return "up";
    case NCKEY_RIGHT:   return "right";
    case NCKEY_DOWN:    return "down";
    case NCKEY_INS:     return "insert";
    case NCKEY_DEL:     return "delete";
    case NCKEY_PGDOWN:  return "pgdown";
    case NCKEY_PGUP:    return "pgup";
    case NCKEY_HOME:    return "home";
    case NCKEY_END:     return "end";
    case NCKEY_F00:     return "F0";
    case NCKEY_F01:     return "F1";
    case NCKEY_F02:     return "F2";
    case NCKEY_F03:     return "F3";
    case NCKEY_F04:     return "F4";
    case NCKEY_F05:     return "F5";
    case NCKEY_F06:     return "F6";
    case NCKEY_F07:     return "F7";
    case NCKEY_F08:     return "F8";
    case NCKEY_F09:     return "F9";
    case NCKEY_F10:     return "F10";
    case NCKEY_F11:     return "F11";
    case NCKEY_F12:     return "F12";
    case NCKEY_F13:     return "F13";
    case NCKEY_F14:     return "F14";
    case NCKEY_F15:     return "F15";
    case NCKEY_F16:     return "F16";
    case NCKEY_F17:     return "F17";
    case NCKEY_F18:     return "F18";
    case NCKEY_F19:     return "F19";
    case NCKEY_F20:     return "F20";
    case NCKEY_F21:     return "F21";
    case NCKEY_F22:     return "F22";
    case NCKEY_F23:     return "F23";
    case NCKEY_F24:     return "F24";
    case NCKEY_F25:     return "F25";
    case NCKEY_F26:     return "F26";
    case NCKEY_F27:     return "F27";
    case NCKEY_F28:     return "F28";
    case NCKEY_F29:     return "F29";
    case NCKEY_F30:     return "F30";
    case NCKEY_BACKSPACE: return "backspace";
    case NCKEY_CENTER:  return "center";
    case NCKEY_ENTER:   return "enter";
    case NCKEY_CLS:     return "clear";
    case NCKEY_DLEFT:   return "down+left";
    case NCKEY_DRIGHT:  return "down+right";
    case NCKEY_ULEFT:   return "up+left";
    case NCKEY_URIGHT:  return "up+right";
    case NCKEY_BEGIN:   return "begin";
    case NCKEY_CANCEL:  return "cancel";
    case NCKEY_CLOSE:   return "close";
    case NCKEY_COMMAND: return "command";
    case NCKEY_COPY:    return "copy";
    case NCKEY_EXIT:    return "exit";
    case NCKEY_PRINT:   return "print";
    case NCKEY_REFRESH: return "refresh";
    default:            return "unknown";
  }
}

// Print the utf8 Control Pictures for otherwise unprintable ASCII
char32_t printutf8(char32_t kp){
  if(kp <= 27 && kp < UINT_LEAST32_MAX){
    return 0x2400 + kp;
  }
  return kp;
}

// Dim all text on the plane by the same amount. This will stack for
// older text, and thus clearly indicate the current output.
static int
dim_rows(struct ncplane* n){
  int y, x;
  cell c = CELL_TRIVIAL_INITIALIZER;
  for(y = 2 ; y < dimy ; ++y){
    for(x = 0 ; x < dimx ; ++x){
      if(ncplane_at_yx(n, y, x, &c) < 0){
        cell_release(n, &c);
        return -1;
      }
      unsigned r, g, b;
      cell_get_fg_rgb(&c, &r, &g, &b);
      r -= r / 32;
      g -= g / 32;
      b -= b / 32;
      if(r > 247){ r = 0; }
      if(g > 247){ g = 0; }
      if(b > 247){ b = 0; }
      if(cell_set_fg_rgb(&c, r, g, b)){
        cell_release(n, &c);
        return -1;
      }
      if(ncplane_putc_yx(n, y, x, &c) < 0){
        cell_release(n, &c);
        return -1;
      }
      if(cell_double_wide_p(&c)){
        ++x;
      }
    }
  }
  cell_release(n, &c);
  return 0;
}

int main(void){
  if(setlocale(LC_ALL, "") == nullptr){
    return EXIT_FAILURE;
  }
  notcurses_options opts{};
  opts.clear_screen_start = true;
  if((nc = notcurses_init(&opts, stdout)) == nullptr){
    return EXIT_FAILURE;;
  }
  struct ncplane* n = notcurses_stdplane(nc);
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  ncplane_set_fg(n, 0);
  ncplane_set_bg(n, 0xbb64bb);
  ncplane_styles_set(n, CELL_STYLE_UNDERLINE);
  if(ncplane_putstr_aligned(n, 0, "mash some keys, yo. give that mouse some waggle!", NCALIGN_CENTER) <= 0){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  ncplane_styles_off(n, CELL_STYLE_UNDERLINE);
  ncplane_set_bg_default(n);
  notcurses_render(nc);
  int y = 2;
  std::deque<char32_t> cells;
  char32_t r;
  if(notcurses_mouse_enable(nc)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  while(errno = 0, (r = notcurses_getc_blocking(nc)) < UINT_LEAST32_MAX){
    if(r == 0){ // interrupted by signal
      continue;
    }
    if(ncplane_cursor_move_yx(n, y, 0)){
      break;
    }
    if(r < 0x80){
      ncplane_set_fg_rgb(n, 128, 250, 64);
      if(ncplane_printf(n, "Got ASCII: [0x%02x (%03d)] '%lc'\n",
                        r, r, iswprint(r) ? r : printutf8(r)) < 0){
        break;
      }
    }else{
      if(wchar_supppuab_p(r)){
        ncplane_set_fg_rgb(n, 250, 64, 128);
        if(ncplane_printf(n, "Got special key: [0x%02x (%02d)] '%s'\n",
                          r, r, nckeystr(r)) < 0){
          break;
        }
      }else{
        ncplane_set_fg_rgb(n, 64, 128, 250);
        ncplane_printf(n, "Got UTF-8: [0x%08x] '%lc'\n", r, r);
      }
    }
    if(dim_rows(n)){
      break;
    }
    if(notcurses_render(nc)){
      break;
    }
    if(++y >= dimy - 2){ // leave a blank line at the bottom
      y = 2;             // and at the top
    }
    while(cells.size() >= dimy - 3u){
      cells.pop_back();
    }
    cells.push_front(r);
  }
  int e = errno;
  notcurses_mouse_disable(nc);
  notcurses_stop(nc);
  if(r == UINT_LEAST32_MAX && e){
    std::cerr << "Error reading from terminal (" << strerror(e) << "?)\n";
  }
  return EXIT_FAILURE;
}
