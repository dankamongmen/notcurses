#include <deque>
#include <cerrno>
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <cstring>
#include <cstdlib>
#include <clocale>
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
static int dimy, dimx;
std::atomic<bool> done;
static struct ncuplot* plot;

// return the string version of a special composed key
const char* nckeystr(char32_t spkey){
  switch(spkey){ // FIXME
    case NCKEY_RESIZE:
      NotCurses::get_instance().refresh(&dimy, &dimx);
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
    case NCKEY_BUTTON1: return "mouse (button 1 pressed)";
    case NCKEY_BUTTON2: return "mouse (button 2 pressed)";
    case NCKEY_BUTTON3: return "mouse (button 3 pressed)";
    case NCKEY_BUTTON4: return "mouse (button 4 pressed)";
    case NCKEY_BUTTON5: return "mouse (button 5 pressed)";
    case NCKEY_BUTTON6: return "mouse (button 6 pressed)";
    case NCKEY_BUTTON7: return "mouse (button 7 pressed)";
    case NCKEY_BUTTON8: return "mouse (button 8 pressed)";
    case NCKEY_BUTTON9: return "mouse (button 9 pressed)";
    case NCKEY_BUTTON10: return "mouse (button 10 pressed)";
    case NCKEY_BUTTON11: return "mouse (button 11 pressed)";
    case NCKEY_RELEASE: return "mouse (button released)";
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
  int y, x;
  Cell c;
  for(y = 2 ; y < dimy ; ++y){
    for(x = 0 ; x < dimx ; ++x){
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
  if(ncuplot_add_sample(plot, sec, 0)){
    throw std::runtime_error("couldn't register timetick");
  }
  if(!nc->render()){
    throw std::runtime_error("error rendering");
  }
}

void Ticker(ncpp::NotCurses* nc) {
  do{
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    const uint64_t sec = (timenow_to_ns() - start) / NANOSECS_IN_SEC;
    Tick(nc, sec);
  }while(!done);
}

int input_demo(ncpp::NotCurses* nc) {
  constexpr auto PLOTHEIGHT = 6;
  auto n = nc->get_stdplane(&dimy, &dimx);
  struct ncplane_options nopts = {
    .y = dimy - PLOTHEIGHT,
    .x = 0,
    .rows = PLOTHEIGHT,
    .cols = dimx,
    .userptr = nullptr,
    .name = "plot",
    .resizecb = nullptr, // FIXME
    .flags = 0,
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
  if(n->putstr(0, NCAlign::Center, "mash keys, yo. give that mouse some waggle! ctrl+d exits.") <= 0){
    ncuplot_destroy(plot);
    return -1;
  }
  n->styles_set(CellStyle::None);
  n->set_bg_default();
  if(!nc->render()){
    ncuplot_destroy(plot);
    return -1;
  }
  int y = 2;
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
    if((r == 'D' || r == 'd') && ni.ctrl){
      done = true;
      tid.join();
      ncuplot_destroy(plot);
      return 0;
    }
    if((r == 'L' || r == 'l') && ni.ctrl){
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
    n->printf("%c%c%c ", ni.alt ? 'A' : 'a', ni.ctrl ? 'C' : 'c',
              ni.shift ? 'S' : 's');
    if(r < 0x80){
      n->set_fg_rgb8(128, 250, 64);
      if(n->printf("ASCII: [0x%02x (%03d)] '%lc'", r, r,
                   (wchar_t)(iswprint(r) ? r : printutf8(r))) < 0){
        break;
      }
    }else{
      if(nckey_supppuab_p(r)){
        n->set_fg_rgb8(250, 64, 128);
        if(n->printf("Special: [0x%02x (%02d)] '%s'", r, r, nckeystr(r)) < 0){
          break;
        }
        if(NCKey::IsMouse(r)){
          if(n->printf(-1, NCAlign::Right, " x: %d y: %d", ni.x, ni.y) < 0){
            break;
          }
        }
      }else{
        n->set_fg_rgb8(64, 128, 250);
        n->printf("Unicode: [0x%08x] '%lc'", r, (wchar_t)r);
      }
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
    if(++y >= dimy - PLOTHEIGHT){ // leave six lines free on the bottom...
      y = 2;                      // ...and one free on the top.
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

int main(void){
  if(setlocale(LC_ALL, "") == nullptr){
    return EXIT_FAILURE;
  }
  notcurses_options nopts{};
  nopts.flags = NCOPTION_INHIBIT_SETLOCALE;
  NotCurses nc(nopts);
  if(!nc.mouse_enable()){
    return EXIT_FAILURE;
  }
  int ret = input_demo(&nc);
  if(!nc.stop() || ret){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
