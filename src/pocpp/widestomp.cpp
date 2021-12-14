#include <cstdlib>
#include <clocale>
#include <unistd.h>
#include <memory>
#include <ncpp/NotCurses.hh>
#include <ncpp/Plane.hh>

using namespace ncpp;

constexpr auto DELAY = 1;

// dump two wide glyphs, then create a new plane and drop it atop them

auto stomper(NotCurses& nc, std::shared_ptr<Plane>& nn) -> int {
  // should knock out both wide glyphs
  nn->move(0, 1);
  nc.render();
  sleep(DELAY);

  // first wide glyph gone, second present
  nn->move(1, 0);
  nc.render();
  sleep(DELAY);

  // second wide glyph gone, first present
  nn->move(2, 2);
  nc.render();
  sleep(DELAY);

  nn->move(4, 0);
  nc.render();
  sleep(DELAY);

  nn->move(5, 1);
  nc.render();
  sleep(DELAY);

  nn->move(6, 2);
  nc.render();
  sleep(DELAY);

  return 0;
}

auto main() -> int {
  setlocale(LC_ALL, "");
  notcurses_options nopts{};
  nopts.flags = NCOPTION_INHIBIT_SETLOCALE
                | NCOPTION_DRAIN_INPUT;
  NotCurses nc(nopts);
  std::shared_ptr<Plane> n(nc.get_stdplane());

  {
    // first, a 2x1 with "AB"
    auto nn = std::make_shared<Plane>(1, 2, 1, 16);
    nn->set_fg_rgb8(0xc0, 0x80, 0xc0);
    nn->set_bg_rgb8(0x20, 0x00, 0x20);
    nn->set_base("", 0, NCCHANNELS_INITIALIZER(0xc0, 0x80, 0xc0, 0x20, 0, 0x20));
    nn->putstr("AB");

    n->set_fg_rgb8(0x80, 0xc0, 0x80);
    n->set_bg_rgb8(0x00, 0x40, 0x00);
    n->putstr("\xe5\xbd\xa2\xe5\x85\xa8");
    n->putstr(1, 0, "\xe5\xbd\xa2\xe5\x85\xa8");
    n->putstr(2, 0, "\xe5\xbd\xa2\xe5\x85\xa8");
    n->putstr(3, 0, "\xe5\xbd\xa2\xe5\x85\xa8");
    n->putstr(4, 0, "abcdef");
    n->putstr(5, 0, "abcdef");
    n->putstr(6, 0, "abcdef");
    n->putstr(7, 0, "abcdef");
    nc.render();
    sleep(1);

    stomper(nc, nn);
    if(nn->putstr(0, 0, "\xe5\xbd\xa1") <= 0){
      return EXIT_FAILURE;
    }
    stomper(nc, nn);
    nn->erase();
    if(nn->putstr(0, 0, "r") <= 0){
      return EXIT_FAILURE;
    }
    stomper(nc, nn);
  }

  // now a 1x1 "*"
  auto nn = std::make_shared<Plane>(1, 1, 1, 16);
  if(nn->putstr(0, 0, "r") <= 0){
    return EXIT_FAILURE;
  }
  stomper(nc, nn);
  return EXIT_SUCCESS;
}
