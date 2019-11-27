#include <cstdlib>
#include <libgen.h>
#include <iostream>

void usage(std::ostream& o, char* argv0, int exitcode){
  o << "usage: " << basename(argv0) << " files" << '\n';
  exit(exitcode);
}

int main(int argc, char** argv){
  if(argc == 1){
    usage(std::cerr, argv[0], EXIT_FAILURE);
  }
  for(int i = 1 ; i < argc ; ++i){
    std::cout << "file: " << argv[i] << std::endl;
  }
  return EXIT_SUCCESS;
}
