#include <cstdlib>
#include "main.h"

constexpr int TABWIDTH = 8;

TEST_CASE("TaBs") { // refreshing and delicious
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

  SUBCASE("PutcTaB") {
    struct ncplane_options nopts{};
    nopts.rows = 2;
    nopts.cols = 80;
    auto n = ncplane_create(n_, &nopts);
    unsigned y, x;
    CHECK(TABWIDTH == ncplane_putchar(n, '\t'));
    ncplane_cursor_yx(n, &y, &x);
    CHECK(y == 0);
    CHECK(x == TABWIDTH);
    char* c = ncplane_at_yx(n, 0, 0, nullptr, nullptr);
    REQUIRE(c);
    CHECK(0 == strcmp(c, " "));
    free(c);
    for(unsigned i = 1 ; i < x ; ++i){
      nccell nc;
      CHECK(1 == ncplane_at_yx_cell(n, 0, i, &nc));
      CHECK(nccell_wide_right_p(&nc));
      nccell_release(n, &nc);
    }
  }

  SUBCASE("PutXoffsetTaBs") {
    struct ncplane_options nopts{};
    nopts.rows = 2;
    nopts.cols = TABWIDTH;
    auto n = ncplane_create(n_, &nopts);
    unsigned y, x;
    for(int i = 0 ; i < TABWIDTH ; ++i){
      CHECK(TABWIDTH - i == ncplane_putchar(n, '\t'));
      ncplane_cursor_yx(n, &y, &x);
      CHECK(y == 0);
      CHECK(x == TABWIDTH - i);
      CHECK(0 == ncplane_cursor_move_yx(n, 0, i + 1));
    }
    for(unsigned i = 0 ; i < x ; ++i){
      char* c = ncplane_at_yx(n, 0, i, nullptr, nullptr);
      REQUIRE(c);
      CHECK(0 == strcmp(c, " "));
      free(c);
    }
  }

  SUBCASE("PutwcTaB") {
    struct ncplane_options nopts{};
    nopts.rows = 2;
    nopts.cols = TABWIDTH;
    auto n = ncplane_create(n_, &nopts);
    unsigned y, x;
    CHECK(TABWIDTH == ncplane_putwc(n, L'\t'));
    ncplane_cursor_yx(n, &y, &x);
    CHECK(y == 0);
    CHECK(x == TABWIDTH);
    for(unsigned i = 0 ; i < x ; ++i){
      char* c = ncplane_at_yx(n, 0, i, nullptr, nullptr);
      REQUIRE(c);
      CHECK(0 == strcmp(c, " "));
      free(c);
    }
  }

  SUBCASE("PutCellTaB") {
    struct ncplane_options nopts{};
    nopts.rows = 2;
    nopts.cols = TABWIDTH;
    auto n = ncplane_create(n_, &nopts);
    nccell c = NCCELL_CHAR_INITIALIZER('\t');
    unsigned y, x;
    CHECK(1 == ncplane_putc(n, &c));
    ncplane_cursor_yx(n, &y, &x);
    CHECK(y == 0);
    CHECK(x == TABWIDTH);
    for(unsigned i = 0 ; i < x ; ++i){
      char* s = ncplane_at_yx(n, 0, i, nullptr, nullptr);
      REQUIRE(s);
      CHECK(0 == strcmp(s, " "));
      free(s);
    }
  }

  SUBCASE("PutMultipleTaBs") {
    struct ncplane_options nopts{};
    nopts.rows = 2;
    nopts.cols = TABWIDTH;
    auto n = ncplane_create(n_, &nopts);
    unsigned y, x;
    CHECK(1 == ncplane_putstr(n, "\t\t"));
    ncplane_cursor_yx(n, &y, &x);
    CHECK(y == 0);
    CHECK(x == 16);
    for(unsigned i = 0 ; i < x ; ++i){
      char* c = ncplane_at_yx(n, 0, i, nullptr, nullptr);
      REQUIRE(c);
      CHECK(0 == strcmp(c, " "));
      free(c);
    }
  }

  SUBCASE("PutRowOfTaBs") {
    struct ncplane_options nopts{};
    nopts.rows = 2;
    nopts.cols = TABWIDTH;
    auto n = ncplane_create(n_, &nopts);
    unsigned y, x;
    CHECK(1 == ncplane_putstr(n, "\t\t\t\t\t\t\t\t\t\t"));
    ncplane_cursor_yx(n, &y, &x);
    CHECK(y == 1);
    CHECK(x == 0);
    for(unsigned i = 0 ; i < ncplane_dim_x(n) ; ++i){
      char* c = ncplane_at_yx(n, 0, i, nullptr, nullptr);
      REQUIRE(c);
      CHECK(0 == strcmp(c, " "));
      free(c);
    }
  }

  CHECK(0 == notcurses_stop(nc_));

}
