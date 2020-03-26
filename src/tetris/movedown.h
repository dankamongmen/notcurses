// returns true if the game has ended as a result of this move down
bool MoveDown() {
  const std::lock_guard<std::mutex> lock(mtx_);
  int y, x;
  if(PrepForMove(&y, &x)){
    if(PieceStuck()){
      if(y <= board_top_y_ - 1){
        return true;
      }
      LockPiece();
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
