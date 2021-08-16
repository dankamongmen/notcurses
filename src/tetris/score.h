void UpdateScore(){
  scoreplane_->printf(1, 0, "level: %02d score: %" PRIu64, level_, static_cast<uintmax_t>(score_));
}
