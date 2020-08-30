void StainBoard(int dimy, int dimx){
  board_->cursor_move(0, 1);
  const int l = level_ - 1;
  int high = 0xff - (l / 2) * 0x10;
  int low = l * 0x10;
  int green = 0;
  if(low >= 0x100){
    low = low % 0x100;
  }
  green = (l / 2) * 0x20;
  const int c1 = level_ % 2 ? high : low;
  const int c2 = level_ % 2 ? low : high;
  uint64_t tl = CHANNELS_RGB_INITIALIZER(c1, green, c2, c1, green, c2);
  uint64_t tr = CHANNELS_RGB_INITIALIZER(c2, green, c1, c2, green, c1);
  uint64_t bl = CHANNELS_RGB_INITIALIZER(c2, green, c1, c2, green, c1);
  uint64_t br = CHANNELS_RGB_INITIALIZER(c1, green, c2, c1, green, c2);
  board_->stain(dimy - 2, dimx - 2, tl, tr, bl, br);
}
