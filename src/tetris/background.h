void DrawBackground(const std::string& s) { // drawn to the standard plane
  backg_ = std::make_unique<ncpp::Visual>(s.c_str());
  backg_->decode();
  ncvisual_options opts{};
  opts.scaling = NCSCALE_STRETCH;
  backg_->render(&opts);
}

void DrawBoard() { // draw all fixed components of the game
  try{
    DrawBackground(BackgroundFile);
  }catch(...){
    stdplane_->printf(1, 1, "couldn't load %s", BackgroundFile.c_str());
  }
  int y, x;
  stdplane_->get_dim(&y, &x);
  board_top_y_ = y - (BOARD_HEIGHT + 2);
  board_ = std::make_unique<ncpp::Plane>(BOARD_HEIGHT, BOARD_WIDTH * 2,
                                          board_top_y_, x / 2 - (BOARD_WIDTH + 1));
  uint64_t channels = 0;
  ncchannels_set_fg_rgb(&channels, 0x00b040);
  ncchannels_set_bg_alpha(&channels, NCALPHA_TRANSPARENT);
  board_->double_box(0, channels, BOARD_HEIGHT - 1, BOARD_WIDTH * 2 - 1, NCBOXMASK_TOP);
  ncchannels_set_fg_alpha(&channels, NCALPHA_TRANSPARENT);
  board_->set_base("", 0, channels);
  scoreplane_ = std::make_unique<ncpp::Plane>(2, 30, y - BOARD_HEIGHT, 2, nullptr);
  uint64_t scorechan = 0;
  ncchannels_set_bg_alpha(&scorechan, NCALPHA_TRANSPARENT);
  ncchannels_set_fg_alpha(&scorechan, NCALPHA_TRANSPARENT);
  scoreplane_->set_base("", 0, scorechan);
  scoreplane_->set_bg_alpha(NCALPHA_TRANSPARENT);
  scoreplane_->set_fg_rgb(0xd040d0);
  scoreplane_->printf(0, 1, "%s", getpwuid(geteuid())->pw_name);
  scoreplane_->set_fg_rgb(0x00d0a0);
  UpdateScore();
  nc_.render();
}
