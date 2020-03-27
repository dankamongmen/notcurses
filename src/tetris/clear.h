// returns true iff the specified row of the board is clear (full with no gaps)
bool LineClear(int y){
  int dimx = board_->get_dim_x();
  for(int x = 1 ; x < dimx - 1 ; ++x){
    ncpp::Cell c;
    if(board_->get_at(y, x, &c) < 0){
      throw TetrisNotcursesErr("get_at()");
    }
    if(c.is_simple()){
      return false;
    }
  }
  return true;
}
