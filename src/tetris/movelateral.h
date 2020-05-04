void MoveLateral(int direction) { // pass in -1 for left, 1 for right
  int shift = 2 * direction;
  int y, x;
  if(PrepForMove(&y, &x)){
    if(!curpiece_->move(y, x + shift)){
      throw TetrisNotcursesErr("move()");
    }
    if(InvalidMove()){
      if(!curpiece_->move(y, x)){
        throw TetrisNotcursesErr("move()");
      }
    }else{
      x += shift;
      if(!nc_.render()){
        throw TetrisNotcursesErr("render()");
      }
    }
  }
}

inline void MoveLeft() {
  MoveLateral(-1);
}

inline void MoveRight() {
  MoveLateral(1);
}
