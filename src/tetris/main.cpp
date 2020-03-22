#include <iostream>
#include <mutex>
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
  Tetris(ncpp::NotCurses& nc) :
    nc_(nc),
    score_(0),
    msdelay_(100ms),
    curpiece_(nullptr),
    board_(nullptr),
    backg_(nullptr),
    stdplane_(nc_.get_stdplane())
  {
    DrawBoard();
    curpiece_ = NewPiece();
  }

  // 0.5 cell aspect: One board height == one row. One board width == two columns.
  static constexpr auto BOARD_WIDTH = 10;
  static constexpr auto BOARD_HEIGHT = 20;

  // FIXME ideally this would be called from constructor :/
  void Ticker() {
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
        if(PieceStuck()){
          if(y <= board_top_y_ - 2){
            return; // FIXME game is over!
          }
          curpiece_->mergedown();
          curpiece_ = NewPiece();
        }else{
          ++y;
          if(!curpiece_->move(y, x) || !nc_.render()){
            throw TetrisNotcursesErr("move() or render()");
          }
        }
      }
    }while(ms != std::chrono::milliseconds::zero());
  }

  void Stop() {
    mtx_.lock();
    msdelay_ = std::chrono::milliseconds::zero(); // FIXME wake it up?
    mtx_.unlock();
  }

  void MoveLeft() {
    const std::lock_guard<std::mutex> lock(mtx_);
    int y, x;
    if(!PrepForMove(&y, &x)){
      return;
    }
    if(x <= stdplane_->get_dim_x() / 2 - BOARD_WIDTH){
      return;
    }
    --x;
    if(!curpiece_->move(y, x) || !nc_.render()){ // FIXME needs y?
      throw TetrisNotcursesErr("move() or render()");
    }
  }

  void MoveRight() {
    const std::lock_guard<std::mutex> lock(mtx_);
    int y, x;
    if(!PrepForMove(&y, &x)){
      return;
    }
    // FIXME need account for width of piece plane
    if(x >= (stdplane_->get_dim_x() + BOARD_WIDTH) / 2){
      return;
    }
    ++x;
    if(!curpiece_->move(y, x) || !nc_.render()){ // FIXME needs y?
      throw TetrisNotcursesErr("move() or render()");
    }
  }

  void RotateCcw() {
    const std::lock_guard<std::mutex> lock(mtx_);
    int y, x;
    if(!PrepForMove(&y, &x)){
      return;
    }
    // FIXME rotate that fucker ccw
  }

  void RotateCw() {
    const std::lock_guard<std::mutex> lock(mtx_);
    int y, x;
    if(!PrepForMove(&y, &x)){
      return;
    }
    // FIXME rotate that fucker cw
  }

private:
  ncpp::NotCurses& nc_;
  uint64_t score_;
  std::mutex mtx_;
  std::chrono::milliseconds msdelay_;
  std::unique_ptr<ncpp::Plane> curpiece_;
  std::unique_ptr<ncpp::Plane> board_;
  std::unique_ptr<ncpp::Visual> backg_;
  ncpp::Plane* stdplane_;
  int board_top_y_;

  // Returns true if there's a current piece which can be moved
  bool PrepForMove(int* y, int* x) {
    if(!curpiece_){
      return false;
    }
    curpiece_->get_yx(y, x);
    return true;
  }

  // background is drawn to the standard plane, at the bottom.
  void DrawBackground(const std::string& s) {
    int averr;
    try{
      backg_ = std::make_unique<ncpp::Visual>(s.c_str(), &averr, 0, 0, ncpp::NCScale::Stretch);
    }catch(std::exception& e){
      throw TetrisNotcursesErr("visual(): " + s + ": " + e.what());
    }
    if(!backg_->decode(&averr)){
      throw TetrisNotcursesErr("decode(): " + s);
    }
    if(!backg_->render(0, 0, 0, 0)){
      throw TetrisNotcursesErr("render(): " + s);
    }
  }

  // draw the background on the standard plane, then create a new plane for
  // the play area.
  void DrawBoard() {
    DrawBackground(BackgroundFile);
    int y, x;
    stdplane_->get_dim(&y, &x);
    board_top_y_ = y - (BOARD_HEIGHT + 2);
    board_ = std::make_unique<ncpp::Plane>(BOARD_HEIGHT, BOARD_WIDTH * 2,
                                           board_top_y_, x / 2 - (BOARD_WIDTH + 1));
    uint64_t channels = 0;
    channels_set_fg(&channels, 0x00b040);
    if(!board_->rounded_box(0, channels, BOARD_HEIGHT - 1, BOARD_WIDTH * 2 - 1, NCBOXMASK_TOP)){
      throw TetrisNotcursesErr("rounded_box()");
    }
    channels_set_bg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
    channels_set_fg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
    board_->set_base(channels, 0, "");
    if(!nc_.render()){
      throw TetrisNotcursesErr("render()");
    }
  }

  bool PieceStuck() {
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
      curpiece_->translate(*board_, &cmpy, &cmpx);
      ncpp::Cell c;
      if(board_->get_at(cmpy, cmpx, &c) < 0){
        throw TetrisNotcursesErr("get_at()");
      }
      if(c.get().gcluster && c.get().gcluster != ' '){
        return true;
      }
    }
    return false;
  }

  // tidx is an index into tetriminos. yoff and xoff are relative to the
  // terminal's origin. returns colored north-facing tetrimino on a plane.
  std::unique_ptr<ncpp::Plane> NewPiece() {
    const int tidx = random() % 7;
    const struct tetrimino* t = &tetriminos[tidx];
    const size_t cols = strlen(t->texture);
    int y, x;
    stdplane_->get_dim(&y, &x);
    const int xoff = x / 2 - BOARD_WIDTH + (random() % BOARD_WIDTH - 1);
    std::unique_ptr<ncpp::Plane> n = std::make_unique<ncpp::Plane>(2, cols, board_top_y_ - 2, xoff, nullptr);
    if(n){
      uint64_t channels = 0;
      channels_set_bg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
      channels_set_fg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
      n->set_fg(t->color);
      n->set_bg_alpha(CELL_ALPHA_TRANSPARENT);
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

int main(void) {
  if(setlocale(LC_ALL, "") == nullptr){
    return EXIT_FAILURE;
  }
  notcurses_options ncopts{};
  ncpp::NotCurses nc(ncopts);
  Tetris t{nc};
  std::thread tid(&Tetris::Ticker, &t);
  ncpp::Plane* stdplane = nc.get_stdplane();
  char32_t input;
  ncinput ni;
  while((input = nc.getc(true, &ni)) != (char32_t)-1){
    if(input == 'q'){
      break;
    }
    switch(input){
      case NCKEY_LEFT: t.MoveLeft(); break;
      case NCKEY_RIGHT: t.MoveRight(); break;
      case 'z': t.RotateCcw(); break;
      case 'x': t.RotateCw(); break;
      default:
        stdplane->cursor_move(0, 0);
        stdplane->printf("Got unknown input U+%06x", input);
        nc.render();
        break;
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
