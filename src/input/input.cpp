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
const char* nckeystr(wchar_t spkey){
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
    default:            return "unknown";
  }
}

// print the utf8 Control Pictures for otherwise unprintable chars
wchar_t printutf8(wchar_t kp){
  if(kp <= 27 && kp >= 0){
    return 0x2400 + kp;
  }
  return kp;
}

int main(void){
  if(setlocale(LC_ALL, "") == nullptr){
    return EXIT_FAILURE;
  }
  notcurses_options opts{};
  if((nc = notcurses_init(&opts, stdout)) == nullptr){
    return EXIT_FAILURE;;
  }
  struct ncplane* n = notcurses_stdplane(nc);
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  if(ncplane_set_fg_rgb(n, 255, 255, 255)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  ncplane_set_bg_default(n);
  int y = 1;
  std::deque<wchar_t> cells;
  wchar_t r;
  while(errno = 0, (r = notcurses_getc_blocking(nc)) >= 0){
    if(r == 0){ // interrupted by signal
      continue;
    }
    if(ncplane_cursor_move_yx(n, y, 0)){
      break;
    }
    if(r < 0x80){
      ncplane_set_fg_rgb(n, 175, 255, 135);
      if(ncplane_printf(n, "Got ASCII: [0x%02x (%03d)] '%lc'\n",
                        r, r, iswprint(r) ? r : printutf8(r)) < 0){
        break;
      }
    }else{
      if(wchar_supppuab_p(r)){
        ncplane_set_fg_rgb(n, 175, 175, 255);
        if(ncplane_printf(n, "Got special key: [0x%02x (%02d)] '%s'\n",
                          r, r, nckeystr(r)) < 0){
          break;
        }
      }else{
        ncplane_set_fg_rgb(n, 215, 0, 95);
        ncplane_printf(n, "Got UTF-8: [0x%08x %lc]\n", r, r);
      }
    }
    // FIXME reprint all lines, fading older ones
    if(notcurses_render(nc)){
      break;
    }
    if(++y >= dimy - 2){ // leave a blank line at the bottom
      y = 1;             // and at the top
    }
    while(cells.size() >= dimy - 3u){
      cells.pop_back();
    }
    cells.push_front(r);
  }
  int e = errno;
  notcurses_stop(nc);
  if(r < 0 && e){
    std::cerr << "Error reading from terminal (" << strerror(e) << "?)\n";
  }
  return EXIT_FAILURE;
}
