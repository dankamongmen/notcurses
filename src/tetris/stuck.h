bool InvalidMove() { // a bit wasteful, but piece are tiny
  int dy, dx;
  curpiece_->get_dim(&dy, &dx);
  while(dy--){
    int x = dx;
    while(x--){
      ncpp::Cell c, b;
      curpiece_->get_at(dy, x, &c);
      if(c.is_simple()){
        continue;
      }
      curpiece_->release(c);
      int transy = dy, transx = x; // need game area coordinates via translation
      curpiece_->translate(*board_, &transy, &transx);
      if(transy < 0 || transy >= board_->get_dim_y() || transx < 0 || transx >= board_->get_dim_x()){
        return true;
      }
      board_->get_at(transy, transx, &b);
      if(!b.is_simple()){
        return true;
      }
      board_->release(b);
    }
  }
  return false;
}
