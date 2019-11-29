#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <clocale>
#include <iostream>
#include <notcurses.h>

int main(void){
  if(setlocale(LC_ALL, "") == nullptr){
    return EXIT_FAILURE;
  }
  notcurses_options opts{};
  opts.outfp = stdout;
  struct notcurses* nc = notcurses_init(&opts);
  if(nc == nullptr){
    return EXIT_FAILURE;;
  }
  int r;
  cell c = CELL_TRIVIAL_INITIALIZER;
  while((r = notcurses_getc_blocking(nc, &c)) >= 0){
    if(r == 0){ // interrupted by signal
      continue;
    }
  }
  int e = errno;
  notcurses_stop(nc);
  std::cerr << "Error reading from terminal (" << strerror(e) << "?)\n";
  return EXIT_FAILURE;
}
