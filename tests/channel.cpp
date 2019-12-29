#include <notcurses.h>
#include "main.h"

TEST_CASE("ChannelGetRGB") {
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
    CHECK(test[i].channel == channel_get_rgb(test[i].channel, &r, &g, &b));
    CHECK(test[i].r == r);
    CHECK(test[i].g == g);
    CHECK(test[i].b == b);
  }
}

TEST_CASE("ChannelGetAlpha") {
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
    CHECK(test[i].a == channel_get_alpha(test[i].channel));
  }
}

TEST_CASE("ChannelGetDefault") {
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
    CHECK(test[i].def == channel_default_p(test[i].channel));
  }
}

TEST_CASE("ChannelSetDefault") {
  const uint32_t channels[] = {
    0x40000000, 0x4fffffff, 0xcfffffff,
    0x40808080, 0x40080808, 0xffffffff,
  };
  for(auto i = 0u ; i < sizeof(channels) / sizeof(*channels) ; ++i){
    uint32_t channel = channels[i];
    CHECK(!channel_default_p(channel));
    channel_set_default(&channel);
    CHECK(channel_default_p(channel));
  }
}

// blend of 0 ought set c1 to c2
TEST_CASE("ChannelBlend0") {
  uint32_t c1 = 0;
  uint32_t c2 = 0;
  channel_set_rgb(&c1, 0x80, 0x40, 0x20);
  channel_set_rgb(&c2, 0x88, 0x44, 0x22);
  uint32_t c = channels_blend(c1, c2, 0);
  CHECK(!channel_default_p(c));
  unsigned r, g, b;
  channel_get_rgb(c, &r, &g, &b);
  CHECK(0x88 == r);
  CHECK(0x44 == g);
  CHECK(0x22 == b);
}

// blend of 1 ought perfectly average c1 and c2
TEST_CASE("ChannelBlend1") {
  uint32_t c1 = 0;
  uint32_t c2 = 0;
  channel_set_rgb(&c1, 0x80, 0x40, 0x20);
  channel_set_rgb(&c2, 0x0, 0x0, 0x0);
  uint32_t c = channels_blend(c1, c2, 1);
  CHECK(!channel_default_p(c));
  unsigned r, g, b;
  channel_get_rgb(c, &r, &g, &b);
  CHECK(0x40 == r);
  CHECK(0x20 == g);
  CHECK(0x10 == b);
}

// blend of 2 ought weigh c1 twice as much as c2
TEST_CASE("ChannelBlend2") {
  uint32_t c1 = 0;
  uint32_t c2 = 0;
  channel_set_rgb(&c1, 0x60, 0x30, 0x0f);
  channel_set_rgb(&c2, 0x0, 0x0, 0x0);
  uint32_t c = channels_blend(c1, c2, 2);
  CHECK(!channel_default_p(c));
  unsigned r, g, b;
  channel_get_rgb(c, &r, &g, &b);
  CHECK(0x40 == r);
  CHECK(0x20 == g);
  CHECK(0x0a == b);
}

// you can't blend into a default color, at any number of blends
TEST_CASE("ChannelBlendDefaultLeft") {
  uint32_t c1 = 0;
  uint32_t c2 = 0;
  channel_set_rgb(&c2, 0x80, 0x40, 0x20);
  uint32_t c = channels_blend(c1, c2, 0);
  CHECK(channel_default_p(c));
  unsigned r, g, b;
  channel_get_rgb(c, &r, &g, &b);
  CHECK(0 == r);
  CHECK(0 == g);
  CHECK(0 == b);
  c = channels_blend(c1, c2, 1);
  CHECK(channel_default_p(c));
  channel_get_rgb(c, &r, &g, &b);
  CHECK(0 == r);
  CHECK(0 == g);
  CHECK(0 == b);
}

// you can't blend from a default color, but blend 0 sets it
TEST_CASE("ChannelBlendDefaultRight") {
  uint32_t c1 = 0;
  uint32_t c2 = 0;
  channel_set_rgb(&c1, 0x80, 0x40, 0x20);
  CHECK(channel_default_p(c2));
  uint32_t c = channels_blend(c1, c2, 0);
  CHECK(channel_default_p(c));
  unsigned r, g, b;
  channel_get_rgb(c, &r, &g, &b);
  CHECK(0x80 == r);
  CHECK(0x40 == g);
  CHECK(0x20 == b);
  c = channels_blend(c1, c2, 1);
  CHECK(!channel_default_p(c));
  channel_get_rgb(c, &r, &g, &b);
  CHECK(0x80 == r);
  CHECK(0x40 == g);
  CHECK(0x20 == b);
}
