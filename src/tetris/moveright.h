void MoveRight() {
  const std::lock_guard<std::mutex> lock(mtx_);
  int y, x;
  if(!PrepForMove(&y, &x)){
    return;
  }
  // For each line of the current piece, find the rightmost populated column.
  // Check the game area to the immediate right. If something's there, we
  // can't make this move.
  ncpp::Cell c;
  for(int ly = 0 ; ly < curpiece_->get_dim_y() ; ++ly){
    int lx = curpiece_->get_dim_x() - 1;
    while(lx >= 0){
      if(curpiece_->get_at(ly, lx, &c)){
        if(c.get().gcluster && c.get().gcluster != ' '){
          break;
        }
      }
      --lx;
    }
    if(lx >= 0){ // otherwise, nothing on this row
      ncpp::Cell b;
      int cmpy = ly, cmpx = lx + 1;
      curpiece_->translate(*board_, &cmpy, &cmpx);
      if(board_->get_at(cmpy, cmpx, &b)){
        if(b.get().gcluster && b.get().gcluster != ' '){
          return; // move is blocked
        }
      }
    }
  }
  ++x;
  if(!curpiece_->move(y, x) || !nc_.render()){ // FIXME needs y?
    throw TetrisNotcursesErr("move() or render()");
  }
}
