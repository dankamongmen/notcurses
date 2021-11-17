#include "main.h"
#include <cerrno>
#include <mutex>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <condition_variable>

static pthread_cond_t cond;
static pthread_mutex_t lock;

auto testfdcb(struct ncfdplane* ncfd, const void* buf, size_t s, void* curry) -> int {
  struct ncplane* n = ncfdplane_plane(ncfd);
  pthread_mutex_lock(&lock);
  if(ncplane_putnstr(n, s, static_cast<const char*>(buf)) <= 0){
    pthread_mutex_unlock(&lock);
    return -1;
  }
  notcurses_render(ncplane_notcurses(ncfdplane_plane(ncfd)));
  pthread_mutex_unlock(&lock);
  (void)curry;
  (void)s;
  return 0;
}

auto testfdeof(struct ncfdplane* n, int fderrno, void* curry) -> int {
  bool* outofline_cancelled = static_cast<bool*>(curry);
  pthread_mutex_lock(&lock);
  *outofline_cancelled = true;
  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&lock);
  (void)n;
  (void)fderrno;
  return 0;
}

auto testfdeofdestroys(struct ncfdplane* n, int fderrno, void* curry) -> int {
  bool* inline_cancelled = static_cast<bool*>(curry);
  pthread_mutex_lock(&lock);
  int ret = ncfdplane_destroy(n);
  *inline_cancelled = true;
  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&lock);
  (void)fderrno;
  return ret;
}

// test ncfdplanes and ncsubprocs
TEST_CASE("FdsAndSubprocs"
          * doctest::description("Fdplanes and subprocedures")) {
  REQUIRE(0 == pthread_cond_init(&cond, NULL));
  REQUIRE(0 == pthread_mutex_init(&lock, NULL));
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
    pthread_mutex_lock(&lock);
    CHECK(0 == notcurses_render(nc_));
    while(!outofline_cancelled){
      pthread_cond_wait(&cond, &lock);
    }
    pthread_mutex_unlock(&lock);
    CHECK(0 == ncfdplane_destroy(ncfdp));
    CHECK(0 == notcurses_render(nc_));
  }

  // destroy the ncfdplane within its own context, i.e. from the eof callback
  SUBCASE("FdPlaneDestroyInline") {
    bool inline_cancelled = false;
    ncfdplane_options opts{};
    opts.curry = &inline_cancelled;
    int fd = open("/dev/null", O_RDONLY|O_CLOEXEC);
    REQUIRE(0 <= fd);
    auto ncfdp = ncfdplane_create(n_, &opts, fd, testfdcb, testfdeofdestroys);
    REQUIRE(ncfdp);
    pthread_mutex_lock(&lock);
    CHECK(0 == notcurses_render(nc_));
    while(!inline_cancelled){
      pthread_cond_wait(&cond, &lock);
    }
    pthread_mutex_unlock(&lock);
    CHECK(0 == notcurses_render(nc_));
  }

  /*
  SUBCASE("SubprocDestroyCmdExecFails") {
    char * const argv[] = { "/should-not-exist", nullptr, };
    bool outofline_cancelled = false;
    ncsubproc_options opts{};
    opts.curry = &outofline_cancelled;
    auto ncsubp = ncsubproc_createvp(n_, &opts, argv[0], argv, testfdcb, testfdeof);
    REQUIRE(ncsubp);
    pthread_mutex_lock(&lock);
    CHECK(0 == notcurses_render(nc_));
    while(!outofline_cancelled){
      pthread_cond_wait(&cond, &lock);
    }
    lck.unlock();
    CHECK(0 != ncsubproc_destroy(ncsubp));
    // FIXME we ought get indication of an error here! or via callback...
    CHECK(0 == notcurses_render(nc_));
  }
  */

  SUBCASE("SubprocDestroyCmdSucceeds") {
    char const * const argv[] = { "/bin/cat", "/dev/null", nullptr, };
    bool outofline_cancelled = false;
    ncsubproc_options opts{};
    opts.curry = &outofline_cancelled;
    auto ncsubp = ncsubproc_createvp(n_, &opts, argv[0], argv, testfdcb, testfdeof);
    REQUIRE(ncsubp);
    pthread_mutex_lock(&lock);
    CHECK(0 == notcurses_render(nc_));
    while(!outofline_cancelled){
      pthread_cond_wait(&cond, &lock);
    }
    pthread_mutex_unlock(&lock);
    CHECK(0 == ncsubproc_destroy(ncsubp));
    CHECK(0 == notcurses_render(nc_));
  }

  // assuming the path /dev/nope doesn't exist, cat ought be successfully
  // launched (fork() and exec() both succeed), but then immediately fail.
  SUBCASE("SubprocDestroyCmdFailed") {
    char const * const argv[] = { "/bin/cat", "/dev/nope", nullptr, };
    bool outofline_cancelled = false;
    ncsubproc_options opts{};
    opts.curry = &outofline_cancelled;
    auto ncsubp = ncsubproc_createvp(n_, &opts, argv[0], argv, testfdcb, testfdeof);
    REQUIRE(ncsubp);
    pthread_mutex_lock(&lock);
    CHECK(0 == notcurses_render(nc_));
    while(!outofline_cancelled){
      pthread_cond_wait(&cond, &lock);
    }
    pthread_mutex_unlock(&lock);
    CHECK(0 != ncsubproc_destroy(ncsubp));
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("SubprocDestroyCmdHung") {
    char const * const argv[] = { "/bin/cat", nullptr, };
    bool outofline_cancelled = false;
    ncsubproc_options opts{};
    opts.curry = &outofline_cancelled;
    auto ncsubp = ncsubproc_createvp(n_, &opts, argv[0], argv, testfdcb, testfdeof);
    REQUIRE(ncsubp);
    WARN(0 != ncsubproc_destroy(ncsubp)); // FIXME
    CHECK(0 == notcurses_render(nc_));
  }

  CHECK(0 == pthread_cond_destroy(&cond));
  // FIXME why does this (very rarely) fail? ugh
  WARN(0 == pthread_mutex_destroy(&lock));

  CHECK(0 == notcurses_stop(nc_));
}
