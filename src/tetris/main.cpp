#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <clocale>
#include <ncpp/NotCurses.hh>

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
    curpiece_(nullptr),
    stdplane_(nc_.get_stdplane())
  {
    curpiece_ = NewPiece();
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
      // FIXME loop and verify we didn't get a spurious wakeup
      mtx_.unlock();
      std::this_thread::sleep_for(ms);
      mtx_.lock();
      if(curpiece_){
        int y, x;
        curpiece_->get_yx(&y, &x);
        ++y;
        if(PieceStuck()){
          curpiece_ = NewPiece();
        }else{
          if(!curpiece_->move(y, x) || !nc_.render()){
            throw TetrisNotcursesErr("move() or render()");
          }
        }
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
  std::unique_ptr<ncpp::Plane> curpiece_;
  ncpp::Plane* stdplane_;

  void DrawBoard(){
    int y, x;
    stdplane_->get_dim(&y, &x);
    uint64_t channels = 0;
    channels_set_fg(&channels, 0x00b040);
    if(!stdplane_->cursor_move(y - (BOARD_HEIGHT + 2), x / 2 - (BOARD_WIDTH + 1))){
      throw TetrisNotcursesErr("cursor_move()");
    }
    if(!stdplane_->rounded_box(0, channels, y - 1, x / 2 + BOARD_WIDTH + 1, NCBOXMASK_TOP)){
      throw TetrisNotcursesErr("rounded_box()");
    }
    if(!nc_.render()){
      throw TetrisNotcursesErr("render()");
    }
  }

  bool PieceStuck(){
    if(!curpiece_){
      return false;
    }
    // check for impact. iterate over bottom row of piece's plane, checking for
    // presence of glyph. if there, check row below. if row below is occupied,
    // we're stuck.
    int y, x;
    curpiece_->get_dim(&y, &x);
    --y;
    while(x--){
      int cmpy = y + 1, cmpx = x; // need absolute coordinates via translation
      curpiece_->translate(nullptr, &cmpy, &cmpx);
      ncpp::Cell c;
      auto egc = nc_.get_at(cmpy, cmpx, c);
      if(!egc){
        throw TetrisNotcursesErr("get_at()");
      }
      if(*egc && *egc != ' '){
        return true;
      }
    }
    return false;
  }

  // tidx is an index into tetriminos. yoff and xoff are relative to the
  // terminal's origin. returns colored north-facing tetrimino on a plane.
  std::unique_ptr<ncpp::Plane> NewPiece(){
    const int tidx = random() % 7;
    const struct tetrimino* t = &tetriminos[tidx];
    const size_t cols = strlen(t->texture);
    int y, x;
    stdplane_->get_dim(&y, &x);
    const int xoff = x / 2 - BOARD_WIDTH + (random() % BOARD_WIDTH - 3);
    const int yoff = y - (BOARD_HEIGHT + 4);
    std::unique_ptr<ncpp::Plane> n = std::make_unique<ncpp::Plane>(2, cols, yoff, xoff, nullptr);
    if(n){
      uint64_t channels = 0;
      channels_set_bg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
      channels_set_fg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
      n->set_fg(t->color);
      n->set_base(channels, 0, "");
      y = 0;
      for(size_t i = 0 ; i < strlen(t->texture) ; ++i){
        if(t->texture[i] == '*'){
          if(n->putstr(y, x, "██") < 0){
            return NULL;
          }
        }
        y += ((x = ((x + 2) % cols)) == 0);
      }
    }
    return n;
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
