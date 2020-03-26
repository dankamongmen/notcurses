// background is drawn to the standard plane, at the bottom.
void DrawBackground(const std::string& s) {
  int averr;
  try{
    backg_ = std::make_unique<ncpp::Visual>(s.c_str(), &averr, 0, 0, ncpp::NCScale::Stretch);
  }catch(std::exception& e){
    throw TetrisNotcursesErr("visual(): " + s + ": " + e.what());
  }
  if(!backg_->decode(&averr)){
    throw TetrisNotcursesErr("decode(): " + s);
  }
  if(backg_->render(0, 0, -1, -1) <= 0){
    throw TetrisNotcursesErr("render(): " + s);
  }
  backg_->get_plane()->greyscale();
}

// draw the background on the standard plane, then create a new plane for the play area.
void DrawBoard() {
  DrawBackground(BackgroundFile);
  int y, x;
  stdplane_->get_dim(&y, &x);
  board_top_y_ = y - (BOARD_HEIGHT + 2);
  board_ = std::make_unique<ncpp::Plane>(BOARD_HEIGHT, BOARD_WIDTH * 2,
                                          board_top_y_, x / 2 - (BOARD_WIDTH + 1));
  uint64_t channels = 0;
  channels_set_fg(&channels, 0x00b040);
  channels_set_bg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
  if(!board_->double_box(0, channels, BOARD_HEIGHT - 1, BOARD_WIDTH * 2 - 1, NCBOXMASK_TOP)){
    throw TetrisNotcursesErr("double_box()");
  }
  channels_set_fg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
  board_->set_base(channels, 0, "");
  scoreplane_ = std::make_unique<ncpp::Plane>(1, 20, y - BOARD_HEIGHT, 2, nullptr);
  if(!scoreplane_){
    throw TetrisNotcursesErr("Plane()");
  }
  uint64_t scorechan = 0;
  channels_set_bg_alpha(&scorechan, CELL_ALPHA_TRANSPARENT);
  channels_set_fg_alpha(&scorechan, CELL_ALPHA_TRANSPARENT);
  if(!scoreplane_->set_base(scorechan, 0, "")){
    throw TetrisNotcursesErr("set_base()");
  }
  scoreplane_->set_fg(0x00d0a0);
  scoreplane_->set_bg_alpha(CELL_ALPHA_TRANSPARENT);
  UpdateScore();
  if(!nc_.render()){
    throw TetrisNotcursesErr("render()");
  }
}
