static constexpr int MAX_LEVEL = 16;
// the number of milliseconds before a drop is forced at the given level,
// using the NES fps counter of 50ms
static constexpr int Gravity(int level) {
  constexpr int MS_PER_GRAV = 30; // 10MHz*63/88/455/525 (~29.97fps) in NTSC
  // The number of frames before a drop is forced, per level
  constexpr std::array<int, MAX_LEVEL + 1> Gravities = {
    43, 38, 33, 28, 23, 18, 13, 8, 6, 5, 5, 4, 4, 3, 2, 1
  };
  if(level < 0){
    throw std::out_of_range("Illegal level");
  }
  if(static_cast<unsigned long>(level) < Gravities.size()){
    return Gravities[level] * MS_PER_GRAV;
  }
  return MS_PER_GRAV; // all levels 29+ are a single grav
}
