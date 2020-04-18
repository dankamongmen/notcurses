void RotateCcw() {
  int y, x;
  if(!PrepForMove(&y, &x)){
    return;
  }
  if(!curpiece_->rotate_ccw()){
    throw TetrisNotcursesErr("rotate_ccw()");
  }
  if(InvalidMove() && !curpiece_->rotate_cw()){
    throw TetrisNotcursesErr("rotate_cw()");
  }
  if(!nc_.render()){
    throw TetrisNotcursesErr("render()");
  }
}

void RotateCw() {
  int y, x;
  if(!PrepForMove(&y, &x)){
    return;
  }
  if(!curpiece_->rotate_cw()){
    throw TetrisNotcursesErr("rotate_cw()");
  }
  if(InvalidMove() && !curpiece_->rotate_ccw()){
    throw TetrisNotcursesErr("rotate_ccw()");
  }
  if(!nc_.render()){
    throw TetrisNotcursesErr("render()");
  }
}
