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
