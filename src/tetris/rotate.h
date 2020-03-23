void RotateCcw() {
  const std::lock_guard<std::mutex> lock(mtx_);
  int y, x;
  if(!PrepForMove(&y, &x)){
    return;
  }
  // FIXME rotate that fucker ccw
}

void RotateCw() {
  const std::lock_guard<std::mutex> lock(mtx_);
  int y, x;
  if(!PrepForMove(&y, &x)){
    return;
  }
  // FIXME rotate that fucker cw
}
