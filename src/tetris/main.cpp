#include <thread>
#include <cstdlib>
#include <clocale>
#include <ncpp/NotCurses.hh>

class Tetris {
public:
  Tetris(ncpp::NotCurses& nc) :
    nc_(nc),
    score_(0)
  {
    // FIXME draw board
  }

  // 0.5 cell aspect: One board height == one row. One board width == two columns.
  static constexpr auto BOARD_WIDTH = 10;
  static constexpr auto BOARD_HEIGHT = 20;

  // FIXME ideally this would be called from constructor :/
  void Ticker(){
    // FIXME check whether we ought die
    while(true){
      // FIXME get value from level
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

private:
  ncpp::NotCurses& nc_;
  uint64_t score_;

};

int main(void){
  if(setlocale(LC_ALL, "") == nullptr){
    return EXIT_FAILURE;
  }
  notcurses_options ncopts{};
  ncpp::NotCurses nc(ncopts);
  Tetris t{nc};
  std::thread tid(&Tetris::Ticker, t);
  while(true); // FIXME spin on keyboard input, play game
  return nc.stop() ? EXIT_SUCCESS : EXIT_FAILURE;
}
