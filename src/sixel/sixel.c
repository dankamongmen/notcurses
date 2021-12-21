#include "sixel/sixel.h"
#include <notcurses/notcurses.h>

// represents a sixel generated from some image
typedef struct sixel {
  char* escape; // nul-terminated escape suitable for writing to the terminal
  // there is both the true pixel geometry, and the sprixel-padded pixel
  // geometry--the latter always has a height which is a multiple of six.
  unsigned pixy, pixx;           // original pixel geometry
  unsigned sprixpixy, sprixpixx; // sprixel-padded pixel geometry
  // we might only occupy a portion of the final column and row of cells.
  unsigned celly, cellx;         // cell geometry
  unsigned colorregs_avail;      // color registers available
  unsigned colorregs_used;       // color registers used
} sixel;

typedef struct sixelctx {
  struct notcurses* nc;
} sixelctx;

sixelctx* libncsixel_init(void){
  sixelctx* sctx = malloc(sizeof(*sctx));
  if(sctx == NULL){
    return NULL;
  }
  struct notcurses_options nopts = {
    .flags = NCOPTION_NO_ALTERNATE_SCREEN |
             NCOPTION_PRESERVE_CURSOR |
             NCOPTION_SUPPRESS_BANNERS |
             NCOPTION_DRAIN_INPUT,
  };
  if((sctx->nc = notcurses_init(&nopts, NULL)) == NULL){
    free(sctx);
    return NULL;
  }
  return sctx;
}

sixel* libncsixel_encode(sixelctx* sctx, const char* file, unsigned colorregs){
  (void)sctx;
  (void)file;
  (void)colorregs;
  return NULL;
}

uint32_t* libncsixel_explode(const sixel* s){
  uint32_t* rgba = malloc(sizeof(*rgba) * s->pixy * s->pixx);
  if(rgba == NULL){
    return NULL;
  }
  uint32_t* colors = malloc(sizeof(*colors) * s->colorregs_used);
  if(colors == NULL){
    free(rgba);
    return NULL;
  }
  const char* sx = s->escape;
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
        ++s;
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
      if(color >= s->colorregs_used){
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
              rgba[ypos * s->pixx + xpos] = colors[color];
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
}

void libncsixel_stop(sixelctx* sctx){
  if(sctx){
    notcurses_stop(sctx->nc);
    free(sctx);
  }
}
