void LockPiece(){
  curpiece_->mergedown(*board_);
  if(!board_->cursor_move(0, 1)){
    throw TetrisNotcursesErr("cursor_move()");
  }
  int bdimy, bdimx;
  board_->get_dim(&bdimy, &bdimx);
  int cleared; // how many contiguous lines were cleared
  do{
    uint64_t tl, tr, bl, br;
    tl = tr = bl = br = 0;
    channels_set_bg_alpha(&tl, CELL_ALPHA_TRANSPARENT);
    channels_set_bg_alpha(&tr, CELL_ALPHA_TRANSPARENT);
    channels_set_bg_alpha(&bl, CELL_ALPHA_TRANSPARENT);
    channels_set_bg_alpha(&br, CELL_ALPHA_TRANSPARENT);
    channels_set_fg(&tl, 0xff0000);
    channels_set_fg(&tr, 0x00ff00);
    channels_set_fg(&bl, 0x0000ff);
    channels_set_fg(&br, 0x00ffff);
    if(!board_->stain(bdimy - 2, bdimx - 2, tl, tr, bl, br)){
      throw TetrisNotcursesErr("stain()");
    }
    cleared = 0;
    for(int y = bdimy - 2 ; y > 0 ; --y){
      if(LineClear(y)){
        ++cleared;
      }else if(cleared){
        break;
      }
    }
    if(cleared){
      // FIXME purge them, augment score
    }
  }while(cleared);
}
