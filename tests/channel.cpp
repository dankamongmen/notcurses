#include <notcurses.h>
#include "main.h"

class ChannelTest : public :: testing::Test {
 protected:
  void SetUp() override {
    setlocale(LC_ALL, "");
  }
};

TEST_F(ChannelTest, ChannelGetRGB){
  const struct t {
    uint32_t channel;
    int r, g, b;
  } test[] = {
    { .channel = 0x000000, .r = 0x00, .g = 0x00, .b = 0x00, },
    { .channel = 0x808080, .r = 0x80, .g = 0x80, .b = 0x80, },
    { .channel = 0x080808, .r = 0x08, .g = 0x08, .b = 0x08, },
    { .channel = 0xffffff, .r = 0xff, .g = 0xff, .b = 0xff, },
  };
  for(auto i = 0u ; i < sizeof(test) / sizeof(*test) ; ++i){
    unsigned r, g, b;
    EXPECT_EQ(test[i].channel, channel_get_rgb(test[i].channel, &r, &g, &b));
    EXPECT_EQ(test[i].r, r);
    EXPECT_EQ(test[i].g, g);
    EXPECT_EQ(test[i].b, b);
  }
}

TEST_F(ChannelTest, ChannelGetAlpha){
  const struct t {
    uint32_t channel;
    int a;
  } test[] = {
    { .channel = 0x00000000, .a = 0, },
    { .channel = 0x10808080, .a = 1, },
    { .channel = 0x20080808, .a = 2, },
    { .channel = 0xe0080808, .a = 2, },
    { .channel = 0x3fffffff, .a = 3, },
    { .channel = 0xffffffff, .a = 3, },
  };
  for(auto i = 0u ; i < sizeof(test) / sizeof(*test) ; ++i){
    EXPECT_EQ(test[i].a, channel_get_alpha(test[i].channel));
  }
}

TEST_F(ChannelTest, ChannelGetDefault){
  const struct t {
    uint32_t channel;
    bool def;
  } test[] = {
    { .channel = 0x00000000, .def = true, },
    { .channel = 0x0fffffff, .def = true, },
    { .channel = 0xbfffffff, .def = true, },
    { .channel = 0x40000000, .def = false, },
    { .channel = 0x40080808, .def = false, },
    { .channel = 0xffffffff, .def = false, },
  };
  for(auto i = 0u ; i < sizeof(test) / sizeof(*test) ; ++i){
    EXPECT_EQ(test[i].def, channel_default_p(test[i].channel));
  }
}

TEST_F(ChannelTest, ChannelSetDefault){
  const uint32_t channels[] = {
    0x40000000, 0x4fffffff, 0xcfffffff,
    0x40808080, 0x40080808, 0xffffffff,
  };
  for(auto i = 0u ; i < sizeof(channels) / sizeof(*channels) ; ++i){
    uint32_t channel = channels[i];
    EXPECT_FALSE(channel_default_p(channel));
    channel_set_default(&channel);
    EXPECT_TRUE(channel_default_p(channel));
  }
}
