bool InvalidMove() { // a bit wasteful, but piece are tiny
  int dy, dx;
  curpiece_->get_dim(&dy, &dx);
  while(dy--){
    int x = dx;
    while(x--){
      ncpp::Cell c, b;
      if(curpiece_->get_at(dy, x, &c) < 0){
        throw TetrisNotcursesErr("get_at()");
      }
      if(c.is_simple()){
        continue;
      }
      curpiece_->release(c);
      int transy = dy, transx = x; // need game area coordinates via translation
      curpiece_->translate(*board_, &transy, &transx);
      if(transy < 0 || transy >= board_->get_dim_y() || transx < 0 || transx >= board_->get_dim_x()){
        return true;
      }
      if(board_->get_at(transy, transx, &b) < 0){
        throw TetrisNotcursesErr("get_at()");
      }
      if(!b.is_simple()){
        return true;
      }
      board_->release(b);
    }
  }
  return false;
}

void RotateCcw() {
  const std::lock_guard<std::mutex> lock(mtx_);
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
  const std::lock_guard<std::mutex> lock(mtx_);
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
