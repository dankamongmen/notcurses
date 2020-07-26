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
  uint64_t tl = 0, tr = 0, bl = 0, br = 0;
  const int c1 = level_ % 2 ? high : low; const int c2 = level_ % 2 ? low : high;
  channels_set_fg_rgb(&tl, c1, green, c2); channels_set_bg_alpha(&tl, CELL_ALPHA_TRANSPARENT);
  channels_set_fg_rgb(&tr, c2, green, c1); channels_set_bg_alpha(&tr, CELL_ALPHA_TRANSPARENT);
  channels_set_fg_rgb(&bl, c2, green, c1); channels_set_bg_alpha(&bl, CELL_ALPHA_TRANSPARENT);
  channels_set_fg_rgb(&br, c1, green, c2); channels_set_bg_alpha(&br, CELL_ALPHA_TRANSPARENT);
  board_->stain(dimy - 2, dimx - 2, tl, tr, bl, br);
}
