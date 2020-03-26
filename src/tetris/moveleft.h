void MoveLeft() {
  const std::lock_guard<std::mutex> lock(mtx_);
  int y, x;
  if(!PrepForMove(&y, &x)){
    return;
  }
  // For each line of the current piece, find the leftmost populated column.
  // Check the game area to the immediate left. If something's there, we
  // can't make this move.
  ncpp::Cell c;
  for(int ly = 0 ; ly < curpiece_->get_dim_y() ; ++ly){
    int lx = 0;
    while(lx < curpiece_->get_dim_x()){
      if(curpiece_->get_at(ly, lx, &c)){
        if(c.get().gcluster && c.get().gcluster != ' '){
          break;
        }
      }
      ++lx;
    }
    if(lx < curpiece_->get_dim_x()){ // otherwise, nothing on this row
      ncpp::Cell b;
      for(int xdelt = 1 ; xdelt < 3 ; ++xdelt){
        int cmpy = ly, cmpx = lx - xdelt;
        curpiece_->translate(*board_, &cmpy, &cmpx);
        if(board_->get_at(cmpy, cmpx, &b)){
          // FIXME deal with e.g. lower half sliding under upper half
          if(b.get().gcluster && b.get().gcluster != ' '){
            return; // move is blocked
          }
        }
      }
    }
  }
  x -= 2;
  if(!curpiece_->move(y, x) || !nc_.render()){ // FIXME needs y?
    throw TetrisNotcursesErr("move() or render()");
  }
}
