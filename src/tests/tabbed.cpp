#include "main.h"

void tabbedcb(struct nctab*, struct ncplane*, void*){

}

TEST_CASE("Tabbed") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);
  REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));

  SUBCASE("CreateNullOpts") {
    struct ncplane_options nopts = {
      .y = 1, .x = 2, .rows = ncplane_dim_y(n_) - 2, .cols = ncplane_dim_x(n_) - 4,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto ncp = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != ncp);
    auto nt = nctabbed_create(ncp, nullptr);
    REQUIRE(nullptr != nt);
    CHECK(0 == nctabbed_hdrchan(nt));
    CHECK(0 == nctabbed_selchan(nt));
    CHECK(0 == nctabbed_sepchan(nt));
    CHECK(nullptr == nctabbed_separator(nt));
    nctabbed_destroy(nt);
  }

  SUBCASE("Create") {
    struct nctabbed_options opts = {
      .selchan = NCCHANNELS_INITIALIZER(0, 255, 0, 70, 70, 70),
      .hdrchan = NCCHANNELS_INITIALIZER(255, 0, 0, 60, 60, 60),
      .sepchan = NCCHANNELS_INITIALIZER(0, 0, 255, 60, 60, 60),
      .separator = const_cast<char*>("-separator-"),
      .flags = NCTABBED_OPTION_BOTTOM
    };
    struct ncplane_options nopts = {
      .y = 1, .x = 2, .rows = ncplane_dim_y(n_) - 2, .cols = ncplane_dim_x(n_) - 4,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto ncp = ncplane_create(n_, &nopts);
    auto nt = nctabbed_create(ncp, &opts);
    REQUIRE(nullptr != nt);
    CHECK(opts.hdrchan == nctabbed_hdrchan(nt));
    CHECK(opts.selchan == nctabbed_selchan(nt));
    CHECK(opts.sepchan == nctabbed_sepchan(nt));
    CHECK(0 == strcmp("-separator-", nctabbed_separator(nt)));
    nctabbed_destroy(nt);
  }

  SUBCASE("Add") {
    struct ncplane_options nopts = {
      .y = 1, .x = 2, .rows = ncplane_dim_y(n_) - 2, .cols = ncplane_dim_x(n_) - 4,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto ncp = ncplane_create(n_, &nopts);
    auto nt = nctabbed_create(ncp, nullptr);
    REQUIRE(nullptr != nt);
    CHECK(0 == nctabbed_tabcount(nt));
    auto t1 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "amazingtab123", nullptr);
    REQUIRE(nullptr != t1);
    CHECK(t1 == nctabbed_selected(nt));
    CHECK(t1 == nctabbed_leftmost(nt));
    CHECK(1 == nctabbed_tabcount(nt));
    CHECK(t1 == nctab_next(t1));
    CHECK(t1 == nctab_prev(t1));
    CHECK(nullptr == nctab_userptr(t1));
    CHECK(0 == strcmp("amazingtab123", nctab_name(t1)));
    auto t2 = nctabbed_add(nt, nullptr, nullptr, nullptr, "nullcbtab", nullptr);
    REQUIRE(nullptr != t2);
    CHECK(t1 == nctabbed_selected(nt));
    CHECK(t1 == nctabbed_leftmost(nt));
    CHECK(2 == nctabbed_tabcount(nt));
    CHECK(t2 == nctab_next(t1));
    CHECK(t2 == nctab_prev(t1));
    CHECK(t1 == nctab_next(t2));
    CHECK(t1 == nctab_prev(t2));
    // this one gets put in the middle
    auto t3 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab3", nullptr);
    REQUIRE(nullptr != t3);
    CHECK(3 == nctabbed_tabcount(nt));
    CHECK(t2 == nctab_next(t3));
    CHECK(t1 == nctab_prev(t3));
    CHECK(t1 == nctab_next(t2));
    CHECK(t2 == nctab_prev(t1));
    // now the last
    auto t4 = nctabbed_add(nt, t2, t1, tabbedcb, "tab4", nullptr);
    REQUIRE(nullptr != t4);
    CHECK(4 == nctabbed_tabcount(nt));
    CHECK(t1 == nctab_next(t4));
    CHECK(t2 == nctab_prev(t4));
    // second to last
    auto t5 = nctabbed_add(nt, nullptr, t4, tabbedcb, "tab5", nullptr);
    REQUIRE(nullptr != t5);
    CHECK(t4 == nctab_next(t5));
    CHECK(t2 == nctab_prev(t5));
    // second
    auto t6 = nctabbed_add(nt, t1, nullptr, tabbedcb, "tab6", nullptr);
    REQUIRE(nullptr != t6);
    CHECK(t3 == nctab_next(t6));
    CHECK(t1 == nctab_prev(t6));
    nctabbed_destroy(nt);
  }

  SUBCASE("Del") {
    struct ncplane_options nopts = {
      .y = 1, .x = 2, .rows = ncplane_dim_y(n_) - 2, .cols = ncplane_dim_x(n_) - 4,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto ncp = ncplane_create(n_, &nopts);
    auto nt = nctabbed_create(ncp, nullptr);
    REQUIRE(nullptr != nt);
    auto t1 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab1", nullptr);
    auto t5 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab5", nullptr);
    auto t4 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab4", nullptr);
    auto t3 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab3", nullptr);
    auto t2 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab2", nullptr);
    CHECK(5 == nctabbed_tabcount(nt));
    // tabs: t1 t2 t3 t4 t5
    CHECK(0 == nctabbed_del(nt, t5));
    CHECK(4 == nctabbed_tabcount(nt));
    CHECK(t1 == nctab_next(t4));
    CHECK(t4 == nctab_prev(t1));
    // tabs: t1 t2 t3 t4
    CHECK(0 == nctabbed_del(nt, t1));
    CHECK(3 == nctabbed_tabcount(nt));
    CHECK(t2 == nctab_next(t4));
    CHECK(t4 == nctab_prev(t2));
    CHECK(t2 == nctabbed_selected(nt));
    CHECK(t2 == nctabbed_leftmost(nt));
    // tabs: t2 t3 t4
    CHECK(0 == nctabbed_del(nt, t3));
    CHECK(2 == nctabbed_tabcount(nt));
    CHECK(t2 == nctab_next(t4));
    CHECK(t4 == nctab_prev(t2));
    // tabs: t2 t4
    CHECK(0 == nctabbed_del(nt, t2));
    CHECK(1 == nctabbed_tabcount(nt));
    CHECK(t4 == nctab_next(t4));
    CHECK(t4 == nctab_prev(t4));
    CHECK(t4 == nctabbed_selected(nt));
    CHECK(t4 == nctabbed_leftmost(nt));
    // only t4 left
    CHECK(0 == nctabbed_del(nt, t4));
    CHECK(0 == nctabbed_tabcount(nt));
    CHECK(nullptr == nctabbed_selected(nt));
    CHECK(nullptr == nctabbed_leftmost(nt));
    nctabbed_destroy(nt);
  }

  SUBCASE("Move") {
    struct ncplane_options nopts = {
      .y = 1, .x = 2, .rows = ncplane_dim_y(n_) - 2, .cols = ncplane_dim_x(n_) - 4,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto ncp = ncplane_create(n_, &nopts);
    auto nt = nctabbed_create(ncp, nullptr);
    REQUIRE(nullptr != nt);
    auto t1 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab1", nullptr);
    auto t5 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab5", nullptr);
    auto t4 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab4", nullptr);
    auto t3 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab3", nullptr);
    auto t2 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab2", nullptr);
    // tabs: t1 t2 t3 t4 t5
    nctab_move(nt, t4, t5, nullptr);
    CHECK(t1 == nctab_next(t4));
    CHECK(t5 == nctab_prev(t4));
    CHECK(t2 == nctab_next(t1));
    CHECK(t4 == nctab_prev(t1));
    CHECK(t4 == nctab_next(t5));
    CHECK(t3 == nctab_prev(t5));
    // tabs: t1 t2 t3 t5 t4
    nctab_move(nt, t1, t2, nullptr);
    CHECK(t3 == nctab_next(t1));
    CHECK(t2 == nctab_prev(t1));
    CHECK(t1 == nctab_next(t2));
    CHECK(t4 == nctab_prev(t2));
    CHECK(t5 == nctab_next(t3));
    CHECK(t1 == nctab_prev(t3));
    CHECK(t1 == nctabbed_selected(nt));
    CHECK(t1 == nctabbed_leftmost(nt));
    // tabs: t1 t3 t5 t4 t2
    nctab_move(nt, t2, nullptr, t3);
    CHECK(t2 == nctab_next(t1));
    CHECK(t4 == nctab_prev(t1));
    CHECK(t3 == nctab_next(t2));
    CHECK(t1 == nctab_prev(t2));
    CHECK(t5 == nctab_next(t3));
    CHECK(t2 == nctab_prev(t3));
    // tabs: t1 t2 t3 t5 t4
    nctab_move(nt, t1, nullptr, t4);
    CHECK(t1 == nctab_next(t5));
    CHECK(t3 == nctab_prev(t5));
    CHECK(t4 == nctab_next(t1));
    CHECK(t5 == nctab_prev(t1));
    CHECK(t2 == nctab_next(t4));
    CHECK(t1 == nctab_prev(t4));
    CHECK(t1 == nctabbed_selected(nt));
    CHECK(t1 == nctabbed_leftmost(nt));
    // tabs: t1 t4 t2 t3 t5
    nctabbed_destroy(nt);
  }

  SUBCASE("Rotate") {
    struct ncplane_options nopts = {
      .y = 1, .x = 2, .rows = ncplane_dim_y(n_) - 2, .cols = ncplane_dim_x(n_) - 4,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto ncp = ncplane_create(n_, &nopts);
    auto nt = nctabbed_create(ncp, nullptr);
    REQUIRE(nullptr != nt);
    auto t1 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab1", nullptr);
    auto t5 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab5", nullptr);
    auto t4 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab4", nullptr);
    auto t3 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab3", nullptr);
    auto t2 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab2", nullptr);
    // tabs: t1 t2 t3 t4 t5
    nctabbed_rotate(nt, 1);
    CHECK(t5 == nctabbed_leftmost(nt));
    CHECK(t1 == nctabbed_selected(nt));
    // tabs: t5 t1 t2 t3 t4
    nctabbed_rotate(nt, 7);
    CHECK(t3 == nctabbed_leftmost(nt));
    CHECK(t1 == nctabbed_selected(nt));
    // tabs: t3 t4 t5 t1 t2
    nctabbed_rotate(nt, -1);
    CHECK(t4 == nctabbed_leftmost(nt));
    CHECK(t1 == nctabbed_selected(nt));
    // tabs: t4 t5 t1 t2 t3
    nctabbed_rotate(nt, -13);
    CHECK(t2 == nctabbed_leftmost(nt));
    CHECK(t1 == nctabbed_selected(nt));
    // tabs: t2 t3 t4 t5 t1
    nctabbed_destroy(nt);
  }

  SUBCASE("MoveLeftRight") {
    struct ncplane_options nopts = {
      .y = 1, .x = 2, .rows = ncplane_dim_y(n_) - 2, .cols = ncplane_dim_x(n_) - 4,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto ncp = ncplane_create(n_, &nopts);
    auto nt = nctabbed_create(ncp, nullptr);
    REQUIRE(nullptr != nt);
    auto t1 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab1", nullptr);
    auto t5 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab5", nullptr);
    auto t4 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab4", nullptr);
    auto t3 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab3", nullptr);
    auto t2 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab2", nullptr);
    // tabs: t1 t2 t3 t4 t5
    nctab_move_right(nt, t1);
    CHECK(t3 == nctab_next(t1));
    CHECK(t2 == nctab_prev(t1));
    CHECK(t2 == nctabbed_leftmost(nt));
    CHECK(t1 == nctabbed_selected(nt));
    // tabs: t2 t1 t3 t4 t5
    nctab_move_right(nt, t1);
    CHECK(t4 == nctab_next(t1));
    CHECK(t3 == nctab_prev(t1));
    CHECK(t2 == nctabbed_leftmost(nt));
    CHECK(t1 == nctabbed_selected(nt));
    // tabs: t2 t3 t1 t4 t5
    nctab_move_right(nt, t5);
    CHECK(t2 == nctab_next(t5));
    CHECK(t4 == nctab_prev(t5));
    CHECK(t5 == nctabbed_leftmost(nt));
    CHECK(t1 == nctabbed_selected(nt));
    // tabs: t5 t2 t3 t1 t4
    nctab_move_right(nt, t5);
    CHECK(t3 == nctab_next(t5));
    CHECK(t2 == nctab_prev(t5));
    CHECK(t2 == nctabbed_leftmost(nt));
    CHECK(t1 == nctabbed_selected(nt));
    // tabs: t2 t5 t3 t1 t4
    nctab_move_left(nt, t5);
    CHECK(t2 == nctab_next(t5));
    CHECK(t4 == nctab_prev(t5));
    CHECK(t5 == nctabbed_leftmost(nt));
    CHECK(t1 == nctabbed_selected(nt));
    // tabs: t5 t2 t3 t1 t4
    nctab_move_left(nt, t5);
    CHECK(t2 == nctab_next(t5));
    CHECK(t4 == nctab_prev(t5));
    CHECK(t2 == nctabbed_leftmost(nt));
    CHECK(t1 == nctabbed_selected(nt));
    // tabs: t2 t3 t1 t4 t5
    nctab_move_left(nt, t5);
    CHECK(t4 == nctab_next(t5));
    CHECK(t1 == nctab_prev(t5));
    CHECK(t2 == nctabbed_leftmost(nt));
    CHECK(t1 == nctabbed_selected(nt));
    // tabs: t2 t3 t1 t5 t4
    nctab_move_left(nt, t5);
    CHECK(t1 == nctab_next(t5));
    CHECK(t3 == nctab_prev(t5));
    CHECK(t2 == nctabbed_leftmost(nt));
    CHECK(t1 == nctabbed_selected(nt));
    // tabs: t2 t3 t5 t1 t4
    nctabbed_destroy(nt);
  }

  SUBCASE("Select") {
    struct ncplane_options nopts = {
      .y = 1, .x = 2, .rows = ncplane_dim_y(n_) - 2, .cols = ncplane_dim_x(n_) - 4,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto ncp = ncplane_create(n_, &nopts);
    auto nt = nctabbed_create(ncp, nullptr);
    REQUIRE(nullptr != nt);
    auto t1 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab1", nullptr);
    auto t5 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab5", nullptr);
    auto t4 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab4", nullptr);
    auto t3 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab3", nullptr);
    auto t2 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab2", nullptr);
    (void)t3;
    (void)t4;
    CHECK(t1 == nctabbed_selected(nt));
    nctabbed_select(nt, t2);
    CHECK(t2 == nctabbed_selected(nt));
    nctabbed_select(nt, t5);
    CHECK(t5 == nctabbed_selected(nt));
    nctabbed_select(nt, t5);
    CHECK(t5 == nctabbed_selected(nt));
    nctabbed_select(nt, t1);
    CHECK(t1 == nctabbed_selected(nt));
    nctabbed_destroy(nt);
  }

  SUBCASE("NextPrev") {
    struct ncplane_options nopts = {
      .y = 1, .x = 2, .rows = ncplane_dim_y(n_) - 2, .cols = ncplane_dim_x(n_) - 4,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto ncp = ncplane_create(n_, &nopts);
    auto nt = nctabbed_create(ncp, nullptr);
    REQUIRE(nullptr != nt);
    auto t1 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab1", nullptr);
    auto t5 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab5", nullptr);
    auto t4 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab4", nullptr);
    auto t3 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab3", nullptr);
    auto t2 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab2", nullptr);
    CHECK(t1 == nctabbed_selected(nt));
    CHECK(t2 == nctabbed_next(nt));
    CHECK(t2 == nctabbed_selected(nt));
    CHECK(t1 == nctabbed_leftmost(nt));
    CHECK(t3 == nctabbed_next(nt));
    CHECK(t3 == nctabbed_selected(nt));
    CHECK(t1 == nctabbed_leftmost(nt));
    CHECK(t4 == nctabbed_next(nt));
    CHECK(t4 == nctabbed_selected(nt));
    CHECK(t1 == nctabbed_leftmost(nt));
    CHECK(t5 == nctabbed_next(nt));
    CHECK(t5 == nctabbed_selected(nt));
    CHECK(t1 == nctabbed_leftmost(nt));
    CHECK(t1 == nctabbed_next(nt));
    CHECK(t1 == nctabbed_selected(nt));
    CHECK(t1 == nctabbed_leftmost(nt));
    nctabbed_destroy(nt);
  }

  SUBCASE("Setters") {
    struct ncplane_options nopts = {
      .y = 1, .x = 2, .rows = ncplane_dim_y(n_) - 2, .cols = ncplane_dim_x(n_) - 4,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto ncp = ncplane_create(n_, &nopts);
    auto nt = nctabbed_create(ncp, nullptr);
    REQUIRE(nullptr != nt);
    uint64_t hdrchan = NCCHANNELS_INITIALIZER(255, 127, 63, 31, 15, 7);
    uint64_t selchan = NCCHANNELS_INITIALIZER(127, 63, 31, 15, 7, 3);
    uint64_t sepchan = NCCHANNELS_INITIALIZER(63, 31, 15, 7, 3, 1);
    nctabbed_set_hdrchan(nt, hdrchan);
    nctabbed_set_selchan(nt, selchan);
    nctabbed_set_sepchan(nt, sepchan);
    uint64_t hdrchan2, selchan2, sepchan2;
    nctabbed_channels(nt, &hdrchan2, &selchan2, &sepchan2);
    CHECK(hdrchan == hdrchan2);
    CHECK(selchan == selchan2);
    CHECK(sepchan == sepchan2);
    const char* sep = "separateur";
    nctabbed_set_separator(nt, sep);
    CHECK(0 == strcmp(sep, nctabbed_separator(nt)));
    auto t1 = nctabbed_add(nt, nullptr, nullptr, tabbedcb, "tab1", nullptr);
    nctab_set_cb(t1, tabbedcb);
    // FIXME workaround for busted doctest 2.4.9 =[
    // CHECK(tabbedcb == nctab_cb(t1));
    nctab_set_userptr(t1, (void*) sep);
    CHECK((void*) sep == nctab_userptr(t1));
    const char* tname = "tab name";
    nctab_set_name(t1, tname);
    CHECK(0 == strcmp(tname, nctab_name(t1)));
    nctabbed_destroy(nt);
  }

  CHECK(0 == notcurses_stop(nc_));
}
