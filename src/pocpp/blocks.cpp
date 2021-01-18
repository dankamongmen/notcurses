#include <cstdio>
#include <cstdlib>
#include <clocale>
#include <cassert>
#include <memory>
#include <unistd.h>
#include <ncpp/NotCurses.hh>

using namespace ncpp;

auto main() -> int {
  setlocale(LC_ALL, "");
  notcurses_options nopts{};
  nopts.flags = NCOPTION_INHIBIT_SETLOCALE | NCOPTION_NO_ALTERNATE_SCREEN;
  NotCurses nc(nopts);
  {
    std::unique_ptr<Plane> n (nc.get_stdplane());
    int dimx, dimy;
    nc.get_term_dim(&dimy, &dimx);

    const int HEIGHT = 5;
    const int WIDTH = 20;
    auto na = std::make_unique<Plane>(n.get(), HEIGHT, WIDTH,
                    dimy - (HEIGHT + 1),
                    NCAlign::Center);
    uint64_t channels = 0;
    if(!na){
      goto err;
    }
    n->set_fg_rgb(0x00ff00);
    na->set_fg_rgb(0x00ff00);
    if(!na->cursor_move(0, 0)){
      goto err;
    }
    if(!na->rounded_box_sized(0, channels, HEIGHT, WIDTH, 0)){
      goto err;
    }
    if(n->printf(0, 0, "arrrrp?") < 0){
      goto err;
    }
    if(!n->rounded_box_sized(4, channels, HEIGHT, WIDTH, 0)){
      goto err;
    }
    if(!nc.render()){
      goto err;
    }
  }
  sleep(1);
  return nc.stop() ? EXIT_SUCCESS : EXIT_FAILURE;

err:
  return EXIT_FAILURE;
}
