bool InvalidMove() { // a bit wasteful, but pieces are tiny
  unsigned dy, dx;
  curpiece_->get_dim(&dy, &dx);
  while(dy--){
    int x = dx;
    while(x--){
      ncpp::Cell c, b;
      curpiece_->get_at(dy, x, &c);
      if(strcmp(curpiece_->get_extended_gcluster(c), "") == 0){
        continue;
      }
      curpiece_->release(c);
      int transy = dy, transx = x; // need game area coordinates via translation
      curpiece_->translate(*board_, &transy, &transx);
      if(transy < 0 || (unsigned)transy >= board_->get_dim_y()
          || transx < 0 || (unsigned)transx >= board_->get_dim_x()){
        return true;
      }
      board_->get_at(transy, transx, &b);
      if(strcmp(board_->get_extended_gcluster(b), "")){
        return true;
      }
      board_->release(b);
    }
  }
  return false;
}
