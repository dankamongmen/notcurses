#ifndef NOTCURSES_SIXEL
#define NOTCURSES_SIXEL

#ifdef __cplusplus
extern "C" {
#endif

#include <ctype.h>
#include <stdlib.h>

uint32_t* ncsixel_as_rgba(const char *sx, unsigned leny, unsigned lenx){
#define MAXCOLORS 65535
  // cast is necessary for c++ callers
  uint32_t* rgba = (typeof rgba)malloc(sizeof(*rgba) * leny * lenx);
  if(rgba == NULL){
    return NULL;
  }
  // cast is necessary for c++ callers
  uint32_t* colors = (typeof colors)malloc(sizeof(*colors) * MAXCOLORS);
  if(colors == NULL){
    free(rgba);
    return NULL;
  }
  // first we skip the header
  while(*sx != '#'){
    ++sx;
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
  while(*sx){
    if(*sx == '\e'){
      break;
    }
    if(state == STATE_WANT_HASH){
      if('#' != *sx){
        goto err;
      }
      state = STATE_WANT_COLOR;
    }else if(state == STATE_WANT_COLOR){
      if(!isdigit(*sx)){
        goto err;
      }
      color = 0;
      do{
        color *= 10;
        color += *sx - '0';
        ++sx;
      }while(isdigit(*sx));
//std::cerr << "Got color " << color << std::endl;
      --sx;
      state = STATE_WANT_COLORSEMI;
    }else if(state == STATE_WANT_COLORSEMI){
      // if we get a semicolon, we're a colorspec, otherwise data
      if(*sx == ';'){
        state = STATE_WANT_COLORSPACE;
      }else{
        state = STATE_WANT_DATA;
        rle = 1;
      }
    }else if(state == STATE_WANT_COLORSPACE){
      if('2' != *(sx++)){
        goto err;
      }
      if(';' != *(sx++)){
        goto err;
      }
      int r = 0;
      do{
        r *= 10;
        r += *sx - '0';
        ++sx;
      }while(isdigit(*sx));
      if(';' != *(sx++)){
        goto err;
      }
      int g = 0;
      do{
        g *= 10;
        g += *sx - '0';
        ++sx;
      }while(isdigit(*sx));
      if(';' != *(sx++)){
        goto err;
      }
      int b = 0;
      do{
        b *= 10;
        b += *sx - '0';
        ++sx;
      }while(isdigit(*sx));
      uint32_t rgb = htole(0xff000000 + (r << 16u) * 255 / 100 + (g << 8u) * 255 / 100 + b * 255 / 100);
//std::cerr << "Got color " << color << ": " << r << "/" << g << "/" << b << std::endl;
      if(color >= MAXCOLORS){
        goto err;
      }
      colors[color] = rgb;
      state = STATE_WANT_HASH;
      --sx;
    }
    // read until we hit next colorspec
    if(state == STATE_WANT_DATA){
//std::cerr << "Character " << *sx << std::endl;
      if(*sx == '#'){
        state = STATE_WANT_HASH;
        --sx;
      }else if(*sx == '!'){ // RLE
        ++sx;
        rle = 0;
        do{
          rle *= 10;
          rle += *sx - '0';
          ++sx;
        }while(isdigit(*sx));
        if(2 >= rle){
          goto err;
        }
        --sx;
      }else if(*sx == '$'){
        x = 0;
        state = STATE_WANT_HASH;
      }else if(*sx == '-'){
        x = 0;
        y += 6;
        state = STATE_WANT_HASH;
      }else{
//std::cerr << "RLE: " << rle << " pos: " << y << "*" << x << std::endl;
        for(unsigned xpos = x ; xpos < x + rle ; ++xpos){
          for(unsigned ypos = y ; ypos < y + 6 ; ++ypos){
            if((*sx - 63) & (1u << (ypos - y))){
              // ought be an empty pixel
//std::cerr << *s << " BMAP[" << ypos << "][" << xpos << "] = " << std::hex << colors[color] << std::dec << std::endl;
              rgba[ypos * lenx + xpos] = colors[color];
            }
          }
        }
        x += rle;
        rle = 1;
      }
    }
    ++sx;
  }
  free(colors);
  return rgba;

err:
  free(rgba);
  free(colors);
  return NULL;
#undef MAXCOLORS
}

#ifdef __cplusplus
}
#endif

#endif
