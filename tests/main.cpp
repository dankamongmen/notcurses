#define DOCTEST_CONFIG_IMPLEMENT
#include <clocale>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <limits.h>
#include <langinfo.h>
#include "version.h"
#include "main.h"

static const char* datadir = NOTCURSES_SHARE;

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
      datadir = strdup(*argv);
      inarg = false;
    }else if(strcmp(*argv, "-p") == 0){
      inarg = true;
    }
    ++argv;
  }
}

// reset the terminal in the event of early exit (notcurses_init() presumably
// ran, but we don't have the notcurses struct with which to run
// notcurses_stop()). so just whip up a new one, and free it immediately.
static void
reset_terminal(){
  notcurses_options nopts{};
  nopts.inhibit_alternate_screen = true;
  auto nc = notcurses_init(&nopts, NULL);
  if(nc){
    notcurses_stop(nc);
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

  // if we exited via REQUIRE(), we likely left the terminal in an invalid
  // state. go ahead and reset it manually.
  if(res){
    reset_terminal();
  }
  return res; // the result from doctest is propagated here as well
}
