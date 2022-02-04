#include <math.h>
#include "internal.h"
#include "fbuf.h"

#define RGBSIZE 3

// this palette entry is a sentinel for a transparent pixel (and thus caps
// the palette at 65535 other entries).
#define TRANS_PALETTE_ENTRY 65535

// bytes per element in the auxiliary vector
#define AUXVECELEMSIZE 2

// returns the number of individual sixels necessary to represent the specified
// pixel geometry. these might encompass more pixel rows than |dimy| would
// suggest, up to the next multiple of 6 (i.e. a single row becomes a 6-row
// bitmap; as do two, three, four, five, or six rows). input is scaled geometry.
static inline int
sixelcount(int dimy, int dimx){
  return (dimy + 5) / 6 * dimx;
}

// returns the number of sixel bands (horizontal series of sixels, aka 6 rows)
// for |dimy| source rows. sixels are encoded as a series of sixel bands.
static inline int
sixelbandcount(int dimy){
  return sixelcount(dimy, 1);
}

// we set P2 based on whether there is any transparency in the sixel. if not,
// use SIXEL_P2_ALLOPAQUE (0), for faster drawing in certain terminals.
typedef enum {
  SIXEL_P2_ALLOPAQUE = 0,
  SIXEL_P2_TRANS = 1,
} sixel_p2_e;

// data for a single sixelband. a vector of sixel rows, one for each color
// represented within the band. we initially create a vector for every
// possible (quantized) color, and then collapse it.
typedef struct sixelband {
  int size;     // capacity FIXME if same for all, eliminate this
  char** vecs;  // array of vectors, many of which can be NULL
} sixelband;

// across the life of the sixel, we'll need to wipe and restore cells, without
// recourse to the original RGBA data. this is prohibitively expensive to do on
// the encoded data, since it might require expanding or collapsing sections in
// the middle (we could use a rope, but it would be annoying). instead, we keep
// for each sixelrow (i.e. for every 6 rows) a vector of colors and distinct
// encoded sections (i.e. *not* from some common long single allocation). this
// way, the encoded sections can be easily and cheaply changed (since they're
// small, and quickly indexed by sixelrow * color)). whenever we want to emit
// the sixel, we just gather all these dynamic sections and write them
// successively into the fbuf. this table can be built up in parallel, since
// it's isolated among sixelrows -- the sixelrow is then the natural work unit.
// this sixelmap is kept across the life of the sprixel; any longlived state
// must be here, whereas state necessary only for rendering ought be in qstate.
typedef struct sixelmap {
  int colors;
  int sixelbands;
  sixelband* bands;      // |sixelbands| collections of sixel vectors
  sixel_p2_e p2;         // set to SIXEL_P2_TRANS if we have transparent pixels
} sixelmap;

// second pass: construct data for extracted colors over the sixels. the
// map will be persisted in the sprixel; the remainder is lost.
// FIXME kill this off; use sixelmap directly
typedef struct sixeltable {
  sixelmap* map;        // copy of palette indices / transparency bits
} sixeltable;

// whip up a sixelmap sans data for the specified pixel geometry and color
// register count.
static sixelmap*
sixelmap_create(int dimy){
  sixelmap* ret = malloc(sizeof(*ret));
  if(ret){
    ret->p2 = SIXEL_P2_ALLOPAQUE;
    // they'll be initialized by their workers, possibly in parallel
    ret->sixelbands = sixelbandcount(dimy);
    ret->bands = malloc(sizeof(*ret->bands) * ret->sixelbands);
    if(ret->bands == NULL){
      free(ret);
      return NULL;
    }
    for(int i = 0 ; i < ret->sixelbands ; ++i){
      ret->bands[i].size = 0;
    }
    ret->colors = 0;
  }
  return ret;
}

static inline void
sixelband_free(sixelband* s){
  for(int j = 0 ; j < s->size ; ++j){
    free(s->vecs[j]);
  }
  free(s->vecs);
}

void sixelmap_free(sixelmap *s){
  if(s){
    for(int i = 0 ; i < s->sixelbands ; ++i){
      sixelband_free(&s->bands[i]);
    }
    free(s->bands);
    free(s);
  }
}

// three scaled sixel [0..100x3] components plus a population count.
typedef struct qsample {
  unsigned char comps[RGBSIZE];
  uint32_t pop;
} qsample;

// lowest samples for each node. first-order nodes track 1000 points in
// sixelspace (10x10x10). there are eight possible second-order nodes from a
// fractured first-order node, covering 125 points each (5x5x5).
typedef struct qnode {
  qsample q;
  // cidx plays two roles. during merge, we select the active set, and extract
  // them (since they'll be sorted, we can't operate directly on the octree).
  // here, we use cidx to map back to the initial octree entry, as we need
  // update them (from the active set) at the end of merging. afterwards, the
  // high bit indicates that it was chosen, and the cidx is a valid index into
  // the final color table. it is otherwise a link to the merged qnode.
  // during initial filtering, qlink determines whether a node has fractured:
  // if qlink is non-zero, it is a one-biased index to an onode.
  // FIXME combine these once more, but for now to keep it easy, we have two.
  // qlink links back into the octree.
  uint16_t qlink;
  uint16_t cidx;
} qnode;

// an octree-style node, used for fractured first-order nodes. the first
// bit is whether we're on the top or bottom of the R, then G, then B.
typedef struct onode {
  qnode* q[8];
} onode;

// convert rgb [0..255] to sixel [0..99]
static inline unsigned
ss(unsigned c){
  unsigned r = round(c * 100.0 / 255); // use real [0..100] scaling
  return r > 99 ? 99: r;
}

// get the keys for an rgb point. the returned value is on [0..999], and maps
// to a static qnode. the second value is on [0..7], and maps within the
// fractured onode (if necessary).
static inline unsigned
qnode_keys(unsigned r, unsigned g, unsigned b, unsigned *skey){
  unsigned ssr = ss(r);
  unsigned ssg = ss(g);
  unsigned ssb = ss(b);
  unsigned ret = ssr / 10 * 100 + ssg / 10 * 10 + ssb / 10;
  *skey = (((ssr % 10) / 5) << 2u) +
          (((ssg % 10) / 5) << 1u) +
          ((ssb % 10) / 5);
//fprintf(stderr, "0x%02x 0x%02x 0x%02x %02u %02u %02u %u %u\n", r, g, b, ssr, ssg, ssb, ret, *skey);
  return ret;
}

// have we been chosen for the color table?
static inline bool
chosen_p(const qnode* q){
  return q->cidx & 0x8000u;
}

static inline unsigned
make_chosen(unsigned cidx){
  return cidx | 0x8000u;
}

// get the cidx without the chosen bit
static inline unsigned
qidx(const qnode* q){
  return q->cidx & ~0x8000u;
}

typedef struct qstate {
  // we always work in terms of quantized colors (quantization is the first
  // step of rendering), using indexes into the derived palette. the actual
  // palette need only be stored during the initial render, since the sixel
  // header can be preserved, and the palette is unchanged by wipes/restores.
  unsigned char* table;  // |colors| x RGBSIZE components
  qnode* qnodes;
  onode* onodes;
  unsigned dynnodes_free;
  unsigned dynnodes_total;
  unsigned onodes_free;
  unsigned onodes_total;
  const struct blitterargs* bargs;
  const uint32_t* data;
  int linesize;
  sixelmap* smap;
  // these are the leny and lenx passed to sixel_blit(), which are likely
  // different from those reachable through bargs->len{y,x}!
  int leny, lenx;
} qstate;

#define QNODECOUNT 1000

// create+zorch an array of QNODECOUNT qnodes. this is 1000 entries covering
// 1000 sixel colors each (we pretend 100 doesn't exist, working on [0..99],
// heh). in addition, at the end we allocate |colorregs| qnodes, to be used
// dynamically in "fracturing". the original QNODECOUNT nodes are a static
// octree, flattened into an array; the latter are used as an actual octree.
// we must have 8 dynnodes available for every onode we create, or we can run
// into a situation where we don't have an available dynnode
// (see insert_color()).
static int
alloc_qstate(unsigned colorregs, qstate* qs){
  qs->dynnodes_free = colorregs;
  qs->dynnodes_total = qs->dynnodes_free;
  if((qs->qnodes = malloc((QNODECOUNT + qs->dynnodes_total) * sizeof(qnode))) == NULL){
    return -1;
  }
  qs->onodes_free = qs->dynnodes_total / 8;
  qs->onodes_total = qs->onodes_free;
  if((qs->onodes = malloc(qs->onodes_total * sizeof(*qs->onodes))) == NULL){
    free(qs->qnodes);
    return -1;
  }
  // don't technically need to clear the components, as we could
  // check the pop, but it's hidden under the compulsory cache misses.
  // we only initialize the static nodes, not the dynamic ones--we know
  // when we pull a dynamic one that it needs its popcount initialized.
  memset(qs->qnodes, 0, sizeof(qnode) * QNODECOUNT);
  qs->table = NULL;
  return 0;
}

// free internals of qstate object
static void
free_qstate(qstate *qs){
  if(qs){
    loginfo("freeing qstate");
    free(qs->qnodes);
    free(qs->onodes);
    free(qs->table);
  }
}

// insert a color from the source image into the octree.
static inline int
insert_color(qstate* qs, uint32_t pixel){
  const unsigned r = ncpixel_r(pixel);
  const unsigned g = ncpixel_g(pixel);
  const unsigned b = ncpixel_b(pixel);
  unsigned skey;
  const unsigned key = qnode_keys(r, g, b, &skey);
  assert(key < QNODECOUNT);
  assert(skey < 8);
  qnode* q = &qs->qnodes[key];
  if(q->q.pop == 0 && q->qlink == 0){ // previously-unused node
    q->q.comps[0] = r;
    q->q.comps[1] = g;
    q->q.comps[2] = b;
    q->q.pop = 1;
    ++qs->smap->colors;
    return 0;
  }
  onode* o;
  // it's not a fractured node, but it's been used. check to see if we
  // match the secondary key of what's here.
  if(q->qlink == 0){
    unsigned skeynat;
    qnode_keys(q->q.comps[0], q->q.comps[1], q->q.comps[2], &skeynat);
    if(skey == skeynat){
      ++q->q.pop; // pretty good match
      return 0;
    }
    // we want to fracture. if we have no onodes, though, we can't.
    // we also need at least one dynamic qnode. note that this means we might
    // open an onode just to fail to insert our current lookup; that's fine;
    // it's a symmetry between creation and extension.
    if(qs->dynnodes_free == 0 || qs->onodes_free == 0){
//fprintf(stderr, "NO FREE ONES %u\n", key);
      ++q->q.pop; // not a great match, but we're already scattered
      return 0;
    }
    // get the next free onode and zorch it out
    o = qs->onodes + qs->onodes_total - qs->onodes_free;
//fprintf(stderr, "o: %p obase: %p %u\n", o, qs->onodes, qs->onodes_total - qs->onodes_free);
    memset(o, 0, sizeof(*o));
    // get the next free dynnode and assign it to o, account for dnode
    o->q[skeynat] = &qs->qnodes[QNODECOUNT + qs->dynnodes_total - qs->dynnodes_free];
    --qs->dynnodes_free;
    // copy over our own details
    memcpy(o->q[skeynat], q, sizeof(*q));
    // set qlink to one-biased index of the onode, and account for onode
    q->qlink = qs->onodes_total - qs->onodes_free + 1;
    --qs->onodes_free;
    // reset our own population count
    q->q.pop = 0;
  }else{
    // the node has already been fractured
    o = qs->onodes + (q->qlink - 1);
  }
  if(o->q[skey]){
    // our subnode is already present, huzzah. increase its popcount.
    ++o->q[skey]->q.pop;
    return 0;
  }
  // we try otherwise to insert ourselves into o. this requires a free dynnode.
  if(qs->dynnodes_free == 0){
//fprintf(stderr, "NO DYNFREE %u\n", key);
    // this should never happen, because we always ought have 8 dynnodes for
    // every possible onode.
    return -1;
  }
  // get the next free dynnode and assign it to o, account for dnode
  o->q[skey] = &qs->qnodes[QNODECOUNT + qs->dynnodes_total - qs->dynnodes_free];
  --qs->dynnodes_free;
  o->q[skey]->q.pop = 1;
  o->q[skey]->q.comps[0] = r;
  o->q[skey]->q.comps[1] = g;
  o->q[skey]->q.comps[2] = b;
  o->q[skey]->qlink = 0;
  o->q[skey]->cidx = 0;
  ++qs->smap->colors;
//fprintf(stderr, "INSERTED[%u]: %u %u %u\n", key, q->q.comps[0], q->q.comps[1], q->q.comps[2]);
  return 0;
}

// resolve the input color to a color table index following any postprocessing
// of the octree.
static inline int
find_color(const qstate* qs, uint32_t pixel){
  const unsigned r = ncpixel_r(pixel);
  const unsigned g = ncpixel_g(pixel);
  const unsigned b = ncpixel_b(pixel);
  unsigned skey;
  const unsigned key = qnode_keys(r, g, b, &skey);
  const qnode* q = &qs->qnodes[key];
  if(q->qlink && q->q.pop == 0){
    if(qs->onodes[q->qlink - 1].q[skey]){
      q = qs->onodes[q->qlink - 1].q[skey];
    }else{
      logpanic("internal error: no color for 0x%016x", pixel);
      return -1;
    }
  }
  return qidx(q);
}

// create an auxiliary vector suitable for a Sixel sprixcell, and zero it out.
// there are two bytes per pixel in the cell: a palette index of up to 65534,
// or 65535 to indicate transparency.
static inline uint8_t*
sixel_auxiliary_vector(const sprixel* s){
  int pixels = ncplane_pile(s->n)->cellpxy * ncplane_pile(s->n)->cellpxx;
  size_t slen = pixels * AUXVECELEMSIZE;
  uint8_t* ret = malloc(slen);
  if(ret){
    memset(ret, 0, sizeof(slen));
  }
  return ret;
}

// the P2 parameter on a sixel specifies how unspecified pixels are drawn.
// if P2 is 1, unspecified pixels are transparent. otherwise, they're drawn
// as something else. some terminals (e.g. foot) can draw more quickly if
// P2 is 0, so we set that when we have no transparent pixels -- i.e. when
// all TAM entries are 0. P2 is at a fixed location in the sixel header.
// obviously, the sixel must already exist.
static inline void
change_p2(char* sixel, sixel_p2_e value){
  sixel[4] = value + '0';
}

static inline void
write_rle(char* vec, int* voff, int rle, int rep){
  if(rle > 2){
    *voff += sprintf(vec + *voff, "!%d", rle);
  }else if(rle == 2){
    vec[(*voff)++] = rep;
  }
  if(rle){
    vec[(*voff)++] = rep;
  }
  vec[*voff] = '\0';
}

// one for each color in the band we're building. |rle| tracks the number of
// consecutive unwritten instances of the current non-0 rep, which is itself
// tracked in |rep|. |wrote| tracks the number of sixels written out for this
// color. whenever we get a new rep (this only happens for non-zero reps),
// we must write any old rle rep, plus any zero-reps since then.
struct band_extender {
  int length; // current length of the vector
  int rle;    // current rep count of non-zero sixel for this color
  int wrote;  // number of sixels we've written out
  int rep;    // representation, 0..63
};

// add the supplied rle section to the appropriate vector, which might
// need to be created. we are writing out [bes->wrote, curx) (i.e. curx
// ought *not* describe the |bes| element, and ought equal |dimx| when
// finalizing the band). caller must update bes->wrote afterwards!
static inline char*
sixelband_extend(char* vec, struct band_extender* bes, int dimx, int curx){
  assert(dimx >= bes->rle);
  assert(0 <= bes->rle);
  assert(0 <= bes->rep);
  assert(64 > bes->rep);
  if(vec == NULL){
    // FIXME for now we make it as big as it could possibly need to be. ps,
    // don't try to just base it off how far in we are; wipe/restore could
    // change that!
    if((vec = malloc(dimx + 1)) == NULL){
      return NULL;
    }
  }
  // rle will equal 0 if this is our first non-zero rep, at a non-zero x;
  // in that case, rep is guaranteed to be 0; catch it at the bottom.
  write_rle(vec, &bes->length, bes->rle, bes->rep + 63);
  int clearlen = curx - (bes->rle + bes->wrote);
  write_rle(vec, &bes->length, clearlen, '?');
  return vec;
}

// get the index into the auxvec (2 bytes per pixel) given the true y/x pixel
// coordinates, plus the origin+dimensions of the relevant cell.
static inline int
auxvec_idx(int y, int x, int sy, int sx, int cellpxy, int cellpxx){
  if(y >= sy + cellpxy || y < sy){
    logpanic("illegal y for %d cell at %d: %d", cellpxy, sy, y);
    return -1;
  }
  if(x >= sx + cellpxx || x < sx){
    logpanic("illegal x for %d cell at %d: %d", cellpxx, sx, x);
    return -1;
  }
  const int xoff = x - sx;
  const int yoff = y - sy;
  const int off = yoff * cellpxx + xoff;
  return AUXVECELEMSIZE * off;
}

// the sixel |rep| is being wiped. the active pixels need be written to the
// |auxvec|, which is (|ey| - |sy| + 1) rows of (|ex| - |sx| + 1) columns.
// we are wiping the sixel |rep|, changing it to |mask|.
static inline void
write_auxvec(uint8_t* auxvec, int color, int x, int len, int sx, int ex,
             int sy, int ey, char rep, char mask){
}

// wipe the color within this band from startx to endx - 1, from starty to
// endy - 1 (0-offset in the band, a cell-sized region), writing out the
// auxvec. mask is the allowable sixel, y-wise. returns a positive number if
// pixels were wiped.
static inline int
wipe_color(sixelband* b, int color, int startx, int endx,
           int starty, int endy, char mask, int dimx, uint8_t* auxvec){
  const char* vec = b->vecs[color];
  if(vec == NULL){
    return 0; // no work to be done here
  }
  int wiped = 0;
  char* newvec = malloc(dimx);
  if(newvec == NULL){
    return -1;
  }
//fprintf(stderr, "color: %d Y: %d-%d X: %d-%d\n", color, starty, endy, startx, endx);
//fprintf(stderr, "s/e: %d/%d mask: %02x WIPE: [%s]\n", starty, endy, mask, vec);
  // we decode the color within the sixelband, and rebuild it without the
  // wiped pixels.
  int rle = 0; // the repetition number for this element
  // the x coordinate through which we've checked this band. if x + rle is
  // less than startx, this element cannot be affected by the wipe.
  // otherwise, starting at startx, it can be affected. once x >= endx, we
  // are done, and can copy any remaining elements blindly.
  int x = 0;
  int voff = 0;
  while(*vec){
    if(isdigit(*vec)){
      rle *= 10;
      rle += (*vec - '0');
    }else if(*vec == '!'){
      rle = 0;
    }else{
      if(rle == 0){
        rle = 1;
      }
      char rep = *vec;
      char masked = ((rep - 63) & mask) + 63;
//fprintf(stderr, "X/RLE/ENDX: %d %d %d\n", x, rle, endx);
      if(x + rle < startx){ // not wiped material; reproduce as-is
        write_rle(newvec, &voff, rle, rep);
        x += rle;
      }else if(masked == rep){ // not changed by wipe; reproduce as-is
        write_rle(newvec, &voff, rle, rep);
        x += rle;
      }else{ // changed by wipe; might have to break it up
        wiped = 1;
        if(x < startx){
          write_rle(newvec, &voff, startx - x, rep);
          rle -= startx - x;
          x = startx;
        }
        if(x + rle >= endx){
          // FIXME this new rep might equal the next rep, in which case we ought combine
          write_rle(newvec, &voff, endx - x, masked);
          write_auxvec(auxvec, color, x, endx - x, startx, endx, starty, endy, rep, mask);
          rle -= endx - x;
          x = endx;
        }else{
          write_rle(newvec, &voff, rle, masked);
          write_auxvec(auxvec, color, x, rle, startx, endx, starty, endy, rep, mask);
          x += rle;
          rle = 0;
        }
        if(rle){
          write_rle(newvec, &voff, rle, rep);
          x += rle;
        }
      }
      rle = 0;
    }
    ++vec;
    if(x >= endx){
      strcpy(newvec + voff, vec); // there is always room
      break;
    }
  }
//if(strcmp(newvec, b->vecs[color])) fprintf(stderr, "WIPED %d y [%d..%d) x [%d..%d) mask: %d [%s]\n", color, starty, endy, startx, endx, mask, newvec);
  free(b->vecs[color]);
  if(voff == 0){
    // FIXME check for other null vectors; free such, and assign NULL
    free(newvec);
    newvec = NULL;
  }
  b->vecs[color] = newvec;
  return wiped;
}

// wipe the band from startx to endx - 1, from starty to endy - 1. returns the
// number of pixels actually wiped.
static inline int
wipe_band(sixelmap* smap, int band, int startx, int endx,
          int starty, int endy, int dimx, int cellpixy, int cellpixx,
          uint8_t* auxvec){
//fprintf(stderr, "******************** BAND %d ********************8\n", band);
  int wiped = 0;
  // get 0-offset start and end row bounds for our band.
  const int sy = band * 6 < starty ? starty - band * 6 : 0;
  const int ey = (band + 1) * 6 > endy ? 6 - ((band + 1) * 6 - endy) : 6;
  // we've got a mask that we'll AND with the decoded sixels; set it to
  // 0 wherever we're wiping.
  unsigned char mask = 63;
  // knock out a bit for each row we're wiping within the band
  for(int i = 0 ; i < 6 ; ++i){
    if(i >= sy && i <= ey){
      mask &= ~(1u << i);
    }
  }
  sixelband* b = &smap->bands[band];
  // offset into map->data where our color starts
  for(int i = 0 ; i < b->size ; ++i){
    wiped += wipe_color(b, i, startx, endx, starty, endy, mask, dimx, auxvec);
  }
  return wiped;
}

// we return -1 because we're not doing a proper wipe -- that's not possible
// using sixel. we just mark it as partially transparent, so that if it's
// redrawn, it's redrawn using P2=1.
int sixel_wipe(sprixel* s, int ycell, int xcell){
//fprintf(stderr, "WIPING %d/%d\n", ycell, xcell);
  uint8_t* auxvec = sixel_auxiliary_vector(s);
  if(auxvec == NULL){
    return -1;
  }
  const int cellpxy = ncplane_pile(s->n)->cellpxy;
  const int cellpxx = ncplane_pile(s->n)->cellpxx;
  memset(auxvec + cellpxx * cellpxy, 0xff, cellpxx * cellpxy);
  sixelmap* smap = s->smap;
  const int startx = xcell * cellpxx;
  const int starty = ycell * cellpxy;
  int endx = ((xcell + 1) * cellpxx);
  if(endx >= s->pixx){
    endx = s->pixx;
  }
  int endy = ((ycell + 1) * cellpxy);
  if(endy >= s->pixy){
    endy = s->pixy;
  }
  const int startband = starty / 6;
  const int endband = (endy - 1) / 6;
//fprintf(stderr, "y/x: %d/%d bands: %d-%d start: %d/%d end: %d/%d\n", ycell, xcell, startband, endband - 1, starty, startx, endy, endx);
  // walk through each color, and wipe the necessary sixels from each band
  int w = 0;
  for(int b = startband ; b <= endband ; ++b){
    w += wipe_band(smap, b, startx, endx, starty, endy, s->pixx,
                   cellpxy, cellpxx, auxvec);
  }
  if(w){
    s->wipes_outstanding = true;
  }
  change_p2(s->glyph.buf, SIXEL_P2_TRANS);
  assert(NULL == s->n->tam[s->dimx * ycell + xcell].auxvector);
  s->n->tam[s->dimx * ycell + xcell].auxvector = auxvec;
  // FIXME this invalidation ought not be necessary, since we're simply
  // wiping, and thus a glyph is going to be printed over whatever we've
  // just destroyed. in alacritty, however, this isn't sufficient to knock
  // out a graphic; we need repaint with the transparency.
  // see https://github.com/dankamongmen/notcurses/issues/2142
  int absx, absy;
  ncplane_abs_yx(s->n, &absy, &absx);
  sprixel_invalidate(s, absy, absx);
  return 0;
}

// rebuilds the auxiliary vectors, and scrubs the actual pixels, following
// extraction of the palette. doing so allows the new frame's pixels to
// contribute to the solved palette, even if they were wiped in the previous
// frame. pixels ought thus have been set up in sixel_blit(), despite TAM
// entries in the ANNIHILATED state.
static int
scrub_color_table(sprixel* s){
  if(s->n && s->n->tam){
    // we use the sprixel cell geometry rather than the plane's because this
    // is called during our initial blit, before we've resized the plane.
    for(unsigned y = 0 ; y < s->dimy ; ++y){
      for(unsigned x = 0 ; x < s->dimx ; ++x){
        unsigned txyidx = y * s->dimx + x;
        sprixcell_e state = s->n->tam[txyidx].state;
        if(state == SPRIXCELL_ANNIHILATED || state == SPRIXCELL_ANNIHILATED_TRANS){
//fprintf(stderr, "POSTEXTRACT WIPE %d/%d\n", y, x);
          sixel_wipe(s, y, x);
        }
      }
    }
  }
  return 0;
}

// goes through the needs_refresh matrix, and damages cells needing refreshing.
void sixel_refresh(const ncpile* p, sprixel* s){
  if(s->needs_refresh == NULL){
    return;
  }
  int absy, absx;
  ncplane_abs_yx(s->n, &absy, &absx);
  for(unsigned y = 0 ; y < s->dimy ; ++y){
    const unsigned yy = absy + y;
    for(unsigned x = 0 ; x < s->dimx ; ++x){
      unsigned idx = y * s->dimx + x;
      if(s->needs_refresh[idx]){
        const unsigned xx = absx + x;
        if(xx < p->dimx && yy < p->dimy){
          unsigned ridx = yy * p->dimx + xx;
          struct crender *r = &p->crender[ridx];
          r->s.damaged = 1;
        }
      }
    }
  }
  free(s->needs_refresh);
  s->needs_refresh = NULL;
}

// when we first cross into a new cell, we check its old state, and if it
// was transparent, set the rmatrix low. otherwise, set it high. this should
// only be called for the first pixel in each cell.
static inline void
update_rmatrix(unsigned char* rmatrix, int txyidx, const tament* tam){
  if(rmatrix == NULL){
    return;
  }
  sprixcell_e state = tam[txyidx].state;
  if(state == SPRIXCELL_TRANSPARENT || state > SPRIXCELL_ANNIHILATED){
    rmatrix[txyidx] = 0;
  }else{
    rmatrix[txyidx] = 1;
  }
}

static int
qnodecmp(const void* q0, const void* q1){
  const qnode* qa = q0;
  const qnode* qb = q1;
  return qa->q.pop < qb->q.pop ? -1 : qa->q.pop == qb->q.pop ? 0 : 1;
}

// from the initial set of QNODECOUNT qnodes, extract the number of active
// ones -- our initial (reduced) color count -- and sort. heap allocation.
// precondition: colors > 0
static qnode*
get_active_set(qstate* qs, uint32_t colors){
  qnode* act = malloc(sizeof(*act) * colors);
  unsigned targidx = 0;
  // filter the initial qnodes for pop != 0
  unsigned total = QNODECOUNT + (qs->dynnodes_total - qs->dynnodes_free);
//fprintf(stderr, "TOTAL IS %u WITH %u COLORS\n", total, colors);
  for(unsigned z = 0 ; z < total && targidx < colors ; ++z){
//fprintf(stderr, "EXTRACT? [%04u] pop %u\n", z, qs->qnodes[z].q.pop);
    if(qs->qnodes[z].q.pop){
      memcpy(&act[targidx], &qs->qnodes[z], sizeof(*act));
      // link it back to the original node's position in the octree
//fprintf(stderr, "LINKING %u to %u\n", targidx, z);
      act[targidx].qlink = z;
      ++targidx;
    }else if(qs->qnodes[z].qlink){
      const struct onode* o = &qs->onodes[qs->qnodes[z].qlink - 1];
      // FIXME i don't think we need the second conditional? in a perfect world?
      for(unsigned s = 0 ; s < 8 && targidx < colors ; ++s){
//fprintf(stderr, "o: %p qlink: %u\n", o, qs->qnodes[z].qlink - 1);
        if(o->q[s]){
          memcpy(&act[targidx], o->q[s], sizeof(*act));
//fprintf(stderr, "O-LINKING %u to %ld[%u]\n", targidx, o->q[s] - qs->qnodes, s);
          act[targidx].qlink = o->q[s] - qs->qnodes;
          ++targidx;
        }
      }
    }
  }
//fprintf(stderr, "targidx: %u colors: %u\n", targidx, colors);
  assert(targidx == colors);
  qsort(act, colors, sizeof(*act), qnodecmp);
  return act;
}

static inline int
find_next_lowest_chosen(const qstate* qs, int z, int i, const qnode** hq){
//fprintf(stderr, "FIRST CHOSEN: %u %d\n", z, i);
  do{
    const qnode* h = &qs->qnodes[z];
//fprintf(stderr, "LOOKING AT %u POP %u QLINK %u CIDX %u\n", z, h->q.pop, h->qlink, h->cidx);
    if(h->q.pop == 0 && h->qlink){
      const onode* o = &qs->onodes[h->qlink - 1];
      while(i >= 0){
        h = o->q[i];
        if(h && chosen_p(h)){
          *hq = h;
//fprintf(stderr, "NEW HQ: %p RET: %u\n", *hq, z * 8 + i);
          return z * 8 + i;
        }
        if(++i == 8){
          break;
        }
      }
    }else{
      if(chosen_p(h)){
        *hq = h;
//fprintf(stderr, "NEW HQ: %p RET: %u\n", *hq, z * 8);
        return z * 8;
      }
    }
    ++z;
    i = 0;
  }while(z < QNODECOUNT);
//fprintf(stderr, "RETURNING -1\n");
  return -1;
}

static inline void
choose(qstate* qs, qnode* q, int z, int i, int* hi, int* lo,
       const qnode** hq, const qnode** lq){
  if(!chosen_p(q)){
//fprintf(stderr, "NOT CHOSEN: %u %u %u %u\n", z, qs->qnodes[z].qlink, qs->qnodes[z].q.pop, qs->qnodes[z].cidx);
    if(z * 8 > *hi){
      *hi = find_next_lowest_chosen(qs, z, i, hq);
    }
    int cur = z * 8 + (i >= 0 ? i : 4);
    if(*lo == -1){
      q->cidx = qidx(*hq);
    }else if(*hi == -1 || cur - *lo < *hi - cur){
      q->cidx = qidx(*lq);
    }else{
      q->cidx = qidx(*hq);
    }
  }else{
    *lq = q;
    *lo = z * 8;
  }
}

// we must reduce the number of colors until we're using less than or equal
// to the number of color registers.
static inline int
merge_color_table(qstate* qs){
  if(qs->smap->colors == 0){
    return 0;
  }
  qnode* qactive = get_active_set(qs, qs->smap->colors);
  if(qactive == NULL){
    return -1;
  }
  // assign color table entries to the most popular colors. use the lowest
  // color table entries for the most popular ones, as they're the shortest
  // (this is not necessarily an optimizing huristic, but it'll do for now).
  int cidx = 0;
//fprintf(stderr, "colors: %u cregs: %u\n", qs->colors, colorregs);
  for(int z = qs->smap->colors - 1 ; z >= 0 ; --z){
    if(qs->smap->colors >= qs->bargs->u.pixel.colorregs){
      if(cidx == qs->bargs->u.pixel.colorregs){
        break; // we just ran out of color registers
      }
    }
    qs->qnodes[qactive[z].qlink].cidx = make_chosen(cidx);
    ++cidx;
  }
  free(qactive);
  if(qs->smap->colors > qs->bargs->u.pixel.colorregs){
    // tend to those which couldn't get a color table entry. we start with two
    // values, lo and hi, initialized to -1. we iterate over the *static* qnodes,
    // descending into onodes to check their qnodes. we thus iterate over all
    // used qnodes, in order (and also unused static qnodes). if the node is
    // empty, continue. if it is chosen, replace lo. otherwise, if hi is less
    // than z, we need find the next lowest chosen one. if there is no next
    // lowest, hi is reset to -1. otherwise, set hi. once we have the new hi > z,
    // determine which of hi and lo are closer to z, discounting -1 values, and
    // link te closer one to z. a toplevel node is worth 8 in terms of distance;
    // and lowlevel node is worth 1.
    int lo = -1;
    int hi = -1;
    const qnode* lq = NULL;
    const qnode* hq = NULL;
    for(int z = 0 ; z < QNODECOUNT ; ++z){
      if(qs->qnodes[z].q.pop == 0){
        if(qs->qnodes[z].qlink == 0){
          continue; // unused
        }
        // process the onode
        const onode* o = &qs->onodes[qs->qnodes[z].qlink - 1];
        for(int i = 0 ; i < 8 ; ++i){
          if(o->q[i]){
            choose(qs, o->q[i], z, i, &hi, &lo, &hq, &lq);
          }
        }
      }else{
        choose(qs, &qs->qnodes[z], z, -1, &hi, &lo, &hq, &lq);
      }
    }
    qs->smap->colors = qs->bargs->u.pixel.colorregs;
  }
  return 0;
}

static inline void
load_color_table(const qstate* qs){
  int loaded = 0;
  int total = QNODECOUNT + (qs->dynnodes_total - qs->dynnodes_free);
  for(int z = 0 ; z < total && loaded < qs->smap->colors ; ++z){
    const qnode* q = &qs->qnodes[z];
    if(chosen_p(q)){
      qs->table[RGBSIZE * qidx(q) + 0] = ss(q->q.comps[0]);
      qs->table[RGBSIZE * qidx(q) + 1] = ss(q->q.comps[1]);
      qs->table[RGBSIZE * qidx(q) + 2] = ss(q->q.comps[2]);
      ++loaded;
    }
  }
//fprintf(stderr, "loaded: %u colors: %u\n", loaded, qs->colors);
  assert(loaded == qs->smap->colors);
}

// build up a sixel band from (up to) 6 rows of the source RGBA.
static inline int
build_sixel_band(qstate* qs, int bnum){
  sixelband* b = &qs->smap->bands[bnum];
  b->size = qs->smap->colors;
  size_t bsize = sizeof(*b->vecs) * b->size;
  size_t mlen = qs->smap->colors * sizeof(struct band_extender);
  struct band_extender* meta = malloc(mlen);
  if(meta == NULL){
    return -1;
  }
  b->vecs = malloc(bsize);
  if(b->vecs == NULL){
    free(meta);
    return -1;
  }
  memset(b->vecs, 0, bsize);
  memset(meta, 0, mlen);
  const int ystart = qs->bargs->begy + bnum * 6;
  const int endy = (bnum + 1 == qs->smap->sixelbands ?
                                 qs->leny - qs->bargs->begy : ystart + 6);
  struct {
    int color; // 0..colormax
    int rep;   // non-zero representation, 1..63
  } active[6];
  // we're going to advance horizontally through the sixelband
  int x;
  // FIXME we could greatly clean this up by tracking, for each color, the active
  // rep and the number of times we've seen it...but only write it out either (a)
  // when the rep changes (b) when we get the color again after a gap or (c) at the
  // end. that way we wouldn't need maintain these prevactive/active sets...
  for(x = qs->bargs->begx ; x < (qs->bargs->begx + qs->lenx) ; ++x){ // pixel column
    // there are at most 6 colors represented in any given sixel. at each
    // sixel, we need to *start tracking* new colors, and colors which changed
    // their representation. we also write out what we previously tracked for
    // this color: possibly a non-zero rep, possibly followed by a zero-rep (we
    // can have zero, either, or both).
    int activepos = 0; // number of active entries used
    for(int y = ystart ; y < endy ; ++y){
      const uint32_t* rgb = (qs->data + (qs->linesize / 4 * y) + x);
      if(rgba_trans_p(*rgb, qs->bargs->transcolor)){
        continue;
      }
      int cidx = find_color(qs, *rgb);
      if(cidx < 0){
        // FIXME free?
        return -1;
      }
      int act;
      for(act = 0 ; act < activepos ; ++act){
        if(active[act].color == cidx){
          active[act].rep |= (1u << (y - ystart));
          break;
        }
      }
      if(act == activepos){ // didn't find it; create new entry
        active[activepos].color = cidx;
        active[activepos].rep = (1u << (y - ystart));
        ++activepos;
      }
    }
    // we now have the active set. check to see if they extend existing RLEs,
    // and if not, write out whatever came before us.
    for(int i = 0 ; i < activepos ; ++i){
      const int c = active[i].color;
      if(meta[c].rep == active[i].rep && meta[c].rle + meta[c].wrote == x){
        ++meta[c].rle;
      }else{
        b->vecs[c] = sixelband_extend(b->vecs[c], &meta[c], qs->lenx, x);
        if(b->vecs[c] == NULL){
          return -1;
        }
        meta[c].rle = 1;
        meta[c].wrote = x;
        meta[c].rep = active[i].rep;
      }
    }
  }
  for(int i = 0 ; i < qs->smap->colors ; ++i){
    if(meta[i].rle){ // color was wholly unused iff rle == 0 at end
      b->vecs[i] = sixelband_extend(b->vecs[i], &meta[i], qs->lenx, x);
      if(b->vecs[i] == NULL){
        return -1;
      }
    }else{
      b->vecs[i] = NULL;
    }
  }
  free(meta);
  return 0;
}

// we have converged upon some number of colors. we now run over the pixels
// once again, and get the actual (color-indexed) sixels.
static inline int
build_data_table(qstate* qs){
  sixelmap* smap = qs->smap;
  if(smap->sixelbands == 0){
    logerror("no sixels");
    return -1;
  }
  for(int i = 0 ; i < smap->sixelbands ; ++i){
    if(build_sixel_band(qs, i) < 0){
      return -1;
    }
  }
  size_t tsize = RGBSIZE * smap->colors;
  qs->table = malloc(tsize);
  if(qs->table == NULL){
    return -1;
  }
  load_color_table(qs);
  return 0;
}

static inline int
extract_cell_color_table(qstate* qs, long cellid){
  const int ccols = qs->bargs->u.pixel.spx->dimx;
  const long x = cellid % ccols;
  const long y = cellid / ccols;
  const int cdimy = qs->bargs->u.pixel.cellpxy;
  const int cdimx = qs->bargs->u.pixel.cellpxx;
  const int begy = qs->bargs->begy;
  const int begx = qs->bargs->begx;
  const int leny = qs->leny;
  const int lenx = qs->lenx;
  const int cstartx = begx + x * cdimx; // starting pixel col for cell
  const int cstarty = begy + y * cdimy; // starting pixel row for cell
  typeof(qs->bargs->u.pixel.spx->needs_refresh) rmatrix = qs->bargs->u.pixel.spx->needs_refresh;
  tament* tam = qs->bargs->u.pixel.spx->n->tam;
  int cendy = cstarty + cdimy;    // one past last pixel row for cell
  if(cendy > begy + leny){
    cendy = begy + leny;
  }
  int cendx = cstartx + cdimx;      // one past last pixel col for cell
  if(cendx > begx + lenx){
    cendx = begx + lenx;
  }
  // we initialize the TAM entry based on the first pixel. if it's transparent,
  // initialize as transparent, and otherwise as opaque. following that, any
  // transparent pixel takes opaque to mixed, and any filled pixel takes
  // transparent to mixed.
  if(cstarty >= cendy){ // we're entirely transparent sixel overhead
    tam[cellid].state = SPRIXCELL_TRANSPARENT;
    qs->smap->p2 = SIXEL_P2_TRANS; // even one forces P2=1
    // FIXME need we set rmatrix?
    return 0;
  }
  const uint32_t* rgb = (qs->data + (qs->linesize / 4 * cstarty) + cstartx);
  if(tam[cellid].state == SPRIXCELL_ANNIHILATED || tam[cellid].state == SPRIXCELL_ANNIHILATED_TRANS){
    if(rgba_trans_p(*rgb, qs->bargs->transcolor)){
      update_rmatrix(rmatrix, cellid, tam);
      tam[cellid].state = SPRIXCELL_ANNIHILATED_TRANS;
      free(tam[cellid].auxvector);
      tam[cellid].auxvector = NULL;
    }else{
      update_rmatrix(rmatrix, cellid, tam);
      free(tam[cellid].auxvector);
      tam[cellid].auxvector = NULL;
    }
  }else{
    if(rgba_trans_p(*rgb, qs->bargs->transcolor)){
      update_rmatrix(rmatrix, cellid, tam);
      tam[cellid].state = SPRIXCELL_TRANSPARENT;
    }else{
      update_rmatrix(rmatrix, cellid, tam);
      tam[cellid].state = SPRIXCELL_OPAQUE_SIXEL;
    }
  }
  for(int visy = cstarty ; visy < cendy ; ++visy){   // current abs pixel row
    for(int visx = cstartx ; visx < cendx ; ++visx){ // current abs pixel col
      rgb = (qs->data + (qs->linesize / 4 * visy) + visx);
      // we do *not* exempt already-wiped pixels from palette creation. once
      // we're done, we'll call sixel_wipe() on these cells. so they remain
      // one of SPRIXCELL_ANNIHILATED or SPRIXCELL_ANNIHILATED_TRANS.
      // intentional bitwise or, to avoid dependency
      if(tam[cellid].state != SPRIXCELL_ANNIHILATED){
        if(tam[cellid].state == SPRIXCELL_ANNIHILATED_TRANS){
          if(!rgba_trans_p(*rgb, qs->bargs->transcolor)){
            tam[cellid].state = SPRIXCELL_ANNIHILATED;
          }
        }else{
          if(rgba_trans_p(*rgb, qs->bargs->transcolor)){
            if(tam[cellid].state == SPRIXCELL_OPAQUE_SIXEL){
              tam[cellid].state = SPRIXCELL_MIXED_SIXEL;
            }
          }else{
            if(tam[cellid].state == SPRIXCELL_TRANSPARENT){
              tam[cellid].state = SPRIXCELL_MIXED_SIXEL;
            }
          }
        }
      }
//fprintf(stderr, "vis: %d/%d\n", visy, visx);
      if(rgba_trans_p(*rgb, qs->bargs->transcolor)){
        continue;
      }
      if(insert_color(qs, *rgb)){
        return -1;
      }
    }
  }
  // if we're opaque, we needn't clear the old cell with a glyph
  if(tam[cellid].state == SPRIXCELL_OPAQUE_SIXEL){
    rmatrix[cellid] = 0;
  }else{
    qs->smap->p2 = SIXEL_P2_TRANS; // even one forces P2=1
  }
  return 0;
}

// we have a 4096-element array that takes the 4-5-3 MSBs from the RGB
// comoponents. once it's complete, we might need to either merge some
// chunks, or expand them, converging towards the available number of
// color registers. |ccols| is cell geometry; |leny| and |lenx| are pixel
// geometry, and *do not* include sixel padding.
static int
extract_color_table(qstate* qs){
  const blitterargs* bargs = qs->bargs;
  // use the cell geometry as computed by the visual layer; leny doesn't
  // include any mandatory sixel padding.
  const int crows = bargs->u.pixel.spx->dimy;
  const int ccols = bargs->u.pixel.spx->dimx;
  typeof(bargs->u.pixel.spx->needs_refresh) rmatrix;
  rmatrix = malloc(sizeof(*rmatrix) * crows * ccols);
  if(rmatrix == NULL){
    return -1;
  }
  bargs->u.pixel.spx->needs_refresh = rmatrix;
  long cellid = 0;
  for(int y = 0 ; y < crows ; ++y){ // cell row
    for(int x = 0 ; x < ccols ; ++x){ // cell column
      if(extract_cell_color_table(qs, cellid)){
        return -1;
      }
      ++cellid;
    }
  }
  loginfo("octree got %"PRIu32" entries", qs->smap->colors);
  if(merge_color_table(qs)){
    return -1;
  }
  if(build_data_table(qs)){
    return -1;
  }
  loginfo("final palette: %u/%u colors", qs->smap->colors, qs->bargs->u.pixel.colorregs);
  return 0;
}

static inline int
write_sixel_intro(fbuf* f, sixel_p2_e p2, int leny, int lenx){
  int rr, r = fbuf_puts(f, "\x1bP0;");
  if(r < 0){
    return -1;
  }
  if((rr = fbuf_putint(f, p2)) < 0){
    return -1;
  }
  r += rr;
  if((rr = fbuf_puts(f, ";0q\"1;1;")) < 0){
    return -1;
  }
  r += rr;
  if((rr = fbuf_putint(f, lenx)) < 0){
    return -1;
  }
  r += rr;
  if(fbuf_putc(f, ';') != 1){
    return -1;
  }
  ++r;
  if((rr = fbuf_putint(f, leny)) < 0){
    return -1;
  }
  r += rr;
  return r;
}

// write a single color register. rc/gc/bc are on [0..100].
static inline int
write_sixel_creg(fbuf* f, int idx, int rc, int gc, int bc){
  int rr, r = 0;
  if(fbuf_putc(f, '#') != 1){
    return -1;
  }
  ++r;
  if((rr = fbuf_putint(f, idx)) < 0){
    return -1;
  }
  r += rr;
  if((rr = fbuf_puts(f, ";2;")) < 0){
    return -1;
  }
  r += rr;
  if((rr = fbuf_putint(f, rc)) < 0){
    return -1;
  }
  r += rr;
  if(fbuf_putc(f, ';') != 1){
    return -1;
  }
  ++r;
  if((rr = fbuf_putint(f, gc)) < 0){
    return -1;
  }
  r += rr;
  if(fbuf_putc(f, ';') != 1){
    return -1;
  }
  ++r;
  if((rr = fbuf_putint(f, bc)) < 0){
    return -1;
  }
  r += rr;
  return r;
}

// write the escape which opens a Sixel, plus the palette table. returns the
// number of bytes written, so that this header can be directly copied in
// future reencodings. |leny| and |lenx| are output pixel geometry.
// returns the number of bytes written, so it can be stored at *parse_start.
static int
write_sixel_header(qstate* qs, fbuf* f, int leny){
  if(leny % 6){
    return -1;
  }
  // Set Raster Attributes - pan/pad=1 (pixel aspect ratio), Ph=qs->lenx, Pv=leny
  int r = write_sixel_intro(f, qs->smap->p2, leny, qs->lenx);
  if(r < 0){
    return -1;
  }
  for(int i = 0 ; i < qs->smap->colors ; ++i){
    const unsigned char* rgb = qs->table + i * RGBSIZE;
    //fprintf(fp, "#%d;2;%u;%u;%u", i, rgb[0], rgb[1], rgb[2]);
    int rr = write_sixel_creg(f, i, rgb[0], rgb[1], rgb[2]);
    if(rr < 0){
      return -1;
    }
    r += rr;
  }
  return r;
}

static int
write_sixel_payload(fbuf* f, const sixelmap* map){
  for(int j = 0 ; j < map->sixelbands ; ++j){
    int needclosure = 0;
    const sixelband* band = &map->bands[j];
    for(int i = 0 ; i < band->size ; ++i){
      if(band->vecs[i]){
        if(needclosure){
          if(fbuf_putc(f, '$') != 1){ // end previous one
            return -1;
          }
        }else{
          needclosure = 1;
        }
        if(fbuf_putc(f, '#') != 1){
          return -1;
        }
        if(fbuf_putint(f, i) < 0){
          return -1;
        }
        if(fbuf_puts(f, band->vecs[i]) < 0){
          return -1;
        }
      }
    }
    if(fbuf_putc(f, '-') != 1){
      return -1;
    }
  }
  if(fbuf_puts(f, "\e\\") < 0){
    return -1;
  }
  return 0;
}

// once per render cycle (if needed), make the actual payload match the TAM. we
// don't do these one at a time due to the complex (expensive) process involved
// in regenerating a sixel (we can't easily do it in-place). anything newly
// ANNIHILATED (state is ANNIHILATED, but no auxvec present) is dropped from
// the payload, and an auxvec is generated. anything newly restored (state is
// OPAQUE_SIXEL or MIXED_SIXEL, but an auxvec is present) is restored to the
// payload, and the auxvec is freed. none of this takes effect until the sixel
// is redrawn, and annihilated sprixcells still require a glyph to be emitted.
static inline int
sixel_reblit(sprixel* s){
  fbuf_chop(&s->glyph, s->parse_start);
  if(write_sixel_payload(&s->glyph, s->smap) < 0){
    return -1;
  }
  change_p2(s->glyph.buf, s->smap->p2);
  return 0;
}

// write out the sixel header after having quantized the palette.
static inline int
sixel_blit_inner(qstate* qs, sixelmap* smap, const blitterargs* bargs, tament* tam){
  fbuf f;
  if(fbuf_init(&f)){
    return -1;
  }
  sprixel* s = bargs->u.pixel.spx;
  const int cellpxy = bargs->u.pixel.cellpxy;
  const int cellpxx = bargs->u.pixel.cellpxx;
  int outy = qs->leny;
  if(outy % 6){
    outy += 6 - (qs->leny % 6);
    smap->p2 = SIXEL_P2_TRANS;
  }
  int parse_start = write_sixel_header(qs, &f, outy);
  if(parse_start < 0){
    fbuf_free(&f);
    return -1;
  }
  // we don't write out the payload yet -- set wipes_outstanding high, and
  // it'll be emitted via sixel_reblit(), taking into account any wipes that
  // occurred before it was displayed. otherwise, such a wipe would require
  // two emissions, one of which would be thrown away.
  scrub_tam_boundaries(tam, outy, qs->lenx, cellpxy, cellpxx);
  // take ownership of buf on success
  if(plane_blit_sixel(s, &f, outy, qs->lenx, parse_start, tam, SPRIXEL_INVALIDATED) < 0){
    fbuf_free(&f);
    return -1;
  }
  s->smap = smap;
  return 1;
}

// |leny| and |lenx| are the scaled output geometry. we take |leny| up to the
// nearest multiple of six greater than or equal to |leny|.
int sixel_blit(ncplane* n, int linesize, const void* data, int leny, int lenx,
               const blitterargs* bargs){
  if(bargs->u.pixel.colorregs >= TRANS_PALETTE_ENTRY){
    logerror("palette too large %d", bargs->u.pixel.colorregs);
    return -1;
  }
  sixelmap* smap = sixelmap_create(leny - bargs->begy);
  if(smap == NULL){
    return -1;
  }
  assert(n->tam);
  qstate qs;
  if(alloc_qstate(bargs->u.pixel.colorregs, &qs)){
    logerror("couldn't allocate qstate");
    sixelmap_free(smap);
    return -1;
  }
  qs.bargs = bargs;
  qs.data = data;
  qs.linesize = linesize;
  qs.smap = smap;
  qs.leny = leny;
  qs.lenx = lenx;
  if(extract_color_table(&qs)){
    free(bargs->u.pixel.spx->needs_refresh);
    bargs->u.pixel.spx->needs_refresh = NULL;
    sixelmap_free(smap);
    free_qstate(&qs);
    return -1;
  }
  // takes ownership of sixelmap on success
  int r = sixel_blit_inner(&qs, smap, bargs, n->tam);
  free_qstate(&qs);
  if(r < 0){
    sixelmap_free(smap);
    // FIXME free refresh table?
  }
  scrub_color_table(bargs->u.pixel.spx);
  // we haven't actually emitted the body of the sixel yet. instead, we'll emit
  // it at sixel_redraw(), thus avoided a double emission in the case of wipes
  // taking place before it's visible.
  bargs->u.pixel.spx->wipes_outstanding = 1;
  return r;
}

// to destroy a sixel, we damage all cells underneath it. we might not have
// to, though, if we've got a new sixel ready to go where the old sixel was
// (though we'll still need to if the new sprixcell not opaque, and the
// old and new sprixcell are different in any transparent pixel).
int sixel_scrub(const ncpile* p, sprixel* s){
  loginfo("%d state %d at %d/%d (%d/%d)", s->id, s->invalidated, s->movedfromy, s->movedfromx, s->dimy, s->dimx);
  int starty = s->movedfromy;
  int startx = s->movedfromx;
  for(int yy = starty ; yy < starty + (int)s->dimy && yy < (int)p->dimy ; ++yy){
    for(int xx = startx ; xx < startx + (int)s->dimx && xx < (int)p->dimx ; ++xx){
      int ridx = yy * p->dimx + xx;
      struct crender *r = &p->crender[ridx];
      if(!s->n){
        // need this to damage cells underneath a sprixel we're removing
        r->s.damaged = 1;
        continue;
      }
      sprixel* trues = r->sprixel ? r->sprixel : s;
      if(yy >= (int)trues->n->leny || yy - trues->n->absy < 0){
        r->s.damaged = 1;
        continue;
      }
      if(xx >= (int)trues->n->lenx || xx - trues->n->absx < 0){
        r->s.damaged = 1;
        continue;
      }
      sprixcell_e state = sprixel_state(trues, yy, xx);
//fprintf(stderr, "CHECKING %d/%d state: %d %d/%d\n", yy - s->movedfromy - s->n->absy, xx - s->movedfromx - s->n->absx, state, yy, xx);
      if(state == SPRIXCELL_TRANSPARENT || state == SPRIXCELL_MIXED_SIXEL){
        r->s.damaged = 1;
      }else if(s->invalidated == SPRIXEL_MOVED){
        // ideally, we wouldn't damage our annihilated sprixcells, but if
        // we're being annihilated only during this cycle, we need to go
        // ahead and damage it.
        r->s.damaged = 1;
      }
    }
  }
  return 1;
}

// returns the number of bytes written
int sixel_draw(const tinfo* ti, const ncpile* p, sprixel* s, fbuf* f,
               int yoff, int xoff){
  (void)ti;
  // if we've wiped or rebuilt any cells, effect those changes now, or else
  // we'll get flicker when we move to the new location.
  if(s->wipes_outstanding){
    if(sixel_reblit(s)){
      return -1;
    }
    s->wipes_outstanding = false;
  }
  if(p){
    const int targy = s->n->absy + yoff;
    const int targx = s->n->absx + xoff;
    if(goto_location(p->nc, f, targy, targx, NULL)){
      return -1;
    }
    if(s->invalidated == SPRIXEL_MOVED){
      for(int yy = s->movedfromy ; yy < s->movedfromy + (int)s->dimy && yy < (int)p->dimy ; ++yy){
        if(yy < 0){
          continue;
        }
        for(int xx = s->movedfromx ; xx < s->movedfromx + (int)s->dimx && xx < (int)p->dimx ; ++xx){
          if(xx < 0){
            continue;
          }
          struct crender *r = &p->crender[yy * p->dimx + xx];
          if(!r->sprixel || sprixel_state(r->sprixel, yy, xx) != SPRIXCELL_OPAQUE_SIXEL){
            r->s.damaged = 1;
          }
        }
      }
    }
  }
  if(fbuf_putn(f, s->glyph.buf, s->glyph.used) < 0){
    return -1;
  }
  s->invalidated = SPRIXEL_QUIESCENT;
  return s->glyph.used;
}

// we keep a few worker threads spun up to assist with quantization.
typedef struct sixel_engine {
  // FIXME we'll want maybe one per core in our cpuset?
  pthread_t tids[3];
  unsigned workers;
  unsigned workers_wanted;
  pthread_mutex_t lock;
  pthread_cond_t cond;
  void* chunks; // FIXME
  bool done;
} sixel_engine;

// FIXME make this part of the context, sheesh
static sixel_engine globsengine;

// a quantization worker.
static void *
sixel_worker(void* v){
  sixel_engine *sengine = v;
  pthread_mutex_lock(&globsengine.lock);
  if(++sengine->workers < sengine->workers_wanted){
    pthread_mutex_unlock(&globsengine.lock);
    // don't bail on a failure here
    if(pthread_create(&sengine->tids[sengine->workers], NULL, sixel_worker, sengine)){
      logerror("couldn't spin up sixel worker %u", sengine->workers);
    }
  }else{
    pthread_mutex_unlock(&globsengine.lock);
  }
  do{
    pthread_mutex_lock(&sengine->lock);
    while(sengine->chunks == NULL && !sengine->done){
      pthread_cond_wait(&sengine->cond, &sengine->lock);
    }
    if(sengine->done){
      pthread_mutex_unlock(&sengine->lock);
      return NULL;
    }
    // FIXME take workchunk
    pthread_mutex_unlock(&sengine->lock);
    // FIXME handle workchunk
  }while(1);
}

static int
sixel_init_core(const char* initstr, int fd){
  globsengine.workers = 0;
  globsengine.workers_wanted = sizeof(globsengine.tids) / sizeof(*globsengine.tids);
  // don't fail on an error here
  if(pthread_create(globsengine.tids, NULL, sixel_worker, &globsengine)){
    logerror("couldn't spin up sixel workers");
  }
  return tty_emit(initstr, fd);
}

// private mode 80 (DECSDM) manages "Sixel Scrolling Mode" vs "Sixel Display
// Mode". when 80 is enabled (i.e. DECSDM mode), images are displayed at the
// upper left, and clipped to the window. we don't want either of those things
// to happen, so we explicitly disable DECSDM.
// private mode 8452 places the cursor at the end of a sixel when it's
//  emitted. we don't need this for rendered mode, but we do want it for
//  direct mode. it causes us no problems, so always set it.
int sixel_init_forcesdm(int fd){
  return sixel_init_core("\e[?80l\e[?8452h", fd);
}

int sixel_init_inverted(int fd){
  // some terminals, at some versions, invert the sense of DECSDM. for those,
  // we must use 80h rather than the correct 80l. this grows out of a
  // misunderstanding in XTerm through patchlevel 368, which was widely
  // copied into other terminals.
  return sixel_init_core("\e[?80h\e[?8452h", fd);
}

// if we aren't sure of the semantics of the terminal we're speaking with,
// don't touch DECSDM at all. it's almost certainly set up the way we want.
int sixel_init(int fd){
  return sixel_init_core("\e[?8452h", fd);
}

static inline int
restore_band(sixelmap* smap, int band, int startx, int endx,
             int starty, int endy, int dimx, int cellpixy, int cellpixx,
             uint8_t* auxvec){
  // FIXME
  return 0;
}

// only called for cells in SPRIXCELL_ANNIHILATED[_TRANS]. just post to
// wipes_outstanding, so the Sixel gets regenerated the next render cycle,
// just like wiping. this is necessary due to the complex nature of
// modifying a Sixel -- we want to do them all in one batch.
int sixel_rebuild(sprixel* s, int ycell, int xcell, uint8_t* auxvec){
//fprintf(stderr, "REBUILDING %d/%d\n", ycell, xcell);
  if(auxvec == NULL){
    return -1;
  }
  const int cellpxy = ncplane_pile(s->n)->cellpxy;
  const int cellpxx = ncplane_pile(s->n)->cellpxx;
  memset(auxvec + cellpxx * cellpxy, 0xff, cellpxx * cellpxy);
  sixelmap* smap = s->smap;
  const int startx = xcell * cellpxx;
  const int starty = ycell * cellpxy;
  int endx = ((xcell + 1) * cellpxx);
  if(endx >= s->pixx){
    endx = s->pixx;
  }
  int endy = ((ycell + 1) * cellpxy);
  if(endy >= s->pixy){
    endy = s->pixy;
  }
  const int startband = starty / 6;
  const int endband = endy / 6;
//fprintf(stderr, "%d/%d start: %d/%d end: %d/%d bands: %d-%d\n", ycell, xcell, starty, startx, endy, endx, starty / 6, endy / 6);
  // walk through each color, and wipe the necessary sixels from each band
  int w = 0;
  for(int b = startband ; b < endband ; ++b){
    w += restore_band(smap, b, startx, endx, starty, endy, s->pixx,
                      cellpxy, cellpxx, auxvec);
  }
  s->wipes_outstanding = true;
  // FIXME need to set this back up...how? return transparent count from
  // restore_band(), and sum them up?
  sprixcell_e newstate = SPRIXCELL_OPAQUE_SIXEL; // FIXME incorrect!
  /*
  if(transparent == cellpxx * cellpxy){
    newstate = SPRIXCELL_TRANSPARENT;
  }else if(transparent){
    newstate = SPRIXCELL_MIXED_SIXEL;
  }else{
    newstate = SPRIXCELL_OPAQUE_SIXEL;
  }
  */
  s->n->tam[s->dimx * ycell + xcell].state = newstate;
  return 1;
}

void sixel_cleanup(tinfo* ti){
  (void)ti; // FIXME pick up globsengine from ti!
  unsigned tids = 0;
  pthread_mutex_lock(&globsengine.lock);
  globsengine.done = 1;
  tids = globsengine.workers;
  pthread_mutex_unlock(&globsengine.lock);
  pthread_cond_broadcast(&globsengine.cond);
  // FIXME what if we spawned another worker since taking zee lock?
  loginfo("joining %u sixel thread%s", tids, tids == 1 ? "" : "s");
  for(unsigned t = 0 ; t < tids ; ++t){
    pthread_join(globsengine.tids[t], NULL);
  }
  loginfo("reaped sixel engine");
  // no way to know what the state was before; we ought use XTSAVE/XTRESTORE
}

uint8_t* sixel_trans_auxvec(const ncpile* p){
  const size_t slen = 3 * p->cellpxy * p->cellpxx;
  uint8_t* a = malloc(slen);
  if(a){
    memset(a, 0, slen);
  }
  return a;
}
