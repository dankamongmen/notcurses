void UpdateScore(){
  scoreplane_->printf(1, 0, "level: %02d score: %" PRIu64, level_, score_);
}
