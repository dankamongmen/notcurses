// returns true iff the specified row of the board is clear (full with no gaps)
bool LineClear(int y){
  int dimx = board_->get_dim_x();
  for(int x = 1 ; x < dimx - 1 ; ++x){
    ncpp::Cell c;
    board_->get_at(y, x, &c);
    if(strcmp(board_->get_extended_gcluster(c), "") == 0){
      return false;
    }
  }
  return true;
}
