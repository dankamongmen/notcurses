#include <cstdio>
#include <cstdlib>
#include <clocale>
#include <cassert>
#include <memory>
#include <ncpp/NotCurses.hh>
#include <ncpp/Plane.hh>

using namespace ncpp;

// What happens when we print over half of a wide glyph?

auto main() -> int {
  setlocale(LC_ALL, "");
  notcurses_options nopts{};
  nopts.flags = NCOPTION_INHIBIT_SETLOCALE | NCOPTION_NO_ALTERNATE_SCREEN;
  NotCurses nc(nopts);
  std::shared_ptr<Plane> n(nc.get_stdplane());
  int dimx, dimy;
  n->get_dim(&dimy, &dimx);
  Cell c;
  c.set_bg_rgb8(0, 0x80, 0);
  //n->set_default(c);
  if(n->load(c, "🐳") < 0){
    goto err;
  }
  for(int i = 0 ; i < dimy ; ++i){
    for(int j = 8 ; j < dimx / 2 ; ++j){ // leave some empty spaces
      if(n->putc(i, j * 2, &c) < 0){
        goto err;
      }
    }
  }
  n->putc(dimy, dimx - 3, &c);
  n->putc(dimy, dimx - 1, &c);
  n->putc(dimy + 1, dimx - 2, &c);
  n->putc(dimy + 1, dimx - 4, &c);
  n->release(c);
  // put these on the right side of the wide glyphs
  for(int i = 0 ; i < dimy / 2 ; ++i){
    for(int j = 5 ; j < dimx / 2 ; j += 2){
      if(n->putc(i, j, (j % 10) + '0') < 0){
        goto err;
      }
    }
  }
  // put these on the left side of the wide glyphs
  for(int i = dimy / 2 ; i < dimy ; ++i){
    for(int j = 4 ; j < dimx / 2 ; j += 2){
      if(n->putc(i, j, (j % 10) + '0') < 0){
        goto err;
      }
    }
  }
  if(!nc.render()){
    goto err;
  }
  return !nc.stop() ? EXIT_FAILURE : EXIT_SUCCESS;

err:
  return EXIT_FAILURE;
}
