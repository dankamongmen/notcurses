void UpdateScore(){
  if(scoreplane_->printf(1, 0, "level: %02d score: %ju", level_, static_cast<uintmax_t>(score_)) < 0){
    throw TetrisNotcursesErr("printf()");
  }
}
