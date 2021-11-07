#include "main.h"
#include "lib/fbuf.h"

TEST_CASE("Fbuf") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  unsigned dimy, dimx;
  struct ncplane* n_ = notcurses_stddim_yx(nc_, &dimy, &dimx);
  REQUIRE(n_);
  REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));

  // check that upon successful initialization, we have some space
  SUBCASE("FbufInit") {
    fbuf f{};
    CHECK(0 == fbuf_init(&f));
    CHECK(0 < f.size);
    CHECK(0 == f.used);
    CHECK(nullptr != f.buf);
    fbuf_free(&f);
  }

  // fill the fbuf a character at a time
  SUBCASE("FbufPutcCover") {
    fbuf f{};
    CHECK(0 == fbuf_init(&f));
    CHECK(0 < f.size);
    CHECK(0 == f.used);
    CHECK(nullptr != f.buf);
    auto oldsize = f.size;
    for(size_t s = 0 ; s < oldsize ; ++s){
      CHECK(1 == fbuf_putc(&f, 'X'));
    }
    CHECK(f.used == oldsize);
    CHECK(oldsize <= f.size);
    fbuf_free(&f);
  }

  // fill the fbuf with one large write
  SUBCASE("FbufPutsCoverSingle") {
    fbuf f{};
    CHECK(0 == fbuf_init(&f));
    CHECK(0 < f.size);
    CHECK(0 == f.used);
    CHECK(nullptr != f.buf);
    auto oldsize = f.size;
    auto erp = new char[oldsize + 1];
    memset(erp, 'X', oldsize);
    erp[oldsize] = '\0';
    CHECK(oldsize == fbuf_puts(&f, erp));
    delete[] erp;
    CHECK(f.used == oldsize);
    CHECK(oldsize <= f.size);
    fbuf_free(&f);
  }

  // fill the fbuf with random writes
  SUBCASE("FbufPutsCoverRandom") {
    fbuf f{};
    CHECK(0 == fbuf_init(&f));
    CHECK(0 < f.size);
    CHECK(0 == f.used);
    CHECK(nullptr != f.buf);
    auto oldsize = f.size;
    auto erp = new char[oldsize + 1];
    size_t used = 0;
    while(used < oldsize){
      size_t oldused = f.used;
      size_t towrite = rand() % (oldsize - used) + 1;
      memset(erp, rand() % 26 + 'A', towrite);
      erp[towrite] = '\0';
      CHECK(towrite == fbuf_puts(&f, erp));
      CHECK(f.used == oldused + towrite);
      used += towrite;
    }
    delete[] erp;
    CHECK(f.used == oldsize);
    CHECK(oldsize <= f.size);
    fbuf_free(&f);
  }

  CHECK(0 == notcurses_stop(nc_));
}
