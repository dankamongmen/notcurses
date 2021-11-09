#include "main.h"
#include "ncpp/Visual.hh"

using namespace ncpp;

TEST_CASE("Ncpp"
          * doctest::description("Basic C++ wrapper tests")) {

  notcurses_options nopts{};
  nopts.flags = NCOPTION_SUPPRESS_BANNERS |
                NCOPTION_INHIBIT_SETLOCALE;
  nopts.loglevel = NCLOGLEVEL_VERBOSE;

  // we ought be able to construct a NotCurses object with a nullptr FILE
  // or even just no argument (decays to nullptr).
  SUBCASE("ConstructNotCurses") {
    NotCurses nc{ nopts };
    CHECK(!NotCurses::is_notcurses_stopped(&nc));
    CHECK(nc.stop());
    CHECK(NotCurses::is_notcurses_stopped(&nc));
  }

  SUBCASE("ConstructNotCursesNullFILE") {
    NotCurses ncnull{ nopts, nullptr };
    CHECK(!NotCurses::is_notcurses_stopped(&ncnull));
    CHECK(ncnull.stop());
    CHECK(NotCurses::is_notcurses_stopped(&ncnull));
  }

  // we ought be able to get a new NotCurses object after stop()ping one.
  SUBCASE("ConstructNotCursesTwice") {
    NotCurses nc{ nopts };
    CHECK(nc.stop());
    CHECK(NotCurses::is_notcurses_stopped(&nc));
    NotCurses ncnull{ nopts, nullptr };
    CHECK(!NotCurses::is_notcurses_stopped(&ncnull));
    CHECK(ncnull.stop());
    CHECK(NotCurses::is_notcurses_stopped(&ncnull));
  }

  SUBCASE("StdPlane") {
    NotCurses nc{ nopts };
    auto std1 = nc.get_stdplane();
    CHECK(nullptr != std1);
    unsigned y, x;
    auto std2 = nc.get_stdplane(&y, &x);
    CHECK(nullptr != std2);
    CHECK(0 < x);
    CHECK(0 < y);
    CHECK(nc.stop());
  }

  SUBCASE("Debug") { // just test to ensure it doesn't coredump
    NotCurses nc{ nopts };
    nc.debug(stderr);
    CHECK(nc.stop());
  }

  SUBCASE("GetPaletteSize") {
    NotCurses nc{ nopts };
    CHECK(0 < nc.get_palette_size());
    CHECK(nc.stop());
  }

  SUBCASE("VisualFromFile") {
    NotCurses nc{ nopts };
    if(nc.can_open_images()){
      {
        Visual v = Visual(find_data("changes.jpg").get());
      }
    }
    CHECK(nc.stop());
  }

  SUBCASE("VisualFromRGBA") {
    NotCurses nc{ nopts };
    uint32_t rgba[] = {
      0x4080c0ff,
      0x105090ff,
    };
    {
      Visual v = Visual(rgba, 1, sizeof(rgba), sizeof(rgba) / sizeof(*rgba));
      // FIXME check geometry
    }
    CHECK(nc.stop());
  }

  SUBCASE("VisualFromPlane") {
    NotCurses nc{ nopts };
    {
      auto n = nc.get_stdplane();
      uint64_t chan = NCCHANNELS_INITIALIZER(0x22, 0xdd, 0x44, 0, 0, 0);
      n->set_base(" ", 0, chan);
      REQUIRE(n);
      // FIXME load it into visual, erase plane, render visual, check for equivalence...
      {
        Visual v = Visual(*n, NCBLIT_1x1, 0, 0, 0, 0);
      }
    }
    CHECK(nc.stop());
  }

}
