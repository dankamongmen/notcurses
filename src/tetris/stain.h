void StainBoard(int dimy, int dimx){
  if(!board_->cursor_move(0, 1)){
    throw TetrisNotcursesErr("cursor_move()");
  }
  int high = 0xff - level_ * 16, low = level_ * 16; // rgb calculation limits us to 16 levels (0--15)
  uint64_t tl = 0, tr = 0, bl = 0, br = 0;
  channels_set_fg_rgb(&tl, high, 0xff, low); channels_set_bg_alpha(&tl, CELL_ALPHA_TRANSPARENT);
  channels_set_fg_rgb(&tr, low, high, 0xff); channels_set_bg_alpha(&tr, CELL_ALPHA_TRANSPARENT);
  channels_set_fg_rgb(&bl, 0xff, low, high); channels_set_bg_alpha(&bl, CELL_ALPHA_TRANSPARENT);
  channels_set_fg_rgb(&br, 0xff, high, low); channels_set_bg_alpha(&br, CELL_ALPHA_TRANSPARENT);
  if(!board_->stain(dimy - 2, dimx - 2, tl, tr, bl, br)){
    throw TetrisNotcursesErr("stain()");
  }
}
