#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <ncpp/NotCurses.hh>

static void
usage(std::ostream& os, const char* name, int code){
  os << "usage: " << name << " -h | [ -lR ] paths...\n";
  os << " -d: list directories themselves, not their contents\n";
  os << " -l: use a long listing format\n";
  os << " -R: list subdirectories recursively\n";
  os << " -h: print usage information\n";
  os << std::flush;
  exit(code);
}

int main(int argc, char** argv){
  bool longlisting = false;
  bool recursedirs = false;
  bool directories = false;
  int c;
  while((c = getopt(argc, argv, "dhlR")) != -1){
    switch(c){
      case 'd':
        directories = true;
        break;
      case 'l':
        longlisting = true;
        break;
      case 'R':
        recursedirs = true;
        break;
      case 'h':
        usage(std::cout, argv[0], EXIT_SUCCESS);
        break;
      default:
        usage(std::cerr, argv[0], EXIT_FAILURE);
        break;
    }
  }
  // FIXME if argv[optind] == nullptr, pass "."
  while(argv[optind]){
    std::cout << "arg: " << argv[optind] << std::endl;
    ++optind;
  }
  return EXIT_SUCCESS;
}
