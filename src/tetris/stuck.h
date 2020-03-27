bool PieceStuck() {
  if(curpiece_){
    // check for impact. iterate over bottom row of piece's plane, checking for
    // presence of glyph. if there, check row below. if row below is occupied,
    // we're stuck.
    int y, dimx, x;
    curpiece_->get_dim(&y, &dimx);
    std::vector<bool> columns(dimx, false); // bitmap for column verification
    int checked = 0;
    while(y--){
      x = dimx;
      while(x--){
        if(!columns[x]){
          ncpp::Cell piecec;
          if(curpiece_->get_at(y, x, &piecec) < 0){
            throw TetrisNotcursesErr("get_at()");
          }
          if(piecec.is_simple()){
            continue;
          }
          int cmpy = y + 1, cmpx = x; // need game area coordinates via translation
          curpiece_->translate(*board_, &cmpy, &cmpx);
          ncpp::Cell c;
          if(board_->get_at(cmpy, cmpx, &c) < 0){
            throw TetrisNotcursesErr("get_at()");
          }
          if(!c.is_simple()){
            return true;
          }
          columns[x] = true;
          if(++checked == dimx){
            return false;
          }
        }
      }
    }
  }
  return false;
}
