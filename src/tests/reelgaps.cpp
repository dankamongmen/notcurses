#include "main.h"

// ahci-1
int t1_tablet_cb(struct nctablet* t, bool drawfromtop){
  REQUIRE(nullptr != t);
  (void)drawfromtop;
  return 6;
}

// mpt3sas-0
int t2_tablet_cb(struct nctablet* t, bool drawfromtop){
  REQUIRE(nullptr != t);
  (void)drawfromtop;
  return 24;
}

// ahci-0
int t3_tablet_cb(struct nctablet* t, bool drawfromtop){
  REQUIRE(nullptr != t);
  (void)drawfromtop;
  return 6;
}

// virtual
int t4_tablet_cb(struct nctablet* t, bool drawfromtop){
  REQUIRE(nullptr != t);
  (void)drawfromtop;
  return 17;
}

// nvme-0
int t5_tablet_cb(struct nctablet* t, bool drawfromtop){
  REQUIRE(nullptr != t);
  (void)drawfromtop;
  return 3;
}

// nvme-1
int t6_tablet_cb(struct nctablet* t, bool drawfromtop){
  REQUIRE(nullptr != t);
  (void)drawfromtop;
  return 3;
}

// nvme-2
int t7_tablet_cb(struct nctablet* t, bool drawfromtop){
  REQUIRE(nullptr != t);
  (void)drawfromtop;
  return 3;
}

// xhci_pci-0
int t8_tablet_cb(struct nctablet* t, bool drawfromtop){
  REQUIRE(nullptr != t);
  (void)drawfromtop;
  return 5;
}

TEST_CASE("ReelGaps") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

  // https://github.com/dankamongmen/notcurses/issues/901
  SUBCASE("ReelsGapping") {
    ncreel_options r{};
    r.bordermask = 0xf;
    r.tabletchan = NCCHANNELS_INITIALIZER(0, 0xb0, 0xb0, 0, 0, 0);
    r.focusedchan = NCCHANNELS_INITIALIZER(0xff, 0xff, 0xff, 0, 0, 0);
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    auto t1 = ncreel_add(nr, NULL, NULL, t1_tablet_cb, NULL);
    CHECK(nullptr != t1);
    CHECK(ncreel_validate(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    auto t2 = ncreel_add(nr, NULL, NULL, t2_tablet_cb, NULL);
    CHECK(nullptr != t2);
    CHECK(ncreel_validate(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    auto t3 = ncreel_add(nr, NULL, NULL, t3_tablet_cb, NULL);
    CHECK(nullptr != t3);
    CHECK(ncreel_validate(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    auto t4 = ncreel_add(nr, NULL, NULL, t4_tablet_cb, NULL);
    CHECK(nullptr != t4);
    CHECK(ncreel_validate(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    auto t5 = ncreel_add(nr, NULL, NULL, t5_tablet_cb, NULL);
    CHECK(nullptr != t5);
    CHECK(ncreel_validate(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    auto t6 = ncreel_add(nr, NULL, NULL, t6_tablet_cb, NULL);
    CHECK(nullptr != t6);
    CHECK(ncreel_validate(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    ncreel_next(nr); // move to t2
    CHECK(ncreel_validate(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    ncreel_next(nr); // move to t3
    CHECK(ncreel_validate(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    ncreel_next(nr); // move to t4
    CHECK(ncreel_validate(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    ncreel_next(nr); // move to t5
    CHECK(ncreel_validate(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    ncreel_next(nr); // move to t6
    CHECK(ncreel_validate(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    ncreel_next(nr); // move to t7
    CHECK(ncreel_validate(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    ncreel_next(nr); // move to t8
    CHECK(ncreel_validate(nr));
    CHECK_EQ(0, notcurses_render(nc_));
  }

  CHECK(0 == notcurses_stop(nc_));
}
