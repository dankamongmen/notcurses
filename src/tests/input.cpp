#include "main.h"
#include "lib/in.h"

// a control sequence (e.g. a DA1 reply) can be dribbled out by the terminal
// across multiple read()s. while we're still interrogating, or when we're
// resuming an escape begun in a prior read, the partial sequence must be
// awaited (as opposed to being reinterpreted as standalone input)
// or we lose the reply and hang notcurses_init() forever
// (seen under WSL in Windows Terminal ConPTY).
TEST_CASE("MidEscapeStandaloneInput") {
  // a lone Escape filling the whole buffer (fresh, not interrogating) is the
  // canonical "treat as input" case: keep it working.
  CHECK(midescape_standalone_input_p(1, -1, 1, 0, false));
  // an escape resuming from a prior read (continuation != 0) is never input...
  CHECK_FALSE(midescape_standalone_input_p(32, -32, 32, 16, false));
  // ...nor is one still in flight while we await interrogation replies.
  CHECK_FALSE(midescape_standalone_input_p(32, -32, 32, 0, true));
  // a fresh, post-interrogation escape at the buffer start is still input.
  CHECK(midescape_standalone_input_p(32, -32, 32, 0, false));
}

TEST_CASE("Input") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  unsigned dimy, dimx;
  struct ncplane* n_ = notcurses_stddim_yx(nc_, &dimy, &dimx);
  REQUIRE(n_);


  CHECK(0 == notcurses_render(nc_));
  CHECK(0 == notcurses_stop(nc_));

}
