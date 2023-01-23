#include <deque>
#include <cerrno>
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <cstring>
#include <cstdlib>
#include <clocale>
#include <getopt.h>
#include <iostream>
#include <ncpp/Plane.hh>
#include <ncpp/NotCurses.hh>

#define NANOSECS_IN_SEC 1000000000

static inline uint64_t
timenow_to_ns(){
  struct timespec t;
  if(clock_gettime(CLOCK_MONOTONIC, &t)){
    throw std::runtime_error("error retrieving time");
  }
  return t.tv_sec * NANOSECS_IN_SEC + t.tv_nsec;
}

using namespace ncpp;

std::mutex mtx;
uint64_t start;
std::atomic<bool> done;
static unsigned dimy, dimx;
static struct ncuplot* plot;

// return the string version of a special composed key
const char* nckeystr(char32_t spkey){
  switch(spkey){ // FIXME
    case NCKEY_RESIZE:
      mtx.lock();
      NotCurses::get_instance().refresh(&dimy, &dimx);
      mtx.unlock();
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
    case NCKEY_F31:     return "F31";
    case NCKEY_F32:     return "F32";
    case NCKEY_F33:     return "F33";
    case NCKEY_F34:     return "F34";
    case NCKEY_F35:     return "F35";
    case NCKEY_F36:     return "F36";
    case NCKEY_F37:     return "F37";
    case NCKEY_F38:     return "F38";
    case NCKEY_F39:     return "F39";
    case NCKEY_F40:     return "F40";
    case NCKEY_F41:     return "F41";
    case NCKEY_F42:     return "F42";
    case NCKEY_F43:     return "F43";
    case NCKEY_F44:     return "F44";
    case NCKEY_F45:     return "F45";
    case NCKEY_F46:     return "F46";
    case NCKEY_F47:     return "F47";
    case NCKEY_F48:     return "F48";
    case NCKEY_F49:     return "F49";
    case NCKEY_F50:     return "F50";
    case NCKEY_F51:     return "F51";
    case NCKEY_F52:     return "F52";
    case NCKEY_F53:     return "F53";
    case NCKEY_F54:     return "F54";
    case NCKEY_F55:     return "F55";
    case NCKEY_F56:     return "F56";
    case NCKEY_F57:     return "F57";
    case NCKEY_F58:     return "F58";
    case NCKEY_F59:     return "F59";
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
    case NCKEY_SEPARATOR: return "separator";
    case NCKEY_CAPS_LOCK: return "caps lock";
    case NCKEY_SCROLL_LOCK: return "scroll lock";
    case NCKEY_NUM_LOCK: return "num lock";
    case NCKEY_PRINT_SCREEN: return "print screen";
    case NCKEY_PAUSE: return "pause";
    case NCKEY_MENU: return "menu";
    // media keys, similarly only available through kitty's protocol
    case NCKEY_MEDIA_PLAY: return "play";
    case NCKEY_MEDIA_PAUSE: return "pause";
    case NCKEY_MEDIA_PPAUSE: return "play-pause";
    case NCKEY_MEDIA_REV: return "reverse";
    case NCKEY_MEDIA_STOP: return "stop";
    case NCKEY_MEDIA_FF: return "fast-forward";
    case NCKEY_MEDIA_REWIND: return "rewind";
    case NCKEY_MEDIA_NEXT: return "next track";
    case NCKEY_MEDIA_PREV: return "previous track";
    case NCKEY_MEDIA_RECORD: return "record";
    case NCKEY_MEDIA_LVOL: return "lower volume";
    case NCKEY_MEDIA_RVOL: return "raise volume";
    case NCKEY_MEDIA_MUTE: return "mute";
    case NCKEY_LSHIFT: return "left shift";
    case NCKEY_LCTRL: return "left ctrl";
    case NCKEY_LALT: return "left alt";
    case NCKEY_LSUPER: return "left super";
    case NCKEY_LHYPER: return "left hyper";
    case NCKEY_LMETA: return "left meta";
    case NCKEY_RSHIFT: return "right shift";
    case NCKEY_RCTRL: return "right ctrl";
    case NCKEY_RALT: return "right alt";
    case NCKEY_RSUPER: return "right super";
    case NCKEY_RHYPER: return "right hyper";
    case NCKEY_RMETA: return "right meta";
    case NCKEY_L3SHIFT: return "level 3 shift";
    case NCKEY_L5SHIFT: return "level 5 shift";
    case NCKEY_MOTION: return "mouse (no buttons pressed)";
    case NCKEY_BUTTON1: return "mouse (button 1)";
    case NCKEY_BUTTON2: return "mouse (button 2)";
    case NCKEY_BUTTON3: return "mouse (button 3)";
    case NCKEY_BUTTON4: return "mouse (button 4)";
    case NCKEY_BUTTON5: return "mouse (button 5)";
    case NCKEY_BUTTON6: return "mouse (button 6)";
    case NCKEY_BUTTON7: return "mouse (button 7)";
    case NCKEY_BUTTON8: return "mouse (button 8)";
    case NCKEY_BUTTON9: return "mouse (button 9)";
    case NCKEY_BUTTON10: return "mouse (button 10)";
    case NCKEY_BUTTON11: return "mouse (button 11)";
    default:            return "unknown";
  }
}

// Print the utf8 Control Pictures for otherwise unprintable ASCII
char32_t printutf8(char32_t kp){
  if(kp <= NCKEY_ESC){
    return 0x2400 + kp;
  }
  return kp;
}

// Dim all text on the plane by the same amount. This will stack for
// older text, and thus clearly indicate the current output.
static bool
dim_rows(const Plane* n){
  Cell c;
  for(unsigned y = 0 ; y < dimy ; ++y){
    for(unsigned x = 0 ; x < dimx ; ++x){
      if(n->get_at(y, x, &c) < 0){
        n->release(c);
        return false;
      }
      unsigned r, g, b;
      c.get_fg_rgb8(&r, &g, &b);
      r -= r / 32;
      g -= g / 32;
      b -= b / 32;
      if(r > 247){ r = 0; }
      if(g > 247){ g = 0; }
      if(b > 247){ b = 0; }
      if(!c.set_fg_rgb8(r, g, b)){
        n->release(c);
        return false;
      }
      if(n->putc(y, x, c) < 0){
        n->release(c);
        return false;
      }
      if(c.is_double_wide()){
        ++x;
      }
    }
  }
  n->release(c);
  return true;
}

void Tick(ncpp::NotCurses* nc, uint64_t sec) {
  const std::lock_guard<std::mutex> lock(mtx);
  // might fail on various geometry changes
  if(ncuplot_add_sample(plot, sec, 0) == 0){
    if(!nc->render()){
      throw std::runtime_error("error rendering");
    }
  }
}

void Ticker(ncpp::NotCurses* nc) {
  do{
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    const uint64_t sec = (timenow_to_ns() - start) / NANOSECS_IN_SEC;
    Tick(nc, sec);
  }while(!done);
}

char evtype_to_char(ncinput* ni){
  switch(ni->evtype){
    case EvType::Unknown:
      return 'u';
    case EvType::Press:
      return 'P';
    case EvType::Repeat:
      return 'R';
    case EvType::Release:
      return 'L';
  }
  return 'X';
}

int input_demo(ncpp::NotCurses* nc) {
  constexpr auto PLOTHEIGHT = 6;
  constexpr auto PLOTWIDTH = 56;
  auto n = nc->get_stdplane(&dimy, &dimx);
  // FIXME no ncpp wrapper for Plane::pixelgeom?
  unsigned celldimx, maxbmapx;
  ncplane_pixel_geom(*n, nullptr, nullptr, nullptr, &celldimx, nullptr, &maxbmapx);
  struct ncplane_options nopts = {
    .y = static_cast<int>(dimy) - PLOTHEIGHT - 1,
    .x = NCALIGN_CENTER,
    .rows = PLOTHEIGHT,
    .cols = PLOTWIDTH,
    .userptr = nullptr,
    .name = "plot",
    .resizecb = ncplane_resize_realign,
    .flags = NCPLANE_OPTION_HORALIGNED,
    .margin_b = 0,
    .margin_r = 0,
  };
  struct ncplane* pplane = ncplane_create(*n, &nopts);
  if(pplane == nullptr){
   return EXIT_FAILURE;
  }
  struct ncplot_options popts{};
  // FIXME would be nice to switch over to exponential at some level
  popts.flags = NCPLOT_OPTION_LABELTICKSD | NCPLOT_OPTION_PRINTSAMPLE;
  popts.minchannels = popts.maxchannels = 0;
  ncchannels_set_fg_rgb8(&popts.minchannels, 0x40, 0x50, 0xb0);
  ncchannels_set_fg_rgb8(&popts.maxchannels, 0x40, 0xff, 0xd0);
  popts.gridtype = static_cast<ncblitter_e>(NCBLIT_PIXEL);
  plot = ncuplot_create(pplane, &popts, 0, 0);
  if(!plot){
    return EXIT_FAILURE;
  }
  n->set_fg_rgb8(0x00, 0x00, 0x00);
  n->set_bg_rgb8(0xbb, 0x64, 0xbb);
  n->styles_on(CellStyle::Underline);
  if(n->putstr(n->get_dim_y() - 1, NCAlign::Center, "mash keys, yo. give that mouse some waggle! ctrl+d exits.") <= 0){
    ncuplot_destroy(plot);
    return -1;
  }
  n->styles_set(CellStyle::None);
  n->set_bg_default();
  if(!nc->render()){
    ncuplot_destroy(plot);
    return -1;
  }
  unsigned y = 0;
  std::deque<wchar_t> cells;
  char32_t r;
  done = false;
  start = timenow_to_ns();
  std::thread tid(Ticker, nc);
  ncinput ni;
  while(errno = 0, (r = nc->get(true, &ni)) != (char32_t)-1){
    if(r == 0){ // interrupted by signal
      continue;
    }
    if((r == 'D' && ncinput_ctrl_p(&ni)) || r == NCKEY_EOF){
      done = true;
      tid.join();
      ncuplot_destroy(plot);
      return 0;
    }
    if(r == 'L' && ncinput_ctrl_p(&ni)){
      mtx.lock();
        if(!nc->refresh(nullptr, nullptr)){
          mtx.unlock();
          break;
        }
      mtx.unlock();
    }
    if(!n->cursor_move(y, 0)){
      break;
    }
    n->set_fg_rgb8(0xd0, 0xd0, 0xd0);
    n->printf("%c%c%c%c%c%c%c%c%c ",
              ncinput_shift_p(&ni) ? 'S' : 's',
              ncinput_alt_p(&ni) ? 'A' : 'a',
              ncinput_ctrl_p(&ni) ? 'C' : 'c',
              ncinput_super_p(&ni) ? 'U' : 'u',
              ncinput_hyper_p(&ni) ? 'H' : 'h',
              ncinput_meta_p(&ni) ? 'M' : 'm',
              ncinput_capslock_p(&ni) ? 'X' : 'x',
              ncinput_numlock_p(&ni) ? '#' : '.',
              evtype_to_char(&ni));
    if(r < 0x80){
      n->set_fg_rgb8(128, 250, 64);
      if(n->printf("ASCII: [0x%02x (%03d)] '%lc'", r, r,
                   (wint_t)(iswprint(r) ? r : printutf8(r))) < 0){
        break;
      }
    }else{
      if(nckey_synthesized_p(r)){
        n->set_fg_rgb8(250, 64, 128);
        if(n->printf("Special: [0x%02x (%02d)] '%s'", r, r, nckeystr(r)) < 0){
          break;
        }
        if(NCKey::IsMouse(r)){
          if(n->printf(-1, NCAlign::Right, " %d/%d", ni.x, ni.y) < 0){
            break;
          }
        }
      }else{
        n->set_fg_rgb8(64, 128, 250);
        n->printf("Unicode: [0x%08x] '%s'", r, ni.utf8);
      }
    }
    if(ni.eff_text[0] != ni.id || ni.eff_text[1] != 0){
      n->printf(" effective text '");
      for (int c=0; ni.eff_text[c]!=0; c++){
        unsigned char egc[5]={0};
        if(notcurses_ucs32_to_utf8(&ni.eff_text[c], 1, egc, 4)>=0){
          n->printf("%s", egc);
        }
      }
      n->printf("'");
    }
    unsigned x;
    n->get_cursor_yx(nullptr, &x);
    for(unsigned i = x ; i < n->get_dim_x() ; ++i){
      n->putc(' ');
    }
    if(!dim_rows(n)){
      break;
    }
    const uint64_t sec = (timenow_to_ns() - start) / NANOSECS_IN_SEC;
    mtx.lock();
    if(ncuplot_add_sample(plot, sec, 1)){
      mtx.unlock();
      break;
    }
    if(!nc->render()){
      mtx.unlock();
      ncuplot_destroy(plot);
      throw std::runtime_error("error rendering");
    }
    mtx.unlock();
    if(++y >= dimy - PLOTHEIGHT - 1){
      y = 0;
    }
    while(cells.size() >= dimy - 3u){
      cells.pop_back();
    }
    cells.push_front(r);
  }
  int e = errno;
  if(r == (char32_t)-1 && e){
    std::cerr << "Error reading from terminal (" << strerror(e) << "?)\n";
  }
  done = true;
  tid.join();
  ncuplot_destroy(plot);
  return 0;
}

static void
usage(const char* arg0, FILE* fp){
  fprintf(fp, "usage: %s [ -v ] [ -m ]\n", arg0);
  if(fp == stderr){
    exit(EXIT_FAILURE);
  }
  exit(EXIT_SUCCESS);
}

int main(int argc, char** argv){
  if(setlocale(LC_ALL, "") == nullptr){
    return EXIT_FAILURE;
  }
  notcurses_options nopts{};
  nopts.margin_t = 2;
  nopts.margin_l = 2;
  nopts.margin_r = 2;
  nopts.margin_b = 2;
  nopts.loglevel = NCLOGLEVEL_ERROR;
  bool nomice = false;
  int opt;
  while((opt = getopt(argc, argv, "vm")) != -1){
    switch(opt){
      case 'm':
        nomice = true;
        break;
      case 'v':
        nopts.loglevel = NCLOGLEVEL_TRACE;
        break;
      default:
        usage(argv[0], stderr);
        break;
    }
  }
  if(argv[optind]){ // non-option argument was provided
    usage(argv[0], stderr);
  }
  nopts.flags = NCOPTION_INHIBIT_SETLOCALE;
  NotCurses nc(nopts);
  if(!nomice){
    nc.mouse_enable(NCMICE_ALL_EVENTS);
  }
  int ret = input_demo(&nc);
  if(!nc.stop() || ret){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
