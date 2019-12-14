#include "main.h"
#include <iostream>

class PanelReelTest : public :: testing::Test {
 protected:
  void SetUp() override {
    setlocale(LC_ALL, "");
    if(getenv("TERM") == nullptr){
      GTEST_SKIP();
    }
    notcurses_options nopts{};
    nopts.inhibit_alternate_screen = true;
    outfp_ = fopen("/dev/tty", "wb");
    ASSERT_NE(nullptr, outfp_);
    nc_ = notcurses_init(&nopts, outfp_);
    ASSERT_NE(nullptr, nc_);
    n_ = notcurses_stdplane(nc_);
    ASSERT_NE(nullptr, n_);
    ASSERT_EQ(0, ncplane_cursor_move_yx(n_, 0, 0));
  }

  void TearDown() override {
    if(nc_){
      EXPECT_EQ(0, notcurses_stop(nc_));
    }
    if(outfp_){
      fclose(outfp_);
    }
  }

  struct notcurses* nc_{};
  struct ncplane* n_{};
  FILE* outfp_{};
};


TEST_F(PanelReelTest, InitLinear) {
  panelreel_options p = { };
  struct panelreel* pr = panelreel_create(n_, &p, -1);
  ASSERT_NE(nullptr, pr);
}

TEST_F(PanelReelTest, InitLinearInfinite) {
  panelreel_options p{};
  p.infinitescroll = true;
  struct panelreel* pr = panelreel_create(n_, &p, -1);
  ASSERT_NE(nullptr, pr);
}

TEST_F(PanelReelTest, InitCircular) {
  panelreel_options p{};
  p.infinitescroll = true;
  p.circular = true;
  struct panelreel* pr = panelreel_create(n_, &p, -1);
  ASSERT_NE(nullptr, pr);
  ASSERT_EQ(0, panelreel_destroy(pr));
}

// circular is not allowed to be true when infinitescroll is false
TEST_F(PanelReelTest, FiniteCircleRejected) {
  panelreel_options p{};
  p.infinitescroll = false;
  p.circular = true;
  struct panelreel* pr = panelreel_create(n_, &p, -1);
  ASSERT_EQ(nullptr, pr);
}

// We ought be able to invoke panelreel_next() and panelreel_prev() safely,
// even if there are no tablets. They both ought return nullptr.
TEST_F(PanelReelTest, MovementWithoutTablets) {
  panelreel_options p{};
  p.infinitescroll = false;
  struct panelreel* pr = panelreel_create(n_, &p, -1);
  ASSERT_NE(nullptr, pr);
  EXPECT_EQ(nullptr, panelreel_next(pr));
  // EXPECT_EQ(0, panelreel_validate(n_, pr));
  EXPECT_EQ(nullptr, panelreel_prev(pr));
  // EXPECT_EQ(0, panelreel_validate(n_, pr));
}

int panelcb(struct ncplane* p, int begx, int begy, int maxx, int maxy,
            bool cliptop, void* curry){
  EXPECT_NE(nullptr, p);
  EXPECT_LT(begx, maxx);
  EXPECT_LT(begy, maxy);
  EXPECT_EQ(nullptr, curry);
  EXPECT_FALSE(cliptop);
  // FIXME verify geometry is as expected
  return 0;
}

TEST_F(PanelReelTest, OneTablet) {
  panelreel_options p{};
  p.infinitescroll = false;
  struct panelreel* pr = panelreel_create(n_, &p, -1);
  ASSERT_NE(nullptr, pr);
  struct tablet* t = panelreel_add(pr, nullptr, nullptr, panelcb, nullptr);
  ASSERT_NE(nullptr, t);
  // EXPECT_EQ(0, panelreel_validate(n_, pr));
  EXPECT_EQ(0, panelreel_del(pr, t));
  // EXPECT_EQ(0, panelreel_validate(n_, pr));
}

TEST_F(PanelReelTest, MovementWithOneTablet) {
  panelreel_options p{};
  p.infinitescroll = false;
  struct panelreel* pr = panelreel_create(n_, &p, -1);
  ASSERT_NE(nullptr, pr);
  struct tablet* t = panelreel_add(pr, nullptr, nullptr, panelcb, nullptr);
  ASSERT_NE(nullptr, t);
  // EXPECT_EQ(0, panelreel_validate(n_, pr));
  EXPECT_NE(nullptr, panelreel_next(pr));
  // EXPECT_EQ(0, panelreel_validate(n_, pr));
  EXPECT_NE(nullptr, panelreel_prev(pr));
  // EXPECT_EQ(0, panelreel_validate(n_, pr));
  EXPECT_EQ(0, panelreel_del(pr, t));
  // EXPECT_EQ(0, panelreel_validate(n_, pr));
}

TEST_F(PanelReelTest, DeleteActiveTablet) {
  panelreel_options p{};
  p.infinitescroll = false;
  struct panelreel* pr = panelreel_create(n_, &p, -1);
  ASSERT_NE(nullptr, pr);
  struct tablet* t = panelreel_add(pr, nullptr, nullptr, panelcb, nullptr);
  ASSERT_NE(nullptr, t);
  EXPECT_EQ(0, panelreel_del_focused(pr));
}

TEST_F(PanelReelTest, NoBorder) {
  panelreel_options p{};
  p.bordermask = NCBOXMASK_LEFT | NCBOXMASK_RIGHT |
                  NCBOXMASK_TOP | NCBOXMASK_BOTTOM;
  struct panelreel* pr = panelreel_create(n_, &p, -1);
  ASSERT_NE(nullptr, pr);
}

TEST_F(PanelReelTest, BadBorderBitsRejected) {
  panelreel_options p{};
  p.bordermask = NCBOXMASK_LEFT * 2;
  struct panelreel* pr = panelreel_create(n_, &p, -1);
  ASSERT_EQ(nullptr, pr);
}

TEST_F(PanelReelTest, NoTabletBorder) {
  panelreel_options p{};
  p.tabletmask = NCBOXMASK_LEFT | NCBOXMASK_RIGHT |
                  NCBOXMASK_TOP | NCBOXMASK_BOTTOM;
  struct panelreel* pr = panelreel_create(n_, &p, -1);
  ASSERT_NE(nullptr, pr);
}

TEST_F(PanelReelTest, NoTopBottomBorder) {
  panelreel_options p{};
  p.bordermask = NCBOXMASK_TOP | NCBOXMASK_BOTTOM;
  struct panelreel* pr = panelreel_create(n_, &p, -1);
  ASSERT_NE(nullptr, pr);
}

TEST_F(PanelReelTest, NoSideBorders) {
  panelreel_options p{};
  p.bordermask = NCBOXMASK_LEFT | NCBOXMASK_RIGHT;
  struct panelreel* pr = panelreel_create(n_, &p, -1);
  ASSERT_NE(nullptr, pr);
}

TEST_F(PanelReelTest, BadTabletBorderBitsRejected) {
  panelreel_options p{};
  p.tabletmask = NCBOXMASK_LEFT * 2;
  struct panelreel* pr = panelreel_create(n_, &p, -1);
  ASSERT_EQ(nullptr, pr);
}

/*
// Make a target window occupying all but a containing perimeter of the
// specified WINDOW (which will usually be n_).
struct ncpanel* make_targwin(struct ncpanel* w) {
  cchar_t cc;
  int cpair = COLOR_GREEN;
  EXPECT_EQ(OK, setcchar(&cc, L"W", 0, 0, &cpair));
  int x, y, xx, yy;
  getbegyx(w, y, x);
  getmaxyx(w, yy, xx);
  yy -= 2;
  xx -= 2;
  ++x;
  ++y;
  WINDOW* ww = subwin(w, yy, xx, y, x);
  EXPECT_NE(nullptr, ww);
  PANEL* p = new_panel(ww);
  EXPECT_NE(nullptr, p);
  EXPECT_EQ(OK, wbkgrnd(ww, &cc));
  return p;
}

TEST_F(PanelReelTest, InitWithinSubwin) {
  panelreel_options p{};
  p.loff = 1;
  p.roff = 1;
  p.toff = 1;
  p.boff = 1;
  EXPECT_EQ(0, clear());
  PANEL* base = make_targwin(n_);
  ASSERT_NE(nullptr, base);
  WINDOW* basew = panel_window(base);
  ASSERT_NE(nullptr, basew);
  struct panelreel* pr = panelreel_create(basew, &p, -1);
  ASSERT_NE(nullptr, pr);
  EXPECT_EQ(0, panelreel_validate(basew, pr));
  ASSERT_EQ(0, panelreel_destroy(pr));
  EXPECT_EQ(OK, del_panel(base));
  EXPECT_EQ(OK, delwin(basew));
}

TEST_F(PanelReelTest, SubwinNoPanelreelBorders) {
  panelreel_options p{};
  p.loff = 1;
  p.roff = 1;
  p.toff = 1;
  p.boff = 1;
  p.bordermask = NCBOXMASK_LEFT | NCBOXMASK_RIGHT |
                  NCBOXMASK_TOP | NCBOXMASK_BOTTOM;
  EXPECT_EQ(0, clear());
  PANEL* base = make_targwin(n_);
  ASSERT_NE(nullptr, base);
  WINDOW* basew = panel_window(base);
  ASSERT_NE(nullptr, basew);
  struct panelreel* pr = panelreel_create(basew, &p, -1);
  ASSERT_NE(nullptr, pr);
  EXPECT_EQ(0, panelreel_validate(basew, pr));
  ASSERT_EQ(0, panelreel_destroy(pr));
  EXPECT_EQ(OK, del_panel(base));
  EXPECT_EQ(OK, delwin(basew));
}

TEST_F(PanelReelTest, SubwinNoOffsetGeom) {
  panelreel_options p{};
  EXPECT_EQ(0, clear());
  PANEL* base = make_targwin(n_);
  ASSERT_NE(nullptr, base);
  WINDOW* basew = panel_window(base);
  ASSERT_NE(nullptr, basew);
  struct panelreel* pr = panelreel_create(basew, &p, -1);
  ASSERT_NE(nullptr, pr);
  EXPECT_EQ(0, panelreel_validate(basew, pr));
  ASSERT_EQ(0, panelreel_destroy(pr));
  EXPECT_EQ(OK, del_panel(base));
  EXPECT_EQ(OK, delwin(basew));
}
*/

TEST_F(PanelReelTest, TransparentBackground) {
  panelreel_options p{};
  channels_set_bg_alpha(&p.bgchannel, 3);
  struct panelreel* pr = panelreel_create(n_, &p, -1);
  ASSERT_NE(nullptr, pr);
  // FIXME
}
