#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <clocale>
#include <ncpp/NotCurses.hh>

const std::string BackgroundFile = "../data/tetris-background.jpeg";

using namespace std::chrono_literals;

// "North-facing" tetrimino forms (the form in which they are released from the
// top) are expressed in terms of two rows having between two and four columns.
// We map each game column to four columns and each game row to two rows.
// Each byte of the texture maps to one 4x4 component block (and wastes 7 bits).
static const struct tetrimino {
  unsigned color;
  const char* texture;
} tetriminos[] = { // OITLJSZ
  { 0xcbc900, "****"},   { 0x009caa, "    ****"}, { 0x952d98, " * ***"},
  { 0xcf7900, "  ****"}, { 0x0065bd, "*  ***"},   { 0x69be28, " **** "},
  { 0xbd2939, "**  **"} };

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
    gameover_(gameover),
    level_(0),
    msdelay_(Gravity(level_))
  {
    DrawBoard();
    curpiece_ = NewPiece();
  }

  // 0.5 cell aspect: One board height == one row. One board width == two columns.
  static constexpr auto BOARD_WIDTH = 10;
  static constexpr auto BOARD_HEIGHT = 20;

  #include "gravity.h"

  // FIXME ideally this would be called from constructor :/
  void Ticker() {
    std::chrono::milliseconds ms;
    mtx_.lock();
    do{
      ms = msdelay_;
      // FIXME loop and verify we didn't get a spurious wakeup
      mtx_.unlock();
      std::this_thread::sleep_for(ms);
      if(MoveDown()){
        gameover_ = true;
        return;
      }
    }while(!gameover_);
  }

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
  std::atomic_bool& gameover_;
  int board_top_y_;
  int level_;
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
