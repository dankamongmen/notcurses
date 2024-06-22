#include "main.h"
#include "lib/visual-details.h"
#include <vector>
#include <iostream>
#include "lib/sixel.h"

/*
void print_bmap(const std::vector<uint32_t> rgba, int pixy, int pixx){
  for(int y = 0 ; y < pixy ; ++y){
    for(int x = 0 ; x < pixx ; ++x){
      std::cerr << "rgba[" << y << "][" << x << "] (" << y * x << "): " << std::hex << rgba[y * pixx + x] << std::dec << std::endl;
    }
  }
}
*/

TEST_CASE("Sixels") {
  auto nc_ = testing_notcurses();
  REQUIRE(nullptr != nc_);
  ncplane* ncp_ = notcurses_stdplane(nc_);
  REQUIRE(ncp_);
  auto n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

  // exercise ncvisual_from_sixel()
  SUBCASE("LoadSixel") {
    auto ncv = ncvisual_from_sixel("\x1bP0;1;0q\"1;1;1;6#0;2;66;18;80#0@-\x1b\\", 1, 1);
    REQUIRE(ncv);
    uint32_t p;
    ncvisual_at_yx(ncv, 0, 0, &p);
    CHECK(0xff == ncpixel_a(p));
    CHECK(0xa8 == ncpixel_r(p));
    CHECK(0x2d == ncpixel_g(p));
    CHECK(0xcc == ncpixel_b(p));
    ncvisual_destroy(ncv);
  }

  SUBCASE("LoadBigSixel") {
    auto ncv = ncvisual_from_sixel("\x1bPq\"1;1;22;14#0;2;9;13;13#1;2;9;16;19#2;2;16;22;25#3;2;53;56;56#4;2;66;63;60#5;2;72;75;69#6;2;69;72;66#7;2;60;63;63#8;2;35;41;44#9;2;28;35;35#10;2;25;28;28#11;2;19;22;25#12;2;0;0;0#13;2;16;19;22#14;2;31;38;38#15;2;63;66;66#16;2;19;25;28#17;2;3;3;6#18;2;47;50;50#19;2;69;72;72#20;2;25;31;31#21;2;6;9;13#22;2;47;47;47#23;2;6;13;19#24;2;47;53;50#25;2;19;19;19#26;2;6;6;13#27;2;44;41;38#28;2;56;53;53#29;2;50;47;47#30;2;19;16;16#31;2;47;50;47#32;2;63;60;56#33;2;47;47;44#34;2;25;25;28#35;2;35;31;31#36;2;53;50;50#37;2;6;3;6#1eLB#14A!7?_K???GO#12@^~~$#0H#13qK#18C???_!6?_#11BDC#0G#26_$#23O#20?_!9?_OC#2?A#13AO$#16??O#3G@??O_?U#22G???_O#25__$#7???O!6?H#8V?_WG#1?@#17A$#4???__@#15o?]M#24_!4?O#21??C$#2???@#5[[FB?o#9??RN#10@C_G$#6!4?Aa??@@#16!4?A$#19!6?GK-#0KO!4?G!6?oG?`X???C$#1@B#10_??_?G???GwCA!4?C#12_`$#21o#13K???G#30oo!5?GCA??F`#1K$#23A#17_#22O?CC??@??A?A#17_G!4?PQ$#27??@!4?CgG#3A#20?@!5?_O$#28??KH??@A?_#8??A@@#11DC_#25GA$#31??A#4_?A#35C?O??o#37??OO#21W???AG$#32???U#14O!6?@C!6?G$#9!4?G!6?C#26???_#34AAO$#29!4?`@#33A?CCG#13!6?C$#5!4?A#25O#24?@#18A?D$#36!9?Oo$#7!9?B-#0B?@#8@A#36@#27@!4?A#10A??AA@#17A@$?BA#1A#32@A??@AA#35@#34@#11BB#13@@A@#12ABB$#28!6?AA?@@$#29!7?@#4A\x1b\\", 18, 22);
    REQUIRE(ncv);
    ncvisual_destroy(ncv);
  }

  SUBCASE("LoadBiggerSixel") {
    auto ncv = ncvisual_from_sixel("\x1bPq\"1;1;18;18#0;2;0;0;0#1;2;6;6;0#2;2;35;28;9#3;2;69;56;19#4;2;91;72;28#5;2;97;78;28#6;2;91;72;25#7;2;94;75;28#8;2;72;53;16#9;2;56;41;6#10;2;78;63;19#11;2;60;44;9#12;2;85;66;22#13;2;53;44;22#14;2;60;72;75#15;2;75;63;31#16;2;66;56;22#17;2;50;50;44#18;2;25;35;35#19;2;88;85;82#20;2;19;35;50#21;2;38;47;44#22;2;47;38;16#23;2;13;22;31#24;2;47;47;31#25;2;41;25;0#26;2;0;3;3#27;2;53;38;6#1O???@!8?@???O$#2_GCA?@!6?@?ACG_$#0NFB@#8O!8?O#0@BFN$#3?O??A?@!4?@?A??O$#5?_OGKEm}~~}mEKGO_$#7??_O?G!6?G?O_$#6??GC#9?O!6?O#6?CG$#11???__!8?__$#12!5?_!6?_$#10!6?O!4?O$#4!7?@??@-Q??C???GSSG???C??Q$#5KLC??HH@JJ@HH??CLK$#3@#7QG?@!8?@?GQ#3@$#16_#14_oww!8?wwo_#16_$#11??A@?AA!4?AA?@A$#8??@#6A???O??O???A#8@$#12!4?A?OA??AO?A$#13!4?C?!6_?C$#9!5?CCC??CCC$#15!5?O!6?O$#17!5?_!6?_-#0oo__#1_!8?_#0__oo$#26G?O#2O?_!6?_?O#26O?G$#20AG#3??O?_!4?_?O#20??GA$#18@#14FNNN@!6?@NNNF#18@$#23C#4!4?G?_OO_?G#23!4?C$#5!5?OOO__OOO$#21!5?A!6?A$#24!5?C!6?C$#8!6?G!4?G$#19!6?!6@$#22!6?A!4?A$#25!6?!6C$#9!7?G??G$#17!7?A??A$#15!8?AA$#27!8?GG\x1b\\", 1, 1);
    REQUIRE(ncv);
    ncvisual_destroy(ncv);
  }

  // remaining tests can only run with a Sixel backend
  if(notcurses_check_pixel_support(nc_) <= 0){
    CHECK(0 == notcurses_stop(nc_));
    return;
  }
  if(nc_->tcache.color_registers <= 0){
    CHECK(0 == notcurses_stop(nc_));
    return;
  }
#ifdef NOTCURSES_USE_MULTIMEDIA
  SUBCASE("SixelRoundtrip") {
    CHECK(1 == ncplane_set_base(n_, "&", 0, 0));
    auto ncv = ncvisual_from_file(find_data("worldmap.png").get());
    REQUIRE(ncv);
    struct ncvisual_options vopts{};
    vopts.n = n_;
    vopts.blitter = NCBLIT_PIXEL;
    vopts.flags = NCVISUAL_OPTION_NODEGRADE | NCVISUAL_OPTION_CHILDPLANE;
    vopts.scaling = NCSCALE_STRETCH;
    auto newn = ncvisual_blit(nc_, ncv, &vopts);
    REQUIRE(nullptr != newn);
    CHECK(0 == notcurses_render(nc_));
    auto rgb = ncsixel_as_rgba(newn->sprite->glyph.buf, newn->sprite->pixy, newn->sprite->pixx);
    REQUIRE(rgb);
    for(int y = 0 ; y < newn->sprite->pixy ; ++y){
      for(int x = 0 ; x < newn->sprite->pixx ; ++x){
//fprintf(stderr, "%03d/%03d NCV: %08x RGB: %08x\n", y, x, ncv->data[y * newn->sprite->pixx + x], rgb[y * newn->sprite->pixx + x]);
        // FIXME
        //CHECK(ncv->data[y * newn->sprite->pixx + x] == rgb[y * newn->sprite->pixx + x]);
      }
    }
    free(rgb);
    ncvisual_destroy(ncv);
  }

  SUBCASE("SixelBlit") {
    CHECK(1 == ncplane_set_base(n_, "&", 0, 0));
    auto ncv = ncvisual_from_file(find_data("natasha-blur.png").get());
    REQUIRE(ncv);
    struct ncvisual_options vopts{};
    vopts.n = n_;
    vopts.blitter = NCBLIT_PIXEL;
    vopts.flags = NCVISUAL_OPTION_NODEGRADE | NCVISUAL_OPTION_CHILDPLANE;
    vopts.scaling = NCSCALE_STRETCH;
    auto newn = ncvisual_blit(nc_, ncv, &vopts);
    REQUIRE(nullptr != newn);
    auto rgbold = ncsixel_as_rgba(newn->sprite->glyph.buf, newn->sprite->pixy, newn->sprite->pixx);
    REQUIRE(rgbold);
//print_bmap(rgbold, newn->sprite->pixy, newn->sprite->pixx);
    CHECK(0 == notcurses_render(nc_));
    struct ncplane_options nopts = {
      .y = (int)ncplane_dim_y(newn) * 3 / 4,
      .x = 0,
      .rows = ncplane_dim_y(newn) / 4,
      .cols = ncplane_dim_x(newn) / 2,
      .userptr = nullptr, .name = "blck",
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto blockerplane = ncplane_create(newn, &nopts);
    REQUIRE(nullptr != blockerplane);
    uint64_t chan = NCCHANNELS_INITIALIZER(0, 0, 0, 0, 0, 0);
    CHECK(1 == ncplane_set_base(blockerplane, " ", 0, chan));
    CHECK(0 == notcurses_render(nc_));
    CHECK(1 == ncplane_set_base(n_, "%", 0, 0));
    CHECK(0 == notcurses_render(nc_));
    // FIXME at this point currently, we get a degraded back of the orca
    // test via conversion back to image? unsure
    auto rgbnew = ncsixel_as_rgba(newn->sprite->glyph.buf, newn->sprite->pixy, newn->sprite->pixx);
    REQUIRE(rgbnew);
//print_bmap(rgbnew, newn->sprite->pixy, newn->sprite->pixx);
    CHECK(0 == ncplane_destroy(newn));
    CHECK(0 == ncplane_destroy(blockerplane));
    free(rgbnew);
    free(rgbold);
    ncvisual_destroy(ncv);
  }
#endif

  CHECK(!notcurses_stop(nc_));
}
