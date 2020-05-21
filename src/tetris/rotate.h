void RotateCcw() {
  int y, x;
  if(!PrepForMove(&y, &x)){
    return;
  }
  curpiece_->rotate_ccw();
  if(InvalidMove()){
    curpiece_->rotate_cw();
  }
  nc_.render();
}

void RotateCw() {
  int y, x;
  if(!PrepForMove(&y, &x)){
    return;
  }
  curpiece_->rotate_cw();
  if(InvalidMove()){
    curpiece_->rotate_ccw();
  }
  nc_.render();
}
