#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <clocale>
//#include <ncpp/Plane.hh>
#include <ncpp/NotCurses.hh>

using namespace std::chrono_literals;

class TetrisNotcursesErr : public std::runtime_error {
public:
  TetrisNotcursesErr(char const* const message) throw()
    : std::runtime_error(message) {
  }

  virtual char const* what() const throw(){
    return exception::what();
  }
};

class Tetris {
public:
  Tetris(ncpp::NotCurses& nc) :
    nc_(nc),
    score_(0),
    msdelay_(10ms),
    curpiece_(nullptr)
  {
    DrawBoard();
  }

  // 0.5 cell aspect: One board height == one row. One board width == two columns.
  static constexpr auto BOARD_WIDTH = 10;
  static constexpr auto BOARD_HEIGHT = 20;

  // FIXME ideally this would be called from constructor :/
  void Ticker(){
    std::chrono::milliseconds ms;
    mtx_.lock();
    do{
      ms = msdelay_;
      mtx_.unlock();
      std::this_thread::sleep_for(ms);
      mtx_.lock();
      if(curpiece_){
        // FIXME move it down
      }
    }while(ms != std::chrono::milliseconds::zero());
  }

  void Stop(){
    mtx_.lock();
    msdelay_ = std::chrono::milliseconds::zero(); // FIXME wake it up?
    mtx_.unlock();
  }

private:
  ncpp::NotCurses& nc_;
  uint64_t score_;
  std::mutex mtx_;
  std::chrono::milliseconds msdelay_;
  ncpp::Plane* curpiece_;
  ncpp::Plane* stdplane_;

  void DrawBoard(){
    int y, x;
    stdplane_ = nc_.get_stdplane(&y, &x);
    uint64_t channels = 0;
    channels_set_fg(&channels, 0x00b040);
    if(!stdplane_->cursor_move(y - (BOARD_HEIGHT + 1), x / 2 - BOARD_WIDTH)){
      throw TetrisNotcursesErr("cursor_move()");
    }
    if(!stdplane_->rounded_box(0, channels, y - 1, x / 2 + BOARD_WIDTH, NCBOXMASK_TOP)){
      throw TetrisNotcursesErr("rounded_box()");
    }
    if(!nc_.render()){
      throw TetrisNotcursesErr("render()");
    }
  }
};

int main(void){
  if(setlocale(LC_ALL, "") == nullptr){
    return EXIT_FAILURE;
  }
  notcurses_options ncopts{};
  ncpp::NotCurses nc(ncopts);
  Tetris t{nc};
  std::thread tid(&Tetris::Ticker, &t);
  char32_t input;
  ncinput ni;
  while((input = nc.getc(true, &ni)) != (char32_t)-1){
    if(input == 'q'){
      break;
    }
    switch(input){
      case NCKEY_LEFT: break;
      case NCKEY_RIGHT: break;
    }
  }
  if(input == 'q'){
    t.Stop();
    tid.join();
  }else{
    return EXIT_FAILURE;
  }
  return nc.stop() ? EXIT_SUCCESS : EXIT_FAILURE;
}
