void MoveLateral(int direction) { // pass in -1 for left, 1 for right
  int shift = 2 * direction;
  int y, x;
  if(PrepForMove(&y, &x)){
    curpiece_->move(y, x + shift);
    if(InvalidMove()){
      curpiece_->move(y, x);
    }else{
      x += shift;
      nc_.render();
    }
  }
}

inline void MoveLeft() {
  MoveLateral(-1);
}

inline void MoveRight() {
  MoveLateral(1);
}
