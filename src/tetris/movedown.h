bool MoveDown() { // returns true if the game has ended as a result of this move
  int y, x;
  if(PrepForMove(&y, &x)){
    if(!curpiece_->move(y + 1, x)){
      throw TetrisNotcursesErr("move()");
    }
    if(InvalidMove()){
      if(!curpiece_->move(y, x)){
        throw TetrisNotcursesErr("move()");
      }
      if(y <= board_top_y_ - 1){
        return true;
      }
      if(LockPiece()){
        return true;
      }
      curpiece_ = NewPiece();
    }else{
      ++y;
    }
    if(nc_.render()){
      throw TetrisNotcursesErr("render()");
    }
  }
  return false;
}
