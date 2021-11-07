bool LockPiece(){ // returns true if game has ended by reaching level 16
  curpiece_->mergedown_simple(*board_);
  unsigned bdimy, bdimx;
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
        for(unsigned x = 1 ; x < bdimx - 1 ; ++x){
          ncpp::Cell c;
          board_->get_at(dy, x, &c);
          board_->putc(dy + cleared, x, &c);
          c.get().gcluster = 0;
          board_->putc(dy, x, &c); // could just do this at end...
        }
      }
      linescleared_ += cleared;
      static constexpr int points[] = {50, 150, 350, 1000};
      score_ += (level_) * points[cleared - 1];
      if((level_ = linescleared_ / 10 + 1) > MAX_LEVEL){
        return true;
      }
      mtx_.lock();
      msdelay_ = std::chrono::milliseconds(Gravity(level_));
      mtx_.unlock();
      StainBoard(bdimy, bdimx);
      UpdateScore();
    }
  }while(cleared);
  return false;
}
