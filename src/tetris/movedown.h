// returns true if the game has ended as a result of this move down
bool MoveDown() {
  const std::lock_guard<std::mutex> lock(mtx_);
  int y, x;
  if(PrepForMove(&y, &x)){
    if(PieceStuck()){
      if(y <= board_top_y_ - 1){
        return true;
      }
      curpiece_->mergedown(*board_);
      if(!board_->cursor_move(0, 1)){
        throw TetrisNotcursesErr("cursor_move()");
      }
      int bdimy, bdimx;
      board_->get_dim(&bdimy, &bdimx);
      uint64_t tl, tr, bl, br;
      tl = tr = bl = br = 0;
      channels_set_bg_alpha(&tl, CELL_ALPHA_TRANSPARENT);
      channels_set_bg_alpha(&tr, CELL_ALPHA_TRANSPARENT);
      channels_set_bg_alpha(&bl, CELL_ALPHA_TRANSPARENT);
      channels_set_bg_alpha(&br, CELL_ALPHA_TRANSPARENT);
      channels_set_fg(&tl, 0xff0000);
      channels_set_fg(&tr, 0x00ff00);
      channels_set_fg(&bl, 0x0000ff);
      channels_set_fg(&br, 0x00ffff);
      if(!board_->stain(bdimy - 2, bdimx - 2, tl, tr, bl, br)){
        throw TetrisNotcursesErr("stain()");
      }
      curpiece_ = NewPiece();
    }else{
      ++y;
      if(!curpiece_->move(y, x)){
        throw TetrisNotcursesErr("move()");
      }
    }
    if(!nc_.render()){
      throw TetrisNotcursesErr("render()");
    }
  }
  return false;
}
