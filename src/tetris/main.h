bool IOLoop(ncpp::NotCurses& nc, Tetris& t, std::atomic_bool& gameover) {
  ncpp::Plane* stdplane = nc.get_stdplane();
  char32_t input = 0;
  ncinput ni;
  while(!gameover && (input = nc.get(true, &ni)) != (char32_t)-1){
    if(input == 'q'){
      break;
    }
    if(ni.evtype == ncpp::EvType::Release){
      continue;
    }
    ncmtx.lock();
    switch(input){
      case NCKEY_LEFT: case 'h': t.MoveLeft(); break;
      case NCKEY_RIGHT: case 'l': t.MoveRight(); break;
      case NCKEY_DOWN: case 'j': { if(t.MoveDown()){ gameover = true; } break; }
      case 'L': if(ni.ctrl){ nc.refresh(nullptr, nullptr); } break;
      case 'z': t.RotateCcw(); break;
      case 'x': t.RotateCw(); break;
      default:
        stdplane->cursor_move(0, 0);
        stdplane->printf("Got unknown input U+%06x", input);
        nc.render();
        break;
    }
    ncmtx.unlock();
  }
  return gameover || input == 'q';
}

int main(void) {
  if(setlocale(LC_ALL, "") == nullptr){
    return EXIT_FAILURE;
  }
  srand(time(nullptr));
  std::atomic_bool gameover = false;
  notcurses_options ncopts{};
  ncopts.flags = NCOPTION_INHIBIT_SETLOCALE;
  ncpp::NotCurses nc(ncopts);
  {
    Tetris t{nc, gameover};
    std::thread tid(&Tetris::Ticker, &t);
    if(IOLoop(nc, t, gameover)){
      gameover = true; // FIXME signal thread
      tid.join();
    }else{
      return EXIT_FAILURE;
    }
  }
  return nc.stop() ? EXIT_SUCCESS : EXIT_FAILURE;
}
