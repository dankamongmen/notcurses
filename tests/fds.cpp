#include "main.h"
#include <cerrno>
#include <mutex>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <condition_variable>

std::mutex lock;
std::condition_variable cond;
bool inline_cancelled = false;
bool outofline_cancelled = false;

int testfdcb(struct ncfdplane* ncfd, const void* buf, size_t s, void* curry){
  struct ncplane* n = ncfdplane_plane(ncfd);
  lock.lock();
  if(ncplane_putstr(n, static_cast<const char*>(buf)) <= 0){
    lock.unlock();
    return -1;
  }
  lock.unlock();
  (void)curry;
  (void)s;
  return 0;
}

int testfdeof(struct ncfdplane* n, int fderrno, void* curry){
  lock.lock();
  outofline_cancelled = true;
  lock.unlock();
  cond.notify_one();
  (void)curry;
  (void)n;
  (void)fderrno;
  return 0;
}

int testfdeofdestroys(struct ncfdplane* n, int fderrno, void* curry){
  lock.lock();
  inline_cancelled = true;
  int ret = ncfdplane_destroy(n);
  lock.unlock();
  cond.notify_one();
  (void)curry;
  (void)fderrno;
  return ret;
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

  // destroy the ncfdplane outside of its own context
  SUBCASE("FdPlaneDestroyOffline") {
    REQUIRE(!outofline_cancelled);
    ncfdplane_options opts{};
    int fd = open("/etc/sysctl.conf", O_RDONLY|O_CLOEXEC);
    REQUIRE(0 <= fd);
    auto ncfdp = ncfdplane_create(n_, &opts, fd, testfdcb, testfdeof);
    REQUIRE(ncfdp);
    std::unique_lock<std::mutex> lck(lock);
    CHECK(0 == notcurses_render(nc_));
    while(!outofline_cancelled){
      cond.wait(lck);
    }
    CHECK(0 == ncfdplane_destroy(ncfdp));
    CHECK(0 == notcurses_render(nc_));
    lock.unlock();
  }

  // destroy the ncfdplane within its own context, i.e. from the eof callback
  SUBCASE("FdPlaneDestroyInline") {
    REQUIRE(!inline_cancelled);
    ncfdplane_options opts{};
    opts.curry = n_;
    int fd = open("/etc/sysctl.conf", O_RDONLY|O_CLOEXEC);
    REQUIRE(0 <= fd);
    auto ncfdp = ncfdplane_create(n_, &opts, fd, testfdcb, testfdeofdestroys);
    REQUIRE(ncfdp);
    std::unique_lock<std::mutex> lck(lock);
    CHECK(0 == notcurses_render(nc_));
    while(!inline_cancelled){
      cond.wait(lck);
    }
    CHECK(0 == notcurses_render(nc_));
    lock.unlock();
  }

  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));
}
