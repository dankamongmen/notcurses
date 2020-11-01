#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <ncpp/NotCurses.hh>

static void
usage(std::ostream& os, const char* name, int code){
  os << "usage: " << name << " -h | paths...\n";
  os << " -h: print usage information\n";
  os << std::flush;
  exit(code);
}

int main(int argc, char** argv){
  int c;
  while((c = getopt(argc, argv, "h")) != -1){
    switch(c){
      case 'h':
        usage(std::cout, argv[0], EXIT_SUCCESS);
        break;
      default:
        usage(std::cerr, argv[0], EXIT_FAILURE);
        break;
    }
  }
  while(argv[optind]){
    std::cout << "arg: " << argv[optind] << std::endl;
    ++optind;
  }
  return EXIT_SUCCESS;
}
