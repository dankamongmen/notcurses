void StainBoard(int dimy, int dimx){
  if(!board_->cursor_move(0, 1)){
    throw TetrisNotcursesErr("cursor_move()");
  }
  int high = 0xff - level_ * 16, low = level_ * 16; // rgb calculation limits us to 16 levels (0--15)
  uint64_t tl = 0, tr = 0, bl = 0, br = 0;
  channels_set_fg_rgb(&tl, high, 0, low); channels_set_bg_alpha(&tl, CELL_ALPHA_TRANSPARENT);
  channels_set_fg_rgb(&tr, low, high, 0); channels_set_bg_alpha(&tr, CELL_ALPHA_TRANSPARENT);
  channels_set_fg_rgb(&bl, 0, low, high); channels_set_bg_alpha(&bl, CELL_ALPHA_TRANSPARENT);
  channels_set_fg_rgb(&br, 0, high, low); channels_set_bg_alpha(&br, CELL_ALPHA_TRANSPARENT);
  if(!board_->stain(dimy - 2, dimx - 2, tl, tr, bl, br)){
    throw TetrisNotcursesErr("stain()");
  }
}

void LockPiece(){
  curpiece_->mergedown(*board_);
  int bdimy, bdimx;
  board_->get_dim(&bdimy, &bdimx);
  int cleared; // how many contiguous lines were cleared
  do{
    cleared = 0;
    int y;
    StainBoard(bdimy, bdimx);
    for(y = bdimy - 2 ; y > 0 ; --y){ // get the lowest cleared area
      if(LineClear(y)){
        ++cleared;
      }else if(cleared){
        break;
      }
    }
    if(cleared){ // topmost verified clear is y + 1, bottommost is y + cleared
      for(int dy = y ; dy >= 0 ; --dy){
        for(int x = 1 ; x < bdimx - 1 ; ++x){
          ncpp::Cell c;
          if(board_->get_at(dy, x, &c) < 0){
            throw TetrisNotcursesErr("get_at()");
          }
          if(board_->putc(dy + cleared, x, &c) < 0){
            throw TetrisNotcursesErr("putc()");
          }
          c.get().gcluster = 0;
          if(board_->putc(dy, x, &c) < 0){ // could just do this at end...
            throw TetrisNotcursesErr("putc()");
          }
        }
      }
      linescleared_ += cleared;
      static constexpr int points[] = {50, 150, 350, 1000};
      score_ += (level_ + 1) * points[cleared - 1];
      level_ = linescleared_ / 10;
      StainBoard(bdimy, bdimx);
      UpdateScore();
    }
  }while(cleared);
}
