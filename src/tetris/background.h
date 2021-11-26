void DrawBackground(const std::string& s) {
  backg_ = std::make_unique<ncpp::Visual>(s.c_str());
  ncvisual_options opts{};
  opts.scaling = NCSCALE_STRETCH;
  opts.n = *nc_.get_stdplane();
  opts.blitter = NCBLIT_3x2;
  opts.flags = NCVISUAL_OPTION_CHILDPLANE;
  backg_->blit(&opts);
}

void DrawLogo(const ncpp::Plane& score,
              const ncpp::Plane& board,
              const std::string& s) {
  auto logo = std::make_unique<ncpp::Visual>(s.c_str());
  auto rows = nc_.get_stdplane()->get_dim_y() -
              (score.get_dim_y() + score.get_abs_y()) - 2;
  auto cols = board.get_abs_x() - score.get_abs_x() - 1;
  logop_ = std::make_unique<ncpp::Plane>(rows, cols,
                score.get_abs_y() + 2, score.get_abs_x());
  ncvisual_options opts{};
  opts.scaling = NCSCALE_STRETCH;
  opts.n = *logop_;
  opts.blitter = NCBLIT_PIXEL;
  logo->blit(&opts);
}

void DrawBoard() { // draw all fixed components of the game
  // FIXME limit these catches to I/O errors!
  try{
    DrawBackground(BackgroundFile);
  }catch(...){
    stdplane_->printf(1, 1, "couldn't load bground from %s", BackgroundFile.c_str());
  }
  unsigned y, x;
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
  char* n = notcurses_accountname();
  scoreplane_->printf(0, 1, "%s", n);
  free(n);
  scoreplane_->set_fg_rgb(0x00d0a0);
  try{
    DrawLogo(*scoreplane_, *board_, LogoFile);
  }catch(...){
    stdplane_->printf(1, 1, "couldn't load logo from %s", LogoFile.c_str());
  }
  UpdateScore();
  nc_.render();
}
