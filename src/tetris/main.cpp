#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <clocale>
#include <ncpp/NotCurses.hh>

const std::string BackgroundFile = "../data/tetris-background.jpeg";

using namespace std::chrono_literals;

class TetrisNotcursesErr : public std::runtime_error {
public:
  TetrisNotcursesErr(const std::string& s) throw()
    : std::runtime_error(s) {
  }
  TetrisNotcursesErr(char const* const message) throw()
    : std::runtime_error(message) {
  }
  virtual char const* what() const throw() {
    return exception::what();
  }
};

class Tetris {
public:
  Tetris(ncpp::NotCurses& nc, std::atomic_bool& gameover) :
    nc_(nc),
    score_(0),
    curpiece_(nullptr),
    board_(nullptr),
    backg_(nullptr),
    stdplane_(nc_.get_stdplane()),
    scoreplane_(nullptr),
    gameover_(gameover),
    level_(0),
    linescleared_(0),
    msdelay_(Gravity(level_))
  {
    DrawBoard();
    curpiece_ = NewPiece();
  }

  // 0.5 cell aspect: 1 board height == one row. 1 board width == two columns.
  static constexpr auto BOARD_WIDTH = 10;
  static constexpr auto BOARD_HEIGHT = 20;

#include "gravity.h"
#include "ticker.h"
#include "score.h"
#include "clear.h"
#include "lock.h"
#include "movedown.h"
#include "moveleft.h"
#include "moveright.h"
#include "rotate.h"

private:
  ncpp::NotCurses& nc_;
  uint64_t score_;
  std::mutex mtx_;
  std::unique_ptr<ncpp::Plane> curpiece_;
  std::unique_ptr<ncpp::Plane> board_;
  std::unique_ptr<ncpp::Visual> backg_;
  ncpp::Plane* stdplane_;
  std::unique_ptr<ncpp::Plane> scoreplane_;
  std::atomic_bool& gameover_;
  int board_top_y_;
  int level_;
  int linescleared_;
  std::chrono::milliseconds msdelay_;

  // Returns true if there's a current piece which can be moved
  bool PrepForMove(int* y, int* x) {
    if(!curpiece_){
      return false;
    }
    curpiece_->get_yx(y, x);
    return true;
  }

#include "background.h"
#include "stuck.h"
#include "newpiece.h"

};

int main(void) {
  if(setlocale(LC_ALL, "") == nullptr){
    return EXIT_FAILURE;
  }
  srand(time(NULL));
  std::atomic_bool gameover = false;
  notcurses_options ncopts{};
  ncpp::NotCurses nc(ncopts);
  Tetris t{nc, gameover};
  std::thread tid(&Tetris::Ticker, &t);
  ncpp::Plane* stdplane = nc.get_stdplane();
  char32_t input = 0;
  ncinput ni;
  while(!gameover && (input = nc.getc(true, &ni)) != (char32_t)-1){
    if(input == 'q'){
      break;
    }
    switch(input){
      case NCKEY_LEFT: case 'h': t.MoveLeft(); break;
      case NCKEY_RIGHT: case 'l': t.MoveRight(); break;
      case NCKEY_DOWN: case 'j': t.MoveDown(); break;
      case 'z': t.RotateCcw(); break;
      case 'x': t.RotateCw(); break;
      default:
        stdplane->cursor_move(0, 0);
        stdplane->printf("Got unknown input U+%06x", input);
        nc.render();
        break;
    }
  }
  if(gameover || input == 'q'){ // FIXME signal it on 'q'
    gameover = true;
    tid.join();
  }else{
    return EXIT_FAILURE;
  }
  return nc.stop() ? EXIT_SUCCESS : EXIT_FAILURE;
}
