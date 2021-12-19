#include "main.h"

// These tests primarily check against expected constant values, which is
// useful in terms of maintaining ABI, but annoying when they do change.
TEST_CASE("Channels") {

  SUBCASE("ChannelGetRGB") {
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

  SUBCASE("ChannelGetAlpha") {
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

  SUBCASE("ChannelGetDefault") {
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

  SUBCASE("ChannelSetDefault") {
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
  SUBCASE("ChannelBlend0") {
    uint32_t c1 = 0;
    uint32_t c2 = 0;
    ncchannel_set_rgb8(&c1, 0x80, 0x40, 0x20);
    ncchannel_set_rgb8(&c2, 0x88, 0x44, 0x22);
    unsigned blends = 0;
    uint32_t c = channels_blend(nullptr, c1, c2, &blends, 0);
    CHECK(!ncchannel_default_p(c));
    unsigned r, g, b;
    ncchannel_rgb8(c, &r, &g, &b);
    CHECK(0x88 == r);
    CHECK(0x44 == g);
    CHECK(0x22 == b);
    CHECK(1 == blends);
  }

  // blend of 1 ought perfectly average c1 and c2
  SUBCASE("ChannelBlend1") {
    uint32_t c1 = 0;
    uint32_t c2 = 0;
    ncchannel_set_rgb8(&c1, 0x80, 0x40, 0x20);
    ncchannel_set_rgb8(&c2, 0x0, 0x0, 0x0);
    unsigned blends = 1;
    uint32_t c = channels_blend(nullptr, c1, c2, &blends, 0);
    CHECK(!ncchannel_default_p(c));
    unsigned r, g, b;
    ncchannel_rgb8(c, &r, &g, &b);
    CHECK(0x40 == r);
    CHECK(0x20 == g);
    CHECK(0x10 == b);
    CHECK(2 == blends);
  }

  // blend of 2 ought weigh c1 twice as much as c2
  SUBCASE("ChannelBlend2") {
    uint32_t c1 = 0;
    uint32_t c2 = 0;
    ncchannel_set_rgb8(&c1, 0x60, 0x30, 0x0f);
    ncchannel_set_rgb8(&c2, 0x0, 0x0, 0x0);
    unsigned blends = 2;
    uint32_t c = channels_blend(nullptr, c1, c2, &blends, 0);
    CHECK(!ncchannel_default_p(c));
    unsigned r, g, b;
    ncchannel_rgb8(c, &r, &g, &b);
    CHECK(0x40 == r);
    CHECK(0x20 == g);
    CHECK(0x0a == b);
    CHECK(3 == blends);
  }

  // blending the default color in ought use the provided default
  SUBCASE("ChannelBlendDefaultLeft") {
    uint32_t c1 = 0;
    uint32_t c2 = 0;
    ncchannel_set_rgb8(&c2, 0x80, 0x40, 0x20);
    unsigned blends = 0;
    uint32_t c = channels_blend(nullptr, c1, c2, &blends, 0); // will replace
    CHECK(!ncchannel_default_p(c));
    unsigned r, g, b;
    ncchannel_rgb8(c, &r, &g, &b);
    CHECK(0x80 == r);
    CHECK(0x40 == g);
    CHECK(0x20 == b);
    CHECK(1 == blends);
    c = channels_blend(nullptr, c, c1, &blends, 0x666666); // blends 0x666666 into 0x804020
    ncchannel_rgb8(c, &r, &g, &b);
    CHECK(0x73 == r);
    CHECK(0x53 == g);
    CHECK(0x43 == b);
    CHECK(2 == blends);
    c = channels_blend(nullptr, c1, c2, &blends, 0x666666); // blends 0x804020 into 0x666666x2
    ncchannel_rgb8(c, &r, &g, &b);
    CHECK(0x6e == r);
    CHECK(0x59 == g);
    CHECK(0x4e == b);
    CHECK(3 == blends);
  }

  // test that colors are inverted, but nothing else
  SUBCASE("ChannelsReverse") {
    uint64_t channels = 0;
    uint64_t rev = ncchannels_reverse(channels);
    CHECK(0 == rev);
    ncchannels_set_fg_palindex(&channels, 8);
    rev = ncchannels_reverse(channels);
    CHECK(8 == ncchannels_bg_palindex(rev));
    CHECK(0 == ncchannels_fg_palindex(rev));
    ncchannels_set_fg_palindex(&rev, 63);
    rev = ncchannels_reverse(rev);
    CHECK(8 == ncchannels_fg_palindex(rev));
    CHECK(63 == ncchannels_bg_palindex(rev));
    ncchannels_set_fg_default(&rev);
    ncchannels_set_bg_alpha(&rev, NCALPHA_TRANSPARENT);
    rev = ncchannels_reverse(rev);
    CHECK(63 == ncchannels_fg_palindex(rev));
    CHECK(ncchannels_bg_default_p(rev));
    CHECK(NCALPHA_OPAQUE == ncchannels_bg_alpha(rev));
    ncchannels_set_fg_rgb(&rev, 0x2288cc);
    rev = ncchannels_reverse(rev);
    CHECK(0x2288cc == ncchannels_bg_rgb(rev));
    CHECK(ncchannels_fg_default_p(rev));
  }

}
