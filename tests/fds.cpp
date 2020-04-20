#include "main.h"
#include <cerrno>
#include <mutex>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

std::mutex lock;

int passwdcb(struct notcurses* nc, const void* buf, size_t s, void* curry){
  struct ncplane* n_ = static_cast<struct ncplane*>(curry);
  lock.lock();
fprintf(stderr, "START HERE WITH %zu\n", s);
  if(ncplane_putstr(n_, static_cast<const char*>(buf)) <= 0){
fprintf(stderr, "FAILED HERE WITH %zu\n", s);
    lock.unlock();
    return -1;
  }
  lock.unlock();
fprintf(stderr, "DONE HERE WITH %zu\n", s);
  return 0;
}

int passwdeof(struct notcurses* nc, int fderrno, void* curry){
  return 0;
}

// test ncfdplanes and ncsubprocs
TEST_CASE("FdsAndSubprocs") {
  if(getenv("TERM") == nullptr){
    return;
  }
  notcurses_options nopts{};
  nopts.inhibit_alternate_screen = true;
  nopts.suppress_banner = true;
  FILE* outfp_ = fopen("/dev/tty", "wb");
  REQUIRE(outfp_);
  struct notcurses* nc_ = notcurses_init(&nopts, outfp_);
  REQUIRE(nc_);
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);
  REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));

  SUBCASE("FdPlanePasswd") {
    ncfdplane_options opts{};
    opts.curry = n_;
    int fd = open("/etc/sysctl.conf", O_RDONLY|O_CLOEXEC);
    REQUIRE(0 <= fd);
    auto ncfdp = ncfdplane_create(n_, &opts, fd, passwdcb, passwdeof);
    REQUIRE(ncfdp);
    lock.lock();
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncfdplane_destroy(ncfdp));
    CHECK(0 == notcurses_render(nc_));
    lock.unlock();
  }

  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));
}
