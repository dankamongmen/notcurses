#include <unistd.h>
#include <cstdlib>
#include <clocale>
#include <iostream>
#include <ncpp/Reader.hh>
#include <ncpp/NotCurses.hh>

auto main() -> int {
  if(!setlocale(LC_ALL, "")){
    std::cout << "Error setting locale\n";
    return EXIT_FAILURE;
  }
  ncpp::NotCurses nc;
  int dimy, dimx;
  nc.get_term_dim(&dimy, &dimx);
  ncreader_options opts{};
  opts.physrows = dimy / 2;
  opts.physcols = dimx / 2;
  opts.egc = strdup("░");
  //ncpp::Reader nr(nc, 0, 0, &opts);
  auto nr = ncreader_create(*nc, 2, 2, &opts);
  if(nr == nullptr){
    return EXIT_FAILURE;
  }
  char32_t id;
  ncinput ni;
  nc.render();
  while((id = nc.getc(true, &ni)) != (char32_t)-1){
    if(!ncreader_offer_input(nr, &ni)){
      break;
    }
  }
  nc.render();
  nc.stop();
  return EXIT_SUCCESS;
}
