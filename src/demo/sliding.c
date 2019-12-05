#include <stdlib.h>
#include <string.h>
#include <notcurses.h>
#include "demo.h"

// FIXME do the bigger dimension on the screen's bigger dimension
#define CHUNKS_VERT 6
#define CHUNKS_HORZ 12
#define MOVES 20
#define GIG 1000000000ul

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
  struct timespec movetime = {
    .tv_sec = (movens / MOVES) / GIG,
    .tv_nsec = (movens / MOVES) % GIG,
  };
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
    if(notcurses_render(nc)){
      return -1;
    }
    nanosleep(&movetime, NULL);
  }
  *holey = newholey;
  *holex = newholex;
  return 0;
}

// we take (MOVES / 5) * demodelay to play MOVES moves
static int
play(struct notcurses* nc, struct ncplane** chunks){
  const uint64_t delayns = demodelay.tv_sec * GIG + demodelay.tv_nsec;
  const int chunkcount = CHUNKS_VERT * CHUNKS_HORZ;
  struct timespec cur;
  clock_gettime(CLOCK_MONOTONIC, &cur);
  // we don't want to spend more than demodelay * 5
  const uint64_t totalns = delayns * 5;
  const uint64_t deadline_ns = cur.tv_sec * GIG + cur.tv_nsec + totalns;
  const uint64_t movens = totalns / MOVES;
  int hole = random() % chunkcount;
  int holex, holey;
  ncplane_yx(chunks[hole], &holey, &holex);
  ncplane_destroy(chunks[hole]);
  chunks[hole] = NULL;
  int m;
  int lastdir = -1;
  for(m = 0 ; m < MOVES ; ++m){
    clock_gettime(CLOCK_MONOTONIC, &cur);
    uint64_t now = cur.tv_sec * GIG + cur.tv_nsec;
    if(now >= deadline_ns){
      break;
    }
    int mover = chunkcount;
    int direction;
    do{
      direction = random() % 4;
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
    move_square(nc, chunks[mover], &holey, &holex, movens);
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
  char buf[4];
  int maxy, maxx;
  ncplane_dim_yx(n, &maxy, &maxx);
  snprintf(buf, sizeof(buf), "%02d", idx);
  uint64_t channels = 0;
  int r = 64 + hidx * 10;
  int b = 64 + vidx * 30;
  int g = 225 - ((hidx + vidx) * 12);
  notcurses_fg_prep(&channels, r, g, b);
  if(ncplane_double_box(n, 0, channels, maxy - 1, maxx - 1, 0)){
    return -1;
  }
  if(maxx >= 4 && maxy >= 3){
    if(ncplane_cursor_move_yx(n, (maxy - 1) / 2, (maxx - 2) / 2)){
      return -1;
    }
    ncplane_fg_rgb8(n, 0, 0, 0);
    ncplane_bg_rgb8(n, r, g, b);
    if(ncplane_putstr(n, buf) <= 0){
      return -1;
    }
  }
  cell style;
  cell_init(&style);
  cell_set_fg(&style, r, g, b);
  cell_prime(n, &style, "█", 0, channels);
  ncplane_set_background(n, &style);
  cell_release(n, &style);
  return 0;
}

static int
draw_bounding_box(struct ncplane* n, int yoff, int xoff, int chunky, int chunkx){
  int ret;
  uint64_t channels = 0;
  notcurses_fg_prep(&channels, 180, 80, 180);
  ncplane_cursor_move_yx(n, yoff, xoff);
  ret = ncplane_rounded_box(n, 0, channels,
                            CHUNKS_VERT * chunky + yoff + 1,
                            CHUNKS_HORZ * chunkx + xoff + 1, 0);
  return ret;
}

// break whatever's on the screen into panels and shift them around like a
// sliding puzzle. FIXME once we have copying, anyway. until then, just use
// background colors.
int sliding_puzzle_demo(struct notcurses* nc){
  int ret = -1, z;
  int maxx, maxy;
  int chunky, chunkx;
  notcurses_term_dim_yx(nc, &maxy, &maxx);
  // want at least 2x2 for each sliding chunk
  if(maxy < CHUNKS_VERT * 2 || maxx < CHUNKS_HORZ * 2){
    fprintf(stderr, "Terminal too small, need at least %dx%d\n",
            CHUNKS_HORZ, CHUNKS_VERT);
    return -1;
  } 
  // we want an 8x8 grid of chunks with a border. the leftover space will be unused
  chunky = (maxy - 2) / CHUNKS_VERT;
  chunkx = (maxx - 2) / CHUNKS_HORZ;
  // don't allow them to be too rectangular, but keep aspect ratio in mind!
  if(chunky > chunkx + 1){
    chunky = chunkx + 1;
  }else if(chunkx > chunky * 2){
    chunkx = chunky * 2;
  }
  int wastey = ((maxy - 2) - (CHUNKS_VERT * chunky)) / 2;
  int wastex = ((maxx - 2) - (CHUNKS_HORZ * chunkx)) / 2;
  struct ncplane* n = notcurses_stdplane(nc);
  ncplane_erase(n);
  int averr = 0;
  struct ncvisual* ncv = ncplane_visual_open(n, "../tests/changes.jpg", &averr);
  if(ncv == NULL){
    return -1;
  }
  if(ncvisual_decode(ncv, &averr) == NULL){
    ncvisual_destroy(ncv);
    return -1;
  }
  if(ncvisual_render(ncv)){
    ncvisual_destroy(ncv);
    return -1;
  }
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
      chunks[idx] =
        notcurses_newplane(nc, chunky, chunkx, cy * chunky + wastey + 1,
                           cx * chunkx + wastex + 1, NULL);
      if(chunks[idx] == NULL){
        goto done;
      }
      fill_chunk(chunks[idx], idx);
    }
  }
  // draw a box around the playing area
  if(draw_bounding_box(n, wastey, wastex, chunky, chunkx)){
    return -1;
  }
  if(notcurses_render(nc)){
    goto done;
  }
  struct timespec ts = { .tv_sec = 0, .tv_nsec = 1000000000, };
  // fade out each of the chunks in succession
  /*for(cy = 0 ; cy < CHUNKS_VERT ; ++cy){
    for(cx = 0 ; cx < CHUNKS_HORZ ; ++cx){
      const int idx = cy * CHUNKS_HORZ + cx;
      if(ncplane_fadeout(chunks[idx], &ts)){
        goto done;
      }
    }
  }*/
  // shuffle up the chunks
  int i;
  clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
  for(i = 0 ; i < 200 ; ++i){
    int i0 = random() % chunkcount;
    int i1 = random() % chunkcount;
    while(i1 == i0){
      i1 = random() % chunkcount;
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
    if(notcurses_render(nc)){
      goto done;
    }
  }
  if(play(nc, chunks)){
    goto done;
  }
  ret = 0;

done:
  for(z = 0 ; z < chunkcount ; ++z){
    ncplane_destroy(chunks[z]);
  }
  free(chunks);
  return ret;
}
