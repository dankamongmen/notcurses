#include <deque>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <clocale>
#include <iostream>
#include <notcurses.h>

static int dimy, dimx;
static struct notcurses* nc;

const char* nckeystr(ncspecial_key key){
  switch(key){
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
wchar_t printutf8(int kp){
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
  opts.outfp = stdout;
  if((nc = notcurses_init(&opts)) == nullptr){
    return EXIT_FAILURE;;
  }
  struct ncplane* n = notcurses_stdplane(nc);
  int r;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  cell c = CELL_TRIVIAL_INITIALIZER;
  if(ncplane_set_fg_rgb(n, 255, 255, 255)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  int y = 1;
  std::deque<cell> cells;
  ncspecial_key special;
  while(errno = 0, (r = notcurses_getc_blocking(nc, &c, &special)) >= 0){
    if(r == 0){ // interrupted by signal
      continue;
    }
    if(ncplane_cursor_move_yx(n, y, 0)){
      break;
    }
    if(cell_simple_p(&c)){
      char kp = c.gcluster;
      if(kp == 0){
        if(ncplane_printf(n, "Got special key: [0x%02x (%02d)] '%s'\n",
                          special, special, nckeystr(special)) < 0){
          break;
        }
      }else if(ncplane_printf(n, "Got ASCII: [0x%02x (%03d)] '%lc'\n",
                              kp, kp, isprint(kp) ? kp : printutf8(kp)) < 0){
        break;
      }
    }else{
      ncplane_printf(n, "Curious! %d%5s\n", c.gcluster, ""); // FIXME
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
    cells.push_front(c);
  }
  int e = errno;
  notcurses_stop(nc);
  if(r < 0 && e){
    std::cerr << "Error reading from terminal (" << strerror(e) << "?)\n";
  }
  return EXIT_FAILURE;
}
