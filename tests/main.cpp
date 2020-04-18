#define DOCTEST_CONFIG_IMPLEMENT
#include <clocale>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <limits.h>
#include <langinfo.h>
#include "main.h"

static char datadir[PATH_MAX + 1] = "/usr/share/notcurses"; // FIXME

bool enforce_utf8(){
  char* enc = nl_langinfo(CODESET);
  if(!enc){
    return false;
  }
  if(strcmp(enc, "UTF-8")){
    return false;
  }
  return true;
}

char* find_data(const char* datum){
  char* path = (char*)malloc(strlen(datadir) + 1 + strlen(datum) + 1);
  strcpy(path, datadir);
  strcat(path, "/");
  strcat(path, datum);
  return path;
}

static void
handle_opts(const char** argv){
  bool inarg = false;
  while(*argv){
    if(inarg){
      strncpy(datadir, *argv, sizeof(datadir) - 1);
      inarg = false;
    }else if(strcmp(*argv, "-p") == 0){
      inarg = true;
    }
    ++argv;
  }
}

// from https://github.com/onqtam/doctest/blob/master/doc/markdown/commandline.md
class dt_removed {
  std::vector<const char*> vec;
public:
  dt_removed(const char** argv_in) {
      for(; *argv_in; ++argv_in)
          if(strncmp(*argv_in, "--dt-", strlen("--dt-")) != 0)
              vec.push_back(*argv_in);
      vec.push_back(NULL);
  }

  int          argc() { return static_cast<int>(vec.size()) - 1; }
  const char** argv() { return &vec[0]; } // Note: non-const char **:
};

int main(int argc, const char **argv){
  if(!setlocale(LC_ALL, "")){
    std::cerr << "Coudln't set locale based on user preferences!" << std::endl;
    return EXIT_FAILURE;
  }
  if(!enforce_utf8()){
    return EXIT_SUCCESS; // hrmmm
  }
  doctest::Context context;

  // defaults
  context.setOption("order-by", "name");            // sort the test cases by their name

  context.applyCommandLine(argc, argv);

  // overrides
  context.setOption("no-breaks", true);             // don't break in the debugger when assertions fail

  dt_removed args(argv);
  handle_opts(argv);

  int res = context.run(); // run

  if(context.shouldExit()){ // important - query flags (and --exit) rely on the user doing this
    return res;             // propagate the result of the tests
  }

  int client_stuff_return_code = 0;
  // your program - if the testing framework is integrated in your production code

  return res + client_stuff_return_code; // the result from doctest is propagated here as well
}
