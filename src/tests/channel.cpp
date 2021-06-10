#include "main.h"

// These tests primarily check against expected constant values, which is
// useful in terms of maintaining ABI, but annoying when they do change.

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
    CHECK(test[i].channel == ncchannel_rgb8(test[i].channel, &r, &g, &b));
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
    { .channel = 0x00000000, .a = NCALPHA_OPAQUE, },
    { .channel = 0x10808080, .a = NCALPHA_BLEND, },
    { .channel = 0x20080808, .a = NCALPHA_TRANSPARENT, },
    { .channel = 0xe0080808, .a = NCALPHA_TRANSPARENT, },
    { .channel = 0x3fffffff, .a = NCALPHA_HIGHCONTRAST, },
    { .channel = 0xffffffff, .a = NCALPHA_HIGHCONTRAST, },
  };
  for(auto i = 0u ; i < sizeof(test) / sizeof(*test) ; ++i){
    CHECK(test[i].a == ncchannel_alpha(test[i].channel));
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
    CHECK(test[i].def == ncchannel_default_p(test[i].channel));
  }
}

TEST_CASE("ChannelSetDefault") {
  const uint32_t channels[] = {
    0x40000000, 0x4fffffff, 0xcfffffff,
    0x40808080, 0x40080808, 0xffffffff,
  };
  for(auto i = 0u ; i < sizeof(channels) / sizeof(*channels) ; ++i){
    uint32_t channel = channels[i];
    CHECK(!ncchannel_default_p(channel));
    ncchannel_set_default(&channel);
    CHECK(ncchannel_default_p(channel));
  }
}

// blend of 0 ought set c1 to c2
TEST_CASE("ChannelBlend0") {
  uint32_t c1 = 0;
  uint32_t c2 = 0;
  ncchannel_set_rgb8(&c1, 0x80, 0x40, 0x20);
  ncchannel_set_rgb8(&c2, 0x88, 0x44, 0x22);
  unsigned blends = 0;
  uint32_t c = channels_blend(c1, c2, &blends);
  CHECK(!ncchannel_default_p(c));
  unsigned r, g, b;
  ncchannel_rgb8(c, &r, &g, &b);
  CHECK(0x88 == r);
  CHECK(0x44 == g);
  CHECK(0x22 == b);
  CHECK(1 == blends);
}

// blend of 1 ought perfectly average c1 and c2
TEST_CASE("ChannelBlend1") {
  uint32_t c1 = 0;
  uint32_t c2 = 0;
  ncchannel_set_rgb8(&c1, 0x80, 0x40, 0x20);
  ncchannel_set_rgb8(&c2, 0x0, 0x0, 0x0);
  unsigned blends = 1;
  uint32_t c = channels_blend(c1, c2, &blends);
  CHECK(!ncchannel_default_p(c));
  unsigned r, g, b;
  ncchannel_rgb8(c, &r, &g, &b);
  CHECK(0x40 == r);
  CHECK(0x20 == g);
  CHECK(0x10 == b);
  CHECK(2 == blends);
}

// blend of 2 ought weigh c1 twice as much as c2
TEST_CASE("ChannelBlend2") {
  uint32_t c1 = 0;
  uint32_t c2 = 0;
  ncchannel_set_rgb8(&c1, 0x60, 0x30, 0x0f);
  ncchannel_set_rgb8(&c2, 0x0, 0x0, 0x0);
  unsigned blends = 2;
  uint32_t c = channels_blend(c1, c2, &blends);
  CHECK(!ncchannel_default_p(c));
  unsigned r, g, b;
  ncchannel_rgb8(c, &r, &g, &b);
  CHECK(0x40 == r);
  CHECK(0x20 == g);
  CHECK(0x0a == b);
  CHECK(3 == blends);
}

// you can't blend into a default color at any positive number of blends
TEST_CASE("ChannelBlendDefaultLeft") {
  uint32_t c1 = 0;
  uint32_t c2 = 0;
  ncchannel_set_rgb8(&c2, 0x80, 0x40, 0x20);
  unsigned blends = 0;
  uint32_t c = channels_blend(c1, c2, &blends); // will replace
  CHECK(!ncchannel_default_p(c));
  unsigned r, g, b;
  ncchannel_rgb8(c, &r, &g, &b);
  CHECK(0x80 == r);
  CHECK(0x40 == g);
  CHECK(0x20 == b);
  CHECK(1 == blends);
  c = channels_blend(c1, c2, &blends); // will not replace
  CHECK(ncchannel_default_p(c));
  ncchannel_rgb8(c, &r, &g, &b);
  CHECK(0 == r);
  CHECK(0 == g);
  CHECK(0 == b);
  CHECK(2 == blends);
  c = channels_blend(c1, c2, &blends); // will not replace
  CHECK(ncchannel_default_p(c));
  ncchannel_rgb8(c, &r, &g, &b);
  CHECK(0 == r);
  CHECK(0 == g);
  CHECK(0 == b);
  CHECK(3 == blends);
}

// you can't blend from a default color, but blend 0 sets it
TEST_CASE("ChannelBlendDefaultRight") {
  uint32_t c1 = 0;
  uint32_t c2 = 0;
  ncchannel_set_rgb8(&c1, 0x80, 0x40, 0x20);
  CHECK(!ncchannel_default_p(c1));
  CHECK(ncchannel_default_p(c2));
  unsigned blends = 0;
  uint32_t c = channels_blend(c1, c2, &blends);
  CHECK(ncchannel_default_p(c));
  CHECK(1 == blends);
  c = channels_blend(c1, c2, &blends);
  CHECK(!ncchannel_default_p(c));
  unsigned r, g, b;
  ncchannel_rgb8(c, &r, &g, &b);
  CHECK(0x80 == r);
  CHECK(0x40 == g);
  CHECK(0x20 == b);
  CHECK(2 == blends);
}
