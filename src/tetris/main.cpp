#define NCPP_EXCEPTIONS_PLEASE
#include <mutex>
#include <array>
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
#include <cstdlib>
#include <clocale>
#include <unistd.h>
#include <cinttypes>
#include <ncpp/NotCurses.hh>
#include <ncpp/Visual.hh>
#include "compat/compat.h"
#include "builddef.h"
#include "version.h"

std::mutex ncmtx;

const std::string BackgroundFile = notcurses_data_path(nullptr, "tetris-background.jpg");
const std::string LogoFile = notcurses_data_path(nullptr, "notcurses.png");

using namespace std::chrono_literals;

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
    level_(1),
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
#include "stain.h"
#include "lock.h"
#include "movedown.h"
#include "movelateral.h"
#include "rotate.h"

private:
  ncpp::NotCurses& nc_;
  uint64_t score_;
  std::mutex mtx_; // guards msdelay_
  std::unique_ptr<ncpp::Plane> curpiece_;
  std::unique_ptr<ncpp::Plane> board_;
  std::unique_ptr<ncpp::Plane> logop_;
  std::unique_ptr<ncpp::Visual> backg_;
  ncpp::Plane* stdplane_;
  std::unique_ptr<ncpp::Plane> scoreplane_;
  std::atomic_bool& gameover_;
  int board_top_y_;
  int level_;
  int linescleared_;
  std::chrono::milliseconds msdelay_;

  // Returns true if there's a current piece which can be moved
  auto PrepForMove(int* y, int* x) -> bool {
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

#include "main.h"
