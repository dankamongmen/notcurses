void LockPiece(){
  curpiece_->mergedown(*board_);
  if(!board_->cursor_move(0, 1)){
    throw TetrisNotcursesErr("cursor_move()");
  }
  int bdimy, bdimx;
  board_->get_dim(&bdimy, &bdimx);
  int cleared; // how many contiguous lines were cleared
  do{
    uint64_t tl = 0, tr = 0, bl = 0, br = 0;
    channels_set_fg(&tl, 0xff0000); channels_set_bg_alpha(&tl, CELL_ALPHA_TRANSPARENT);
    channels_set_fg(&tr, 0x00ff00); channels_set_bg_alpha(&tr, CELL_ALPHA_TRANSPARENT);
    channels_set_fg(&bl, 0x0000ff); channels_set_bg_alpha(&bl, CELL_ALPHA_TRANSPARENT);
    channels_set_fg(&br, 0x00ffff); channels_set_bg_alpha(&br, CELL_ALPHA_TRANSPARENT);
    if(!board_->stain(bdimy - 2, bdimx - 2, tl, tr, bl, br)){
      throw TetrisNotcursesErr("stain()");
    }
    cleared = 0;
    int y;
    for(y = bdimy - 2 ; y > 0 ; --y){ // get the lowest cleared area
      if(LineClear(y)){
        ++cleared;
      }else if(cleared){
        break;
      }
    }
    if(cleared){ // topmost verified clear is y + 1, bottommost is y + cleared
      for(int dy = y ; dy >= 0 ; --dy){
        for(int x = 1 ; x < bdimx - 2 ; ++x){
          ncpp::Cell c;
          if(board_->get_at(dy, x, &c) < 0){
            throw TetrisNotcursesErr("get_at()");
          }
          if(board_->putc(dy + cleared, x, &c) < 0){
            throw TetrisNotcursesErr("putc()");
          }
          c.get().gcluster = 0;
          if(board_->putc(dy, x, &c) < 0){
            throw TetrisNotcursesErr("putc()");
          }
        }
      }
      linescleared_ += cleared;
      static constexpr int points[] = {50, 150, 350, 1000};
      score_ += (level_ + 1) * points[cleared - 1];
      level_ = linescleared_ / 10;
      UpdateScore();
    }
  }while(cleared);
}
