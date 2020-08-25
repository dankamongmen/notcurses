#include <unistd.h>
#include <cstdlib>
#include <clocale>
#include <iostream>
#include <ncpp/Reader.hh>
#include <ncpp/NotCurses.hh>

using namespace ncpp;

auto main() -> int {
  if(!setlocale(LC_ALL, "")){
    std::cout << "Error setting locale\n";
    return EXIT_FAILURE;
  }
  notcurses_options nopts{};
  nopts.flags = NCOPTION_INHIBIT_SETLOCALE;
  NotCurses nc(nopts);
  int dimy, dimx;
  std::unique_ptr<Plane *> n = std::make_unique<Plane *>(nc.get_stdplane(&dimy, &dimx));
  nc.get_term_dim(&dimy, &dimx);
  ncreader_options opts{};
  opts.physrows = dimy / 8;
  opts.physcols = dimx / 2;
  opts.egc = "░";
  // FIXME c++ is crashing
  //Reader nr(nc, 0, 0, &opts);
  auto nr = ncreader_create(**n, 2, 2, &opts);
  if(nr == nullptr){
    return EXIT_FAILURE;
  }
  if(!nc.cursor_enable(2, 2)){
    return EXIT_FAILURE;
  }
  ncinput ni;
  nc.render();
  while(nc.getc(true, &ni) != (char32_t)-1){
    if(!ncreader_offer_input(nr, &ni)){
      break;
    }
    int y, x;
    ncplane_cursor_yx(ncreader_plane(nr), &y, &x);
    nc.cursor_move_yx(y + 2, x + 2);
    nc.render();
  }
  nc.render();
  char* contents;
  ncreader_destroy(nr, &contents);
  nc.stop();
  if(contents){
    printf("\n input: %s\n", contents);
  }
  return EXIT_SUCCESS;
}
