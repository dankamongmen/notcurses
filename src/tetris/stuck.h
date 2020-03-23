bool PieceStuck() {
  if(!curpiece_){
    return false;
  }
  // check for impact. iterate over bottom row of piece's plane, checking for
  // presence of glyph. if there, check row below. if row below is occupied,
  // we're stuck.
  int y, x;
  curpiece_->get_dim(&y, &x);
  --y;
  while(x--){
    int cmpy = y + 1, cmpx = x; // need game area coordinates via translation
    curpiece_->translate(*board_, &cmpy, &cmpx);
    ncpp::Cell c;
    if(board_->get_at(cmpy, cmpx, &c) < 0){
      throw TetrisNotcursesErr("get_at()");
    }
    if(c.get().gcluster && c.get().gcluster != ' '){
      return true;
    }
  }
  return false;
}
