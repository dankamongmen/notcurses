// tidx is an index into tetriminos. yoff and xoff are relative to the
// terminal's origin. returns colored north-facing tetrimino on a plane.
std::unique_ptr<ncpp::Plane> NewPiece() {
  // "North-facing" tetrimino forms (form in which they are released from the top) are expressed in terms of
  // two rows having between 2 and 4 columns. We map each game column to four columns and each game row to two
  // rows. Each byte of the texture maps to one 4x4 component block (and wastes 7 bits).
  static const struct tetrimino {
    unsigned color;
    const char* texture;
  } tetriminos[] = { // OITLJSZ
    { 0xcbc900, "****"}, { 0x009caa, "    ****"}, { 0x952d98, " * ***"}, { 0xcf7900, "  ****"},
    { 0x0065bd, "*  ***"},   { 0x69be28, " **** "}, { 0xbd2939, "**  **"} };
  const int tidx = random() % 7;
  const struct tetrimino* t = &tetriminos[tidx];
  const size_t cols = strlen(t->texture);
  int y, x;
  stdplane_->get_dim(&y, &x);
  const int xoff = x / 2 - BOARD_WIDTH + 2 * (random() % (BOARD_WIDTH / 2));
  std::unique_ptr<ncpp::Plane> n = std::make_unique<ncpp::Plane>(2, cols, board_top_y_ - 1, xoff, nullptr);
  if(n){
    uint64_t channels = 0;
    channels_set_bg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
    channels_set_fg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
    n->set_fg(t->color);
    n->set_bg_alpha(CELL_ALPHA_TRANSPARENT);
    n->set_base("", 0, channels);
    y = 0; x = 0;
    for(size_t i = 0 ; i < strlen(t->texture) ; ++i){
      if(t->texture[i] == '*'){
        if(n->putstr(y, x, "██") < 0){
          throw TetrisNotcursesErr("putstr()");
        }
      }
      y += ((x = ((x + 2) % cols)) == 0);
    }
  }
  if(!nc_.render()){
    throw TetrisNotcursesErr("render()");
  }
  return n;
}
