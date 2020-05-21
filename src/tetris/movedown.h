bool MoveDown() { // returns true if the game has ended as a result of this move
  int y, x;
  if(PrepForMove(&y, &x)){
    curpiece_->move(y + 1, x);
    if(InvalidMove()){
      curpiece_->move(y, x);
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
    nc_.render();
  }
  return false;
}
