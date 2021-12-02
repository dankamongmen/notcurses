#include <stdlib.h>
#include <string.h>
#include "demo.h"

// FIXME do the bigger dimension on the screen's bigger dimension
#define CHUNKS_VERT 6
#define CHUNKS_HORZ 12
#define MOVES 20

static int
move_square(struct notcurses* nc, struct ncplane* chunk, int* holey, int* holex,
            uint64_t movens){
  int newholex, newholey;
  ncplane_yx(chunk, &newholey, &newholex);
  // we need to move from newhole to hole over the course of movetime
  int deltay, deltax;
  deltay = *holey - newholey;
  deltax = *holex - newholex;
  // divide movetime into abs(max(deltay, deltax)) units, and move delta
  int units = abs(deltay) > abs(deltax) ? abs(deltay) : abs(deltax);
  movens /= units;
  struct timespec movetime;
  ns_to_timespec(movens, &movetime);
  int i;
  int targy = newholey;
  int targx = newholex;
  deltay = deltay < 0 ? -1 : deltay == 0 ? 0 : 1;
  deltax = deltax < 0 ? -1 : deltax == 0 ? 0 : 1;
  // FIXME do an adaptive time, like our fades, so we whip along under load
  for(i = 0 ; i < units ; ++i){
    targy += deltay;
    targx += deltax;
    ncplane_move_yx(chunk, targy, targx);
    DEMO_RENDER(nc);
    demo_nanosleep(nc, &movetime);
  }
  *holey = newholey;
  *holex = newholex;
  return 0;
}

// we take demodelay * 5 to play MOVES moves
static int
play(struct notcurses* nc, struct ncplane** chunks, uint64_t startns){
  const uint64_t delayns = timespec_to_ns(&demodelay);
  const int chunkcount = CHUNKS_VERT * CHUNKS_HORZ;
  // we don't want to spend more than demodelay * 5
  const uint64_t totalns = delayns * 5;
  const uint64_t deadline_ns = startns + totalns;
  const uint64_t movens = totalns / MOVES;
  int hole = rand() % chunkcount;
  int holex, holey;
  ncplane_yx(chunks[hole], &holey, &holex);
  ncplane_destroy(chunks[hole]);
  chunks[hole] = NULL;
  int m;
  int lastdir = -1;
  for(m = 0 ; m < MOVES ; ++m){
    uint64_t now = clock_getns(CLOCK_MONOTONIC);
    if(now >= deadline_ns){
      break;
    }
    int mover = chunkcount;
    int direction;
    do{
      direction = rand() % 4;
      switch(direction){
        case 3: // up
          if(lastdir != 1 && hole >= CHUNKS_HORZ){ mover = hole - CHUNKS_HORZ; } break;
        case 2: // right
          if(lastdir != 0 && hole % CHUNKS_HORZ < CHUNKS_HORZ - 1){ mover = hole + 1; } break;
        case 1: // down
          if(lastdir != 3 && hole < chunkcount - CHUNKS_HORZ){ mover = hole + CHUNKS_HORZ; } break;
        case 0: // left
          if(lastdir != 2 && hole % CHUNKS_HORZ){ mover = hole - 1; } break;
      }
    }while(mover == chunkcount);
    lastdir = direction;
    int err = move_square(nc, chunks[mover], &holey, &holex, movens);
    if(err){
      return err;
    }
    chunks[hole] = chunks[mover];
    chunks[mover] = NULL;
    hole = mover;
  }
  return 0;
}

static int
fill_chunk(struct ncplane* n, int idx){
  const int hidx = idx % CHUNKS_HORZ;
  const int vidx = idx / CHUNKS_HORZ;
  unsigned maxy, maxx;
  ncplane_dim_yx(n, &maxy, &maxx);
  uint64_t channels = 0;
  int r = 64 + hidx * 10;
  int b = 64 + vidx * 30;
  int g = 225 - ((hidx + vidx) * 12);
  ncchannels_set_fg_rgb8(&channels, r, g, b);
  uint32_t ul = 0, ur = 0, ll = 0, lr = 0;
  ncchannel_set_rgb8(&ul, r, g, b);
  ncchannel_set_rgb8(&lr, r, g, b);
  ncchannel_set_rgb8(&ur, g, b, r);
  ncchannel_set_rgb8(&ll, b, r, g);
  int ret = 0;
  if(notcurses_canutf8(ncplane_notcurses(n))){
    if(ncplane_gradient2x1(n, -1, -1, 0, 0, ul, ur, ll, lr) <= 0){
      ret = -1;
    }
    ret |= ncplane_double_box(n, 0, channels, maxy - 1, maxx - 1, 0);
  }else{
    if(ncplane_gradient(n, -1, -1, 0, 0, " ", NCSTYLE_NONE, ul, ur, ll, lr) <= 0){
      ret = -1;
    }
    ret |= ncplane_ascii_box(n, 0, channels, maxy - 1, maxx - 1, 0);
  }
  if(maxx >= 4 && maxy >= 3){
    // don't zero-index to viewer
    ret |= (ncplane_printf_yx(n, (maxy - 1) / 2, (maxx - 1) / 2, "%02d", idx + 1) < 0);
  }
  return ret;
}

static int
draw_bounding_box(struct ncplane* n, int yoff, int xoff, int chunky, int chunkx){
  int ret;
  uint64_t channels = 0;
  ncchannels_set_fg_rgb8(&channels, 180, 80, 180);
  //channels_set_bg_rgb8(&channels, 0, 0, 0);
  ncplane_cursor_move_yx(n, yoff, xoff);
  if(notcurses_canutf8(ncplane_notcurses(n))){
    ret = ncplane_rounded_box(n, 0, channels,
                              CHUNKS_VERT * chunky + yoff + 1,
                              CHUNKS_HORZ * chunkx + xoff + 1, 0);
  }else{
    ret = ncplane_ascii_box(n, 0, channels,
                            CHUNKS_VERT * chunky + yoff + 1,
                            CHUNKS_HORZ * chunkx + xoff + 1, 0);
  }
  return ret;
}

// make a bunch of boxes with gradients and use them to play a sliding puzzle.
int sliders_demo(struct notcurses* nc, uint64_t startns){
  int ret = -1, z;
  unsigned maxx, maxy;
  struct ncplane* n = notcurses_stddim_yx(nc, &maxy, &maxx);
  int chunky, chunkx;
  // want at least 2x2 for each sliding chunk
  if(maxy < CHUNKS_VERT * 2 || maxx < CHUNKS_HORZ * 2){
    return -1;
  } 
  // we want an 8x8 grid of chunks with a border. the leftover space will be unused
  chunky = (maxy - 2) / CHUNKS_VERT;
  chunkx = (maxx - 2) / CHUNKS_HORZ;
  // want an even width so our 2-digit IDs are centered exactly
  chunkx -= (chunkx % 2);
  // don't allow them to be too rectangular, but keep aspect ratio in mind!
  if(chunky > chunkx + 1){
    chunky = chunkx + 1;
  }else if(chunkx > chunky * 2){
    chunkx = chunky * 2;
  }
  int wastey = ((maxy - 2) - (CHUNKS_VERT * chunky)) / 2;
  int wastex = ((maxx - 2) - (CHUNKS_HORZ * chunkx)) / 2;
  const int chunkcount = CHUNKS_VERT * CHUNKS_HORZ;
  struct ncplane** chunks = malloc(sizeof(*chunks) * chunkcount);
  if(chunks == NULL){
    return -1;
  }
  memset(chunks, 0, sizeof(*chunks) * chunkcount);
  // draw the 72 boxes in a nice color pattern, in order
  int cy, cx;
  for(cy = 0 ; cy < CHUNKS_VERT ; ++cy){
    for(cx = 0 ; cx < CHUNKS_HORZ ; ++cx){
      const int idx = cy * CHUNKS_HORZ + cx;
      struct ncplane_options nopts = {
        .y = cy * chunky + wastey + 1,
        .x = cx * chunkx + wastex + 1,
        .rows = chunky,
        .cols = chunkx,
      };
      chunks[idx] = ncplane_create(n, &nopts);
      if(chunks[idx] == NULL){
        goto done;
      }
      if(fill_chunk(chunks[idx], idx)){
        goto done;
      }
    }
  }
  // draw a box around the playing area
  if(draw_bounding_box(n, wastey, wastex, chunky, chunkx)){
    goto done;
  }
  DEMO_RENDER(nc);
  // shuffle up the chunks
  int i;
  demo_nanosleep(nc, &demodelay);
  for(i = 0 ; i < 200 ; ++i){
    int i0 = rand() % chunkcount;
    int i1 = rand() % chunkcount;
    while(i1 == i0){
      i1 = rand() % chunkcount;
    }
    int targy0, targx0;
    int targy, targx;
    ncplane_yx(chunks[i0], &targy0, &targx0);
    ncplane_yx(chunks[i1], &targy, &targx);
    struct ncplane* t = chunks[i0];
    ncplane_move_yx(t, targy, targx);
    chunks[i0] = chunks[i1];
    ncplane_move_yx(chunks[i0], targy0, targx0);
    chunks[i1] = t;
    DEMO_RENDER(nc);
  }
  ret = play(nc, chunks, startns);

done:
  for(z = 0 ; z < chunkcount ; ++z){
    ncplane_destroy(chunks[z]);
  }
  free(chunks);
  return ret;
}
