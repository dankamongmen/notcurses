#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <clocale>
#include <ncpp/NotCurses.hh>

using namespace std::chrono_literals;

class Tetris {
public:
  Tetris(ncpp::NotCurses& nc) :
    nc_(nc),
    score_(0),
    msdelay_(10ms)
  {
    // FIXME draw board
  }

  // 0.5 cell aspect: One board height == one row. One board width == two columns.
  static constexpr auto BOARD_WIDTH = 10;
  static constexpr auto BOARD_HEIGHT = 20;

  // FIXME ideally this would be called from constructor :/
  void Ticker(){
    std::chrono::milliseconds ms;
    do{
      mtx_.lock();
      ms = msdelay_;
      mtx_.unlock();
      std::this_thread::sleep_for(ms);
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
