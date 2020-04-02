void MoveRight() {
  const std::lock_guard<std::mutex> lock(mtx_);
  int y, x;
  if(PrepForMove(&y, &x)){
    if(!curpiece_->move(y, x + 2)){
      throw TetrisNotcursesErr("move()");
    }
    if(InvalidMove()){
      if(!curpiece_->move(y, x)){
        throw TetrisNotcursesErr("move()");
      }
    }else{
      x += 2;
      if(!nc_.render()){
        throw TetrisNotcursesErr("render()");
      }
    }
  }
}
