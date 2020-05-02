#include "main.h"
#include <cerrno>
#include <mutex>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include "internal.h"
#include <condition_variable>

std::mutex lock;
std::condition_variable cond;
bool inline_cancelled = false;

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
  bool* outofline_cancelled = static_cast<bool*>(curry);
  lock.lock();
  *outofline_cancelled = true;
  lock.unlock();
  cond.notify_one();
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
    bool outofline_cancelled = false;
    ncfdplane_options opts{};
    opts.curry = &outofline_cancelled;
    int fd = open("/dev/null", O_RDONLY|O_CLOEXEC);
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
    inline_cancelled = false;
    ncfdplane_options opts{};
    opts.curry = n_;
    int fd = open("/dev/null", O_RDONLY|O_CLOEXEC);
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

  SUBCASE("SubprocDestroyCmdExecFails") {
    char * const argv[] = { strdup("/dev/nope"), NULL, };
    bool outofline_cancelled = false;
    ncsubproc_options opts{};
    opts.popts.curry = &outofline_cancelled;
    auto ncsubp = ncsubproc_createvp(n_, &opts, argv[0], argv, testfdcb, testfdeof);
    REQUIRE(ncsubp);
    std::unique_lock<std::mutex> lck(lock);
    CHECK(0 == notcurses_render(nc_));
    while(!outofline_cancelled){
      cond.wait(lck);
    }
    CHECK(0 == ncsubproc_destroy(ncsubp));
    CHECK(0 == notcurses_render(nc_));
    lock.unlock();
  }

  // FIXME SIGCHLD seems to blow up doctest...
  SUBCASE("SubprocDestroyCmdSucceeds") {
    char * const argv[] = { strdup("/bin/cat"), strdup("/dev/null"), NULL, };
    bool outofline_cancelled = false;
    ncsubproc_options opts{};
    opts.popts.curry = &outofline_cancelled;
    auto ncsubp = ncsubproc_createvp(n_, &opts, argv[0], argv, testfdcb, testfdeof);
    REQUIRE(ncsubp);
    std::unique_lock<std::mutex> lck(lock);
    CHECK(0 == notcurses_render(nc_));
    while(!outofline_cancelled){
      cond.wait(lck);
    }
    CHECK(0 == ncsubproc_destroy(ncsubp));
    CHECK(0 == notcurses_render(nc_));
    lock.unlock();
  }

  SUBCASE("SubprocDestroyCmdFailed") {
    char * const argv[] = { strdup("/bin/cat"), strdup("/dev/nope"), NULL, };
    bool outofline_cancelled = false;
    ncsubproc_options opts{};
    opts.popts.curry = &outofline_cancelled;
    auto ncsubp = ncsubproc_createvp(n_, &opts, argv[0], argv, testfdcb, testfdeof);
    REQUIRE(ncsubp);
    std::unique_lock<std::mutex> lck(lock);
    CHECK(0 == notcurses_render(nc_));
    while(!outofline_cancelled){
      cond.wait(lck);
    }
    CHECK(0 == ncsubproc_destroy(ncsubp));
    CHECK(0 == notcurses_render(nc_));
    lock.unlock();
  }

  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));
}
