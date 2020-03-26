bool PieceStuck() {
  if(curpiece_){
    // check for impact. iterate over bottom row of piece's plane, checking for
    // presence of glyph. if there, check row below. if row below is occupied,
    // we're stuck.
    int y, x;
    curpiece_->get_dim(&y, &x);
    while(x--){
      ncpp::Cell piecec;
      if(curpiece_->get_at(y - 1, x, &piecec) < 0){
        throw TetrisNotcursesErr("get_at()");
      }
      if(!piecec.get().gcluster || piecec.get().gcluster == ' '){
        continue;
      }
      const char* egc = curpiece_->get_extended_gcluster(piecec);
      if(strcmp(egc, "█")){
        continue;
      }
      int cmpy = y, cmpx = x; // need game area coordinates via translation
      curpiece_->translate(*board_, &cmpy, &cmpx);
      ncpp::Cell c;
      if(board_->get_at(cmpy, cmpx, &c) < 0){
        throw TetrisNotcursesErr("get_at()");
      }
      if(c.get().gcluster){
        if(c.get().gcluster != ' '){
          return true;
        }
      }
    }
  }
  return false;
}
