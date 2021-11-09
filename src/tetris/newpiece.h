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
  const int tidx = rand() % 7;
  const struct tetrimino* t = &tetriminos[tidx];
  const size_t cols = strlen(t->texture);
  unsigned y, x;
  stdplane_->get_dim(&y, &x);
  const int xoff = x / 2 - BOARD_WIDTH + 2 * (rand() % (BOARD_WIDTH / 2));
  std::unique_ptr<ncpp::Plane> n = std::make_unique<ncpp::Plane>(2, cols, board_top_y_ - 1, xoff, nullptr);
  if(n){
    uint64_t channels = 0;
    ncchannels_set_bg_alpha(&channels, NCALPHA_TRANSPARENT);
    ncchannels_set_fg_alpha(&channels, NCALPHA_TRANSPARENT);
    n->set_fg_rgb(t->color);
    n->set_bg_alpha(NCALPHA_TRANSPARENT);
    n->set_base("", 0, channels);
    y = 0; x = 0;
    for(size_t i = 0 ; i < strlen(t->texture) ; ++i){
      if(t->texture[i] == '*'){
        n->putstr(y, x, "██");
      }
      y += ((x = ((x + 2) % cols)) == 0);
    }
  }
  nc_.render();
  return n;
}
