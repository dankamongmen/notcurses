#define DOCTEST_CONFIG_IMPLEMENT
#include "main.h"
#include <fcntl.h>
#include <clocale>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <climits>
#include <sys/stat.h>

const char* datadir = notcurses_data_dir();

// we define loglevel for any use of logging in internal header files.
// note that this has no bearing on the library's true inner loglevel!
ncloglevel_e loglevel;

auto testing_notcurses() -> struct notcurses* {
  notcurses_options nopts{};
  // get loglevel from command line. enabling it by default leads to
  // more confusion than useful information, so leave it off by default.
  nopts.loglevel = loglevel;
  nopts.flags = NCOPTION_SUPPRESS_BANNERS
                | NCOPTION_NO_ALTERNATE_SCREEN
                | NCOPTION_DRAIN_INPUT;
  auto nc = notcurses_init(&nopts, nullptr);
  return nc;
}

template <typename T> using uniqptr = std::unique_ptr<T,free_deleter>;

auto find_data(const char* datum) -> uniqptr<char> {
  std::string s = datadir;
  s += path_separator();
  s += datum;
  uniqptr<char> uptr(strdup(s.c_str()));
  return uptr;
}

auto is_test_tty() -> bool {
  int fd = open("/dev/tty", O_RDWR);
  if(fd < 0){
    return false;
  }
  close(fd);
  return true;
}

static void
handle_opts(const char** argv){
  // now that we've spun up one testing framework, switch to _SILENT unless
  // something else has been provided on the command line.
  loglevel = NCLOGLEVEL_SILENT;
  bool inarg = false;
  while(*argv){
    if(inarg){
      datadir = strdup(*argv);
      inarg = false;
    }else if(strcmp(*argv, "-p") == 0){
      inarg = true;
    }else if(strncmp(*argv, "-l", 2) == 0){ // just require -l
      char* eol;
      long ll = strtol(*argv + 2, &eol, 0);
      if(ll < NCLOGLEVEL_SILENT || ll > NCLOGLEVEL_TRACE){
        std::cerr << "illegal loglevel: " << *argv + 2 << std::endl;
        exit(EXIT_FAILURE);
      }
      if(*eol){
        std::cerr << "illegal loglevel: " << *argv + 2 << std::endl;
        exit(EXIT_FAILURE);
      }
      std::cout << "got loglevel " << ll << std::endl;
      loglevel = static_cast<ncloglevel_e>(ll);
    }
    ++argv;
  }
  auto nc = testing_notcurses();
  if(!nc){
    std::cerr << "Couldn't create notcurses testing framework" << std::endl;
    exit(EXIT_FAILURE);
  }
  unsigned dimy, dimx;
  notcurses_stddim_yx(nc, &dimy, &dimx);
  std::cout << "Detected cell geometry: " << dimx << 'x' << dimy << std::endl;
  notcurses_stop(nc);
  if(dimx < 50 || dimy < 24){ // minimum assumed geometry
    std::cerr << "Terminal was too small for tests (minimum 50x24)" << std::endl;
    exit(EXIT_FAILURE);
  }
}

// check that the (provided or default) data directory exists, and has at
// least one of our necessary files. otherwise, print a warning + return error.
static int
check_data_dir(){
  auto p = find_data("changes.jpg");
  if(!p){
    std::cerr << "Coudln't find testing data! Supply directory with -p." << std::endl;
    return -1;
  }
  struct stat s;
  if(stat(p.get(), &s)){
    std::cerr << "Couldn't open " << p.get() << ". Supply directory with -p." << std::endl;
    return -1;
  }
  return 0;
}

// reset the terminal in the event of early exit (notcurses_init() presumably
// ran, but we don't have the notcurses struct to destroy, so just do it raw).
static void
reset_terminal(){
  int fd = open("/dev/tty", O_RDWR|O_CLOEXEC);
  if(fd >= 0){
    struct termios tios;
    if(tcgetattr(fd, &tios) == 0){
      tios.c_iflag |= INLCR;
      tios.c_lflag |= ISIG | ICANON | ECHO;
      tcsetattr(fd, TCSADRAIN, &tios);
    }
    printf(KKEYBOARD_POP);
    char* str = tigetstr("sgr0");
    if(str != (char*)-1){
      printf("%s", str);
    }
    fflush(stdout);
    str = tigetstr("oc");
    if(str != (char*)-1){
      printf("%s", str);
    }
    fflush(stdout);
    str = tigetstr("cnorm");
    if(str != (char*)-1){
      printf("%s", str);
    }
    fflush(stdout);
    close(fd);
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
      vec.push_back(nullptr);
  }

  auto argc() -> int { return static_cast<int>(vec.size()) - 1; }
  auto argv() -> const char** { return &vec[0]; }
};

auto lang_and_term() -> void {
  const char* lang = getenv("LANG");
  if(lang == nullptr){
    std::cerr << "Warning: LANG wasn't defined" << std::endl;
  }else{
    std::cout << "Running with LANG=" << lang << std::endl;
  }
#ifndef __MINGW32__
  const char* term = getenv("TERM");
  // ubuntu's buildd sets TERM=unknown, fuck it, handle this atrocity
  if(term == nullptr || strcmp(term, "unknown") == 0){
    std::cerr << "TERM wasn't defined, exiting with success" << std::endl;
    exit(EXIT_SUCCESS);
  }
  std::cout << "Running with TERM=" << term << std::endl;
#endif
}

auto main(int argc, const char **argv) -> int {
  if(!setlocale(LC_ALL, "")){
    std::cerr << "Couldn't set locale based on user preferences!" << std::endl;
    return EXIT_FAILURE;
  }
  lang_and_term(); // might exit
  doctest::Context context;

  context.setOption("order-by", "name"); // sort the test cases by their name
  context.applyCommandLine(argc, argv);
  context.setOption("no-breaks", true); // don't break in the debugger when assertions fail
  dt_removed args(argv);
  handle_opts(argv);
  int res = context.run(); // run
  reset_terminal();
  if(res){
    check_data_dir();
  }
  if(context.shouldExit()){ // important - query flags (and --exit) rely on the user doing this
    return res;             // propagate the result of the tests
  }
  return res; // the result from doctest is propagated here as well
}
