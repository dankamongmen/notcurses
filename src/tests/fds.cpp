#include "main.h"
#include <cerrno>
#include <mutex>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include "internal.h"
#include <condition_variable>

static std::mutex lock;
static std::condition_variable cond;
static bool inline_cancelled = false;

auto testfdcb(struct ncfdplane* ncfd, const void* buf, size_t s, void* curry) -> int {
  struct ncplane* n = ncfdplane_plane(ncfd);
  std::lock_guard<std::mutex> lck(lock);
  if(ncplane_putnstr(n, s, static_cast<const char*>(buf)) <= 0){
    return -1;
  }
  notcurses_render(ncplane_notcurses(ncfdplane_plane(ncfd)));
  (void)curry;
  (void)s;
  return 0;
}

auto testfdeof(struct ncfdplane* n, int fderrno, void* curry) -> int {
  std::unique_lock<std::mutex> lck(lock);
  bool* outofline_cancelled = static_cast<bool*>(curry);
  *outofline_cancelled = true;
  lck.unlock();
  cond.notify_one();
  (void)n;
  (void)fderrno;
  return 0;
}

auto testfdeofdestroys(struct ncfdplane* n, int fderrno, void* curry) -> int {
  std::unique_lock<std::mutex> lck(lock);
  inline_cancelled = true;
  int ret = ncfdplane_destroy(n);
  lck.unlock();
  cond.notify_one();
  (void)curry;
  (void)fderrno;
  return ret;
}

// test ncfdplanes and ncsubprocs
TEST_CASE("FdsAndSubprocs"
          * doctest::description("Fdplanes and subprocedures")) {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
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
    lck.unlock();
    CHECK(0 == ncfdplane_destroy(ncfdp));
    CHECK(0 == notcurses_render(nc_));
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
    lck.unlock();
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("SubprocDestroyCmdExecFails") {
    char * const argv[] = { strdup("/should-not-exist"), nullptr, };
    bool outofline_cancelled = false;
    ncsubproc_options opts{};
    opts.curry = &outofline_cancelled;
    auto ncsubp = ncsubproc_createvp(n_, &opts, argv[0], argv, testfdcb, testfdeof);
    REQUIRE(ncsubp);
    std::unique_lock<std::mutex> lck(lock);
    CHECK(0 == notcurses_render(nc_));
    while(!outofline_cancelled){
      cond.wait(lck);
    }
    lck.unlock();
    CHECK(0 != ncsubproc_destroy(ncsubp));
    // FIXME we ought get indication of an error here! or via callback...
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("SubprocDestroyCmdSucceeds") {
    char * const argv[] = { strdup("/bin/cat"), strdup("/dev/null"), nullptr, };
    bool outofline_cancelled = false;
    ncsubproc_options opts{};
    opts.curry = &outofline_cancelled;
    auto ncsubp = ncsubproc_createvp(n_, &opts, argv[0], argv, testfdcb, testfdeof);
    REQUIRE(ncsubp);
    std::unique_lock<std::mutex> lck(lock);
    CHECK(0 == notcurses_render(nc_));
    while(!outofline_cancelled){
      cond.wait(lck);
    }
    lck.unlock();
    CHECK(0 == ncsubproc_destroy(ncsubp));
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("SubprocDestroyCmdFailed") {
    char * const argv[] = { strdup("/bin/cat"), strdup("/dev/nope"), nullptr, };
    bool outofline_cancelled = false;
    ncsubproc_options opts{};
    opts.curry = &outofline_cancelled;
    auto ncsubp = ncsubproc_createvp(n_, &opts, argv[0], argv, testfdcb, testfdeof);
    REQUIRE(ncsubp);
    std::unique_lock<std::mutex> lck(lock);
    CHECK(0 == notcurses_render(nc_));
    while(!outofline_cancelled){
      cond.wait(lck);
    }
    lck.unlock();
    CHECK(0 != ncsubproc_destroy(ncsubp));
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("SubprocDestroyCmdHung") {
    char * const argv[] = { strdup("/bin/cat"), nullptr, };
    bool outofline_cancelled = false;
    ncsubproc_options opts{};
    opts.curry = &outofline_cancelled;
    auto ncsubp = ncsubproc_createvp(n_, &opts, argv[0], argv, testfdcb, testfdeof);
    REQUIRE(ncsubp);
    // FIXME ought be CHECK, breaking in drone
    WARN(0 != ncsubproc_destroy(ncsubp));
    CHECK(0 == notcurses_render(nc_));
  }

  CHECK(0 == notcurses_stop(nc_));
}
