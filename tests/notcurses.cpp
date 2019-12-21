#include <string>
#include <cstdlib>
#include <notcurses.h>
#include "internal.h"
#include "main.h"

class NotcursesTest : public :: testing::Test {
 protected:
  void SetUp() override {
    setlocale(LC_ALL, "");
    if(getenv("TERM") == nullptr){
      GTEST_SKIP();
    }
    notcurses_options nopts{};
    nopts.inhibit_alternate_screen = true;
    nopts.suppress_bannner = true;
    outfp_ = fopen("/dev/tty", "wb");
    ASSERT_NE(nullptr, outfp_);
    nc_ = notcurses_init(&nopts, outfp_);
    ASSERT_NE(nullptr, nc_);
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
  FILE* outfp_{};
};

TEST_F(NotcursesTest, NotcursesVersionString) {
  const char* ver = notcurses_version();
  ASSERT_NE(nullptr, ver);
  ASSERT_LT(0, strlen(ver));
  std::cout << "notcurses version " << ver << std::endl;
}

TEST_F(NotcursesTest, BasicLifetime) {
}

TEST_F(NotcursesTest, TermDimensions) {
  int x, y;
  notcurses_term_dim_yx(nc_, &y, &x);
  auto stry = getenv("LINES");
  if(stry){
    auto envy = std::stoi(stry, nullptr);
    EXPECT_EQ(envy, y);
  }
  auto strx = getenv("COLUMNS");
  if(stry){
    auto envx = std::stoi(strx, nullptr);
    EXPECT_EQ(envx, x);
  }
}

TEST_F(NotcursesTest, ResizeSameSize) {
  int x, y;
  notcurses_term_dim_yx(nc_, &y, &x);
  int newx, newy;
  EXPECT_EQ(0, notcurses_resize(nc_, &newy, &newx));
  EXPECT_EQ(newx, x);
  EXPECT_EQ(newy, y);
}

// we should at least have CELL_STYLE_BOLD everywhere, i should think?
TEST_F(NotcursesTest, CursesStyles) {
  unsigned attrs = notcurses_supported_styles(nc_);
  EXPECT_EQ(1, !!(CELL_STYLE_BOLD & attrs));
}

// it is an error to attempt to destroy the standard plane
TEST_F(NotcursesTest, RejectDestroyStdPlane) {
  ncplane* ncp = notcurses_stdplane(nc_);
  ASSERT_NE(nullptr, ncp);
  ASSERT_NE(0, ncplane_destroy(ncp));
}

// it is an error to attempt to move the standard plane
TEST_F(NotcursesTest, RejectMoveStdPlane) {
  ncplane* ncp = notcurses_stdplane(nc_);
  ASSERT_NE(nullptr, ncp);
  ASSERT_NE(0, ncplane_move_yx(ncp, 1, 1));
}

// create planes partitioning the entirety of the screen, one at each coordinate
TEST_F(NotcursesTest, TileScreenWithPlanes) {
  int maxx, maxy;
  notcurses_term_dim_yx(nc_, &maxy, &maxx);
  auto total = maxx * maxy;
  struct ncplane** planes = new struct ncplane*[total];
  int* planesecrets = new int[total];
  for(int y = 0 ; y < maxy ; ++y){
    for(int x = 0 ; x < maxx ; ++x){
      const auto idx = y * maxx + x;
      planes[idx] = notcurses_newplane(nc_, 1, 1, y, x, &planesecrets[idx]);
      ASSERT_NE(nullptr, planes[idx]);
    }
  }
  ASSERT_EQ(0, notcurses_render(nc_));
  for(int y = 0 ; y < maxy ; ++y){
    for(int x = 0 ; x < maxx ; ++x){
      const auto idx = y * maxx + x;
      auto userptr = ncplane_userptr(planes[idx]);
      ASSERT_NE(nullptr, userptr);
      EXPECT_EQ(userptr, &planesecrets[idx]);
      ASSERT_EQ(0, ncplane_destroy(planes[idx]));
    }
  }
  delete[] planesecrets;
  delete[] planes;
  ASSERT_EQ(0, notcurses_render(nc_));
}

TEST_F(NotcursesTest, ChannelSetFGAlpha){
  uint64_t channel = 0;
  EXPECT_GT(0, channels_set_fg_alpha(&channel, -1));
  EXPECT_GT(0, channels_set_fg_alpha(&channel, 4));
  EXPECT_EQ(0, channels_set_fg_alpha(&channel, CELL_ALPHA_OPAQUE));
  EXPECT_EQ(CELL_ALPHA_OPAQUE, channels_get_fg_alpha(channel));
  EXPECT_EQ(0, channels_set_fg_alpha(&channel, CELL_ALPHA_HIGHCONTRAST));
  EXPECT_EQ(CELL_ALPHA_HIGHCONTRAST, channels_get_fg_alpha(channel));
  EXPECT_TRUE(channels_fg_default_p(channel));
  EXPECT_TRUE(channels_bg_default_p(channel));
}

TEST_F(NotcursesTest, ChannelSetBGAlpha){
  uint64_t channel = 0;
  EXPECT_GT(0, channels_set_bg_alpha(&channel, -1));
  EXPECT_GT(0, channels_set_bg_alpha(&channel, 4));
  EXPECT_EQ(0, channels_set_bg_alpha(&channel, CELL_ALPHA_OPAQUE));
  EXPECT_EQ(0, channels_get_bg_alpha(channel));
  EXPECT_EQ(0, channels_set_bg_alpha(&channel, CELL_ALPHA_TRANSPARENT));
  EXPECT_NE(0, channels_set_bg_alpha(&channel, CELL_ALPHA_HIGHCONTRAST));
  EXPECT_EQ(CELL_ALPHA_TRANSPARENT, channels_get_bg_alpha(channel));
  EXPECT_TRUE(channels_fg_default_p(channel));
  EXPECT_TRUE(channels_bg_default_p(channel));
}

TEST_F(NotcursesTest, Stats){
  struct ncstats stats;
  notcurses_stats(nc_, &stats);
  EXPECT_EQ(0, stats.renders);
  EXPECT_EQ(0, notcurses_render(nc_));
  notcurses_stats(nc_, &stats);
  EXPECT_EQ(1, stats.renders);
  notcurses_reset_stats(nc_);
  notcurses_stats(nc_, &stats);
  EXPECT_EQ(0, stats.renders);
}
