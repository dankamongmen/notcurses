#include <stdlib.h>
#include <string.h>
#include <notcurses.h>
#include "demo.h"

// FIXME do the bigger dimension on the screen's bigger dimension
#define CHUNKS_VERT 6
#define CHUNKS_HORZ 12
#define MOVES 45
#define GIG 1000000000

static int
move_square(struct notcurses* nc, struct ncplane* chunk, int* holey, int* holex){
  int newholex, newholey;
  ncplane_yx(chunk, &newholey, &newholex);
  // we need to move from newhole to hole over the course of movetime
  int deltay, deltax;
  deltay = *holey - newholey;
  deltax = *holex - newholex;
  // divide movetime into abs(max(deltay, deltax)) units, and move delta
  int units = abs(deltay) > abs(deltax) ? abs(deltay) : abs(deltax);
  uint64_t movens = (MOVES / 5) * (demodelay.tv_sec * GIG + demodelay.tv_nsec);
  movens /= units;
  struct timespec movetime = {
    .tv_sec = movens / MOVES / GIG,
    .tv_nsec = movens / MOVES % GIG,
  };
  int i;
  int targy = newholey;
  int targx = newholex;
  deltay = deltay < 0 ? -1 : deltay == 0 ? 0 : 1;
  deltax = deltax < 0 ? -1 : deltax == 0 ? 0 : 1;
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
  const int chunkcount = CHUNKS_VERT * CHUNKS_HORZ;
  // struct ncplane* n = notcurses_stdplane(nc);
  int hole = random() % chunkcount;
  int holex, holey;
  ncplane_yx(chunks[hole], &holey, &holex);
  ncplane_destroy(chunks[hole]);
  chunks[hole] = NULL;
  int m;
  int lastdir = -1;
  for(m = 0 ; m < MOVES ; ++m){
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
    move_square(nc, chunks[mover], &holey, &holex);
    chunks[hole] = chunks[mover];
    chunks[mover] = NULL;
    hole = mover;
  }
  return 0;
}

static int
fill_chunk(struct ncplane* n, int idx){
  char buf[4];
  int maxy, maxx;
  ncplane_dim_yx(n, &maxy, &maxx);
  snprintf(buf, sizeof(buf), "%03d", idx);
  cell ul = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
  cell ll = CELL_TRIVIAL_INITIALIZER, lr = CELL_TRIVIAL_INITIALIZER;
  cell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
  if(ncplane_double_box_cells(n, &ul, &ur, &ll, &lr, &hl, &vl)){
    return -1;
  }
  int r = random() % 256, g = random() % 256, b = random() % 256;
  cell_set_fg(&ul, r, g, b);
  cell_set_fg(&ur, r, g, b);
  cell_set_fg(&ll, r, g, b);
  cell_set_fg(&lr, r, g, b);
  cell_set_fg(&hl, r, g, b);
  cell_set_fg(&vl, r, g, b);
  if(ncplane_box(n, &ul, &ur, &ll, &lr, &hl, &vl, maxy - 1, maxx - 1)){
    return -1;
  }
  if(maxx >= 5 && maxy >= 3){
    if(ncplane_cursor_move_yx(n, (maxy - 1) / 2, (maxx - 3) / 2)){
      return -1;
    }
    ncplane_fg_rgb8(n, 255 - r, 255 - g, 255 - b);
    ncplane_bg_rgb8(n, r, g, b);
    if(ncplane_putstr(n, buf) <= 0){
      return -1;
    }
  }
  cell_release(n, &ul);
  cell_release(n, &ur);
  cell_release(n, &ll);
  cell_release(n, &lr);
  cell_release(n, &hl);
  cell_release(n, &vl);
  cell bg = CELL_TRIVIAL_INITIALIZER;
  cell_set_bg(&bg, r, g, b);
  ncplane_set_background(n, &bg);
  cell_release(n, &bg);
  return 0;
}

static int
draw_bounding_box(struct ncplane* n, int yoff, int xoff, int chunky, int chunkx){
  int ret = -1;
  cell ul = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
  cell ll = CELL_TRIVIAL_INITIALIZER, lr = CELL_TRIVIAL_INITIALIZER;
  cell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
  if(ncplane_rounded_box_cells(n, &ul, &ur, &ll, &lr, &hl, &vl)){
    return -1;
  }
  cell_set_fg(&ul, 180, 80, 180);
  cell_set_fg(&ur, 180, 80, 180);
  cell_set_fg(&ll, 180, 80, 180);
  cell_set_fg(&lr, 180, 80, 180);
  cell_set_fg(&hl, 180, 80, 180);
  cell_set_fg(&vl, 180, 80, 180);
  ncplane_cursor_move_yx(n, yoff, xoff);
  if(!ncplane_box(n, &ul, &ur, &ll, &lr, &hl, &vl,
                 CHUNKS_VERT * chunky + yoff + 1,
                 CHUNKS_HORZ * chunkx + xoff + 1)){
    ret = 0;
  }
  cell_release(n, &ul);
  cell_release(n, &ur);
  cell_release(n, &ll);
  cell_release(n, &lr);
  cell_release(n, &hl);
  cell_release(n, &vl);
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
  int wastey = ((maxy - 2) % CHUNKS_VERT) / 2;
  int wastex = ((maxx - 2) % CHUNKS_HORZ) / 2;
  struct ncplane* n = notcurses_stdplane(nc);
  ncplane_erase(n);
  const int chunkcount = CHUNKS_VERT * CHUNKS_HORZ;
  struct ncplane** chunks = malloc(sizeof(*chunks) * chunkcount);
  if(chunks == NULL){
    return -1;
  }
  memset(chunks, 0, sizeof(*chunks) * chunkcount);
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
  if(draw_bounding_box(n, wastey, wastex, chunky, chunkx)){
    return -1;
  }
  if(notcurses_render(nc)){
    goto done;
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
