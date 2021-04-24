#include "main.h"
#include "visual-details.h"
#include <vector>
#include <iostream>

// convert the sprixel at s having pixel dimensions dimyXdimx to an rgb(a)
// matrix for easier analysis. breaks on malformed sixels.
std::vector<uint32_t> sixel_to_rgb(const char* s, int dimy, int dimx) {
  std::vector<uint32_t> bmap(dimy * dimx, 0x00000000ull);
  std::vector<uint32_t> colors;
  // first we skip the header
  while(*s != '#'){
    ++s;
  }
  // now we build the color table (form: #Pc;Pu;Px;Py;Pz). data starts with
  // a color spec lacking a semicolon.
  enum {
    STATE_WANT_HASH,
    STATE_WANT_COLOR,
    STATE_WANT_COLORSEMI,
    STATE_WANT_COLORSPACE,
    STATE_WANT_DATA,
  } state = STATE_WANT_HASH;
  unsigned color = 0;
  unsigned x = 0;
  unsigned y = 0;
  unsigned rle = 1;
  while(*s){
    if(*s == '\e'){
      break;
    }
    if(state == STATE_WANT_HASH){
      REQUIRE('#' == *s);
      state = STATE_WANT_COLOR;
    }else if(state == STATE_WANT_COLOR){
      CHECK(isdigit(*s));
      color = 0;
      do{
        color *= 10;
        color += *s - '0';
        ++s;
      }while(isdigit(*s));
//std::cerr << "Got color " << color << std::endl;
      --s;
      state = STATE_WANT_COLORSEMI;
    }else if(state == STATE_WANT_COLORSEMI){
      // if we get a semicolon, we're a colorspec, otherwise data
      if(*s == ';'){
        state = STATE_WANT_COLORSPACE;
      }else{
        state = STATE_WANT_DATA;
        rle = 1;
      }
    }else if(state == STATE_WANT_COLORSPACE){
      CHECK('2' == *(s++));
      CHECK(';' == *(s++));
      int r = 0;
      do{
        r *= 10;
        r += *s - '0';
        ++s;
      }while(isdigit(*s));
      CHECK(';' == *(s++));
      int g = 0;
      do{
        g *= 10;
        g += *s - '0';
        ++s;
      }while(isdigit(*s));
      CHECK(';' == *(s++));
      int b = 0;
      do{
        b *= 10;
        b += *s - '0';
        ++s;
      }while(isdigit(*s));
      uint32_t rgb = 0xff000000 + (r << 16u) * 255 / 100 + (g << 8u) * 255 / 100 + b * 255 / 100;
//std::cerr << "Got color " << color << ": " << r << "/" << g << "/" << b << std::endl;
      if(color >= colors.capacity()){
        colors.resize(color + 1);
      }
      colors[color] = rgb;
      state = STATE_WANT_HASH;
      --s;
    }
    // read until we hit next colorspec
    if(state == STATE_WANT_DATA){
//std::cerr << "Character " << *s << std::endl;
      if(*s == '#'){
        state = STATE_WANT_HASH;
        --s;
      }else if(*s == '!'){ // RLE
        ++s;
        rle = 0;
        do{
          rle *= 10;
          rle += *s - '0';
          ++s;
        }while(isdigit(*s));
        CHECK(2 < rle);
        --s;
      }else if(*s == '$'){
        x = 0;
        state = STATE_WANT_HASH;
      }else if(*s == '-'){
        x = 0;
        y += 6;
        state = STATE_WANT_HASH;
      }else{
//std::cerr << "RLE: " << rle << " pos: " << y << "*" << x << std::endl;
        for(unsigned xpos = x ; xpos < x + rle ; ++xpos){
          for(unsigned ypos = y ; ypos < y + 6 ; ++ypos){
            if((*s - 63) & (1u << (ypos - y))){
              // ought be an empty pixel
              CHECK(0x00000000ull == bmap[ypos * dimx + xpos]);
//std::cerr << *s << " BMAP[" << ypos << "][" << xpos << "] = " << std::hex << colors[color] << std::dec << std::endl;
              bmap[ypos * dimx + xpos] = colors[color];
            }
          }
        }
        x += rle;
        rle = 1;
      }
    }
    ++s;
  }
  return bmap;
}

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

  // this can only run with a Sixel backend
  if(notcurses_check_pixel_support(nc_) <= 0){
    return;
  }
  if(nc_->tcache.color_registers <= 0){
    return;
  }

#ifdef NOTCURSES_USE_MULTIMEDIA
  SUBCASE("SixelRoundtrip") {
    CHECK(1 == ncplane_set_base(n_, "&", 0, 0));
    auto ncv = ncvisual_from_file(find_data("worldmap.png"));
    REQUIRE(ncv);
    struct ncvisual_options vopts{};
    vopts.blitter = NCBLIT_PIXEL;
    vopts.flags = NCVISUAL_OPTION_NODEGRADE;
    auto newn = ncvisual_render(nc_, ncv, &vopts);
    CHECK(newn);
    CHECK(0 == notcurses_render(nc_));
    auto rgb = sixel_to_rgb(newn->sprite->glyph, newn->sprite->pixy, newn->sprite->pixx);
    for(int y = 0 ; y < newn->sprite->pixy ; ++y){
      for(int x = 0 ; x < newn->sprite->pixx ; ++x){
//fprintf(stderr, "%03d/%03d NCV: %08x RGB: %08x\n", y, x, ncv->data[y * newn->sprite->pixx + x], rgb[y * newn->sprite->pixx + x]);
        // FIXME
        //CHECK(ncv->data[y * newn->sprite->pixx + x] == rgb[y * newn->sprite->pixx + x]);
      }
    }
    ncvisual_destroy(ncv);
  }

  SUBCASE("SixelBlit") {
    CHECK(1 == ncplane_set_base(n_, "&", 0, 0));
    auto ncv = ncvisual_from_file(find_data("natasha-blur.png"));
    REQUIRE(ncv);
    struct ncvisual_options vopts{};
    vopts.blitter = NCBLIT_PIXEL;
    vopts.flags = NCVISUAL_OPTION_NODEGRADE;
    auto newn = ncvisual_render(nc_, ncv, &vopts);
    CHECK(newn);
    auto rgbold = sixel_to_rgb(newn->sprite->glyph, newn->sprite->pixy, newn->sprite->pixx);
//print_bmap(rgbold, newn->sprite->pixy, newn->sprite->pixx);
    CHECK(0 == notcurses_render(nc_));
    struct ncplane_options nopts = {
      .y = ncplane_dim_y(newn) * 3 / 4,
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
    uint64_t chan = CHANNELS_RGB_INITIALIZER(0, 0, 0, 0, 0, 0);
    CHECK(1 == ncplane_set_base(blockerplane, " ", 0, chan));
    CHECK(0 == notcurses_render(nc_));
    CHECK(1 == ncplane_set_base(n_, "%", 0, 0));
    CHECK(0 == notcurses_render(nc_));
    // FIXME at this point currently, we get a degraded back of the orca
    // test via conversion back to image? unsure
    auto rgbnew = sixel_to_rgb(newn->sprite->glyph, newn->sprite->pixy, newn->sprite->pixx);
//print_bmap(rgbnew, newn->sprite->pixy, newn->sprite->pixx);
    CHECK(0 == ncplane_destroy(newn));
    CHECK(0 == ncplane_destroy(blockerplane));
    ncvisual_destroy(ncv);
  }
#endif

  CHECK(!notcurses_stop(nc_));
}
