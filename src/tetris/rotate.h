void RotateCcw() {
  const std::lock_guard<std::mutex> lock(mtx_);
  int y, x;
  if(!PrepForMove(&y, &x)){
    return;
  }
  // FIXME need to check game board for validity of rotation in both of these
  if(!curpiece_->rotate_ccw() || !nc_.render()){
    throw TetrisNotcursesErr("rotate_ccw() or render()");
  }
}

void RotateCw() {
  const std::lock_guard<std::mutex> lock(mtx_);
  int y, x;
  if(!PrepForMove(&y, &x)){
    return;
  }
  if(!curpiece_->rotate_cw() || !nc_.render()){
    throw TetrisNotcursesErr("rotate_cw() or render()");
  }
}
