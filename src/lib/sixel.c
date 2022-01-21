#include <math.h>
#include "internal.h"
#include "fbuf.h"

#define RGBSIZE 3

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

typedef struct qstate {
  qnode* qnodes;
  onode* onodes;
  unsigned dynnodes_free;
  unsigned dynnodes_total;
  unsigned onodes_free;
  unsigned onodes_total;
} qstate;


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
  return 0;
}

// free internals of qstate object
static void
free_qstate(qstate *qs){
  if(qs){
    loginfo("freeing qstate");
    free(qs->qnodes);
    free(qs->onodes);
  }
}

// insert a color from the source image into the octree.
static int
insert_color(qstate* qs, uint32_t pixel, uint32_t* colors){
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
    ++*colors;
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
  ++*colors;
//fprintf(stderr, "INSERTED[%u]: %u %u %u\n", key, q->q.comps[0], q->q.comps[1], q->q.comps[2]);
  return 0;
}

// resolve the input color to a color table index following any postprocessing
// of the octree.
static int
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
//fprintf(stderr, "OH NOOOOOOOOOO %u:%u QLINK: %u\n", key, skey, q->qlink); // FIXME find one
      return -1;
    }
  }
  return qidx(q);
}

// size of a color table entry: just the three components
#define CENTSIZE (RGBSIZE)

// we set P2 based on whether there is any transparency in the sixel. if not,
// use SIXEL_P2_ALLOPAQUE (0), for faster drawing in certain terminals.
typedef enum {
  SIXEL_P2_ALLOPAQUE = 0,
  SIXEL_P2_TRANS = 1,
} sixel_p2_e;

// returns the number of individual sixels necessary to represent the specified
// pixel geometry. these might encompass more pixel rows than |dimy| would
// suggest, up to the next multiple of 6 (i.e. a single row becomes a 6-row
// bitmap; as do two, three, four, five, or six rows). input is scaled geometry.
static inline int
sixelcount(int dimy, int dimx){
  return (dimy + 5) / 6 * dimx;
}

// create an auxiliary vector suitable for a Sixel sprixcell, and zero it out.
// there are three bytes per pixel in the cell: a contiguous set of 16-bit
// palette indices, and a contiguous set of two-value transparencies (these
// could be folded down to bits from bytes, saving 7/8 of the space FIXME).
static inline uint8_t*
sixel_auxiliary_vector(const sprixel* s){
  int pixels = ncplane_pile(s->n)->cellpxy * ncplane_pile(s->n)->cellpxx;
  size_t slen = pixels * 3;
  uint8_t* ret = malloc(slen);
  if(ret){
    memset(ret, 0, sizeof(slen));
  }
  return ret;
}

// we keep a color-indexed set of sixels (a single-row column of six pixels,
// encoded as a byte) across the life of the sprixel. This provides a good
// combination of easy-to-edit (for wipes and restores) -- you can index by
// color, and then by position, in O(1) -- and a form which can easily be
// converted to the actual Sixel encoding. wipes and restores come in and edit
// these sixels in O(1), and then at display time we recreate the encoded
// bitmap in one go if necessary. we could just wipe and restore directly using
// the encoded form, but it's a tremendous pain in the ass. this sixelmap will
// be kept in the sprixel.
// for initial quantization we fill out a static 5-level octree, with a sixth
// dynamic level. we then merge as necessary to converge upon the number of
// color registers. once we've settled on the colors, we run the pixels through
// the octree again to build up the sixelmap.
typedef struct sixelmap {
  int colors;
  int sixelcount;
  // FIXME we ought be able to combine these under the new scheme
  // for each color, for each sixel (stack of six), the representation.
  unsigned char* data;  // |colors| x |sixelcount|-byte arrays
  // for each color, the components and a dindex.
  unsigned char* table; // |colors| x CENTSIZE: components
  sixel_p2_e p2;        // set to SIXEL_P2_TRANS if we have transparent pixels
} sixelmap;

// second pass: construct data for extracted colors over the sixels. the
// map will be persisted in the sprixel; the remainder is lost.
typedef struct sixeltable {
  sixelmap* map;        // copy of palette indices / transparency bits
  int colorregs;        // *available* color registers
} sixeltable;

// whip up a sixelmap sans data for the specified pixel geometry and color
// register count.
static sixelmap*
sixelmap_create(int dimy, int dimx){
  sixelmap* ret = malloc(sizeof(*ret));
  if(ret){
    ret->p2 = SIXEL_P2_ALLOPAQUE;
    ret->sixelcount = sixelcount(dimy, dimx);
    ret->data = NULL;
    ret->table = NULL;
  }
  return ret;
}

void sixelmap_free(sixelmap *s){
  if(s){
    free(s->table);
    free(s->data);
    free(s);
  }
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

// wipe the color from startx to endx, from starty to endy. returns 1 if any
// pixels were actually wiped.
static inline int
wipe_color(sixelmap* smap, int color, int sband, int eband,
           int startx, int endx, int starty, int endy, int dimx,
           int cellpixy, int cellpixx, uint8_t* auxvec){
  int wiped = 0;
  // offset into map->data where our color starts
  int coff = smap->sixelcount * color;
//fprintf(stderr, "sixels: %d color: %d B: %d-%d Y: %d-%d X: %d-%d coff: %d\n", smap->sixelcount, color, sband, eband, starty, endy, startx, endx, coff);
  // we're going to repurpose starty as "starting row of this band", so keep it
  // around as originy for auxvecidx computations
  int originy = starty;
  for(int b = sband ; b <= eband && b * 6 <= endy ; ++b){
    const int boff = coff + b * dimx; // offset in data where band starts
    unsigned char mask = 63;
    for(int i = 0 ; i < 6 ; ++i){
      if(b * 6 + i >= starty && b * 6 + i <= endy){
        mask &= ~(1u << i);
      }
//fprintf(stderr, "s/e: %d/%d mask: %02x\n", starty, endy, mask);
    }
    for(int x = startx ; x <= endx ; ++x){
      const int xoff = boff + x;
      assert(xoff < (smap->colors + 1) * smap->sixelcount);
//fprintf(stderr, "band: %d color: %d idx: %d mask: %02x\n", b, color, color * smap->sixelcount + xoff, mask);
//fprintf(stderr, "color: %d idx: %d data: %02x\n", color, color * smap->sixelcount + xoff, smap->data[color * smap->sixelcount + xoff]);
      // this is the auxvec position of the upperleftmost pixel of the sixel
      // there will be up to five more, each cellpxx away, for the five pixels
      // below it. there will be cellpxx - 1 after it, each with their own five.
//fprintf(stderr, "smap->data[%d] = %02x boff: %d x: %d color: %d\n", xoff, smap->data[xoff], boff, x, color);
      for(int i = 0 ; i < 6 && b * 6 + i <= endy ; ++i){
        int auxvecidx = (x - startx) + ((b * 6 + i - originy) * cellpixx);
        unsigned bit = 1u << i;
//fprintf(stderr, "xoff: %d i: %d b: %d endy: %d mask: 0x%02x\n", xoff, i, b, endy, mask);
        if(!(mask & bit) && (smap->data[xoff] & bit)){
//fprintf(stderr, "band %d %d/%d writing %d to auxvec[%d] %p xoff: %d boff: %d\n", b, b * 6 + i, x, color, auxvecidx, auxvec, xoff, boff);
          auxvec[auxvecidx] = color;
          auxvec[cellpixx * cellpixy + auxvecidx] = 0;
        }
      }
      if((smap->data[xoff] & mask) != smap->data[xoff]){
        smap->data[xoff] &= mask;
        wiped = 1;
      }
//fprintf(stderr, "post: %02x\n", smap->data[color * smap->sixelcount + xoff]);
    }
    starty = (starty + 6) / 6 * 6;
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
  int endx = ((xcell + 1) * cellpxx) - 1;
  if(endx >= s->pixx){
    endx = s->pixx - 1;
  }
  int endy = ((ycell + 1) * cellpxy) - 1;
  if(endy >= s->pixy){
    endy = s->pixy - 1;
  }
  const int startband = starty / 6;
  const int endband = endy / 6;
//fprintf(stderr, "y/x: %d/%d start: %d/%d end: %d/%d\n", ycell, xcell, starty, startx, endy, endx);
  // walk through each color, and wipe the necessary sixels from each band
  int w = 0;
  for(int c = 0 ; c < smap->colors ; ++c){
    w |= wipe_color(smap, c, startband, endband, startx, endx, starty, endy,
                    s->pixx, cellpxy, cellpxx, auxvec);
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
merge_color_table(qstate* qs, uint32_t* colors, uint32_t colorregs){
  if(*colors == 0){
    return 0;
  }
  qnode* qactive = get_active_set(qs, *colors);
  if(qactive == NULL){
    return -1;
  }
  // assign color table entries to the most popular colors. use the lowest
  // color table entries for the most popular ones, as they're the shortest
  // (this is not necessarily an optimizing huristic, but it'll do for now).
  unsigned cidx = 0;
//fprintf(stderr, "colors: %u cregs: %u\n", *colors, colorregs);
  for(int z = *colors - 1 ; z >= 0 ; --z){
    if(*colors >= colorregs){
      if(cidx == colorregs){
        break; // we just ran out of color registers
      }
    }
    qs->qnodes[qactive[z].qlink].cidx = make_chosen(cidx);
    ++cidx;
  }
  free(qactive);
  if(*colors > colorregs){
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
    *colors = colorregs;
  }
  return 0;
}

static inline void
load_color_table(const qstate* qs, uint32_t colors, unsigned char* table){
  uint32_t loaded = 0;
  unsigned total = QNODECOUNT + (qs->dynnodes_total - qs->dynnodes_free);
  for(unsigned z = 0 ; z < total && loaded < colors ; ++z){
    const qnode* q = &qs->qnodes[z];
    if(chosen_p(q)){
      table[CENTSIZE * qidx(q) + 0] = ss(q->q.comps[0]);
      table[CENTSIZE * qidx(q) + 1] = ss(q->q.comps[1]);
      table[CENTSIZE * qidx(q) + 2] = ss(q->q.comps[2]);
      ++loaded;
    }
  }
//fprintf(stderr, "loaded: %u colors: %u\n", loaded, colors);
  assert(loaded == colors);
}

// we have converged upon colorregs in the octree. we now run over the pixels
// once again, and get the actual final color table entries.
static inline int
build_data_table(qstate* qs, uint32_t colors, sixeltable* stab, const uint32_t* data,
                 int linesize, int begy, int begx, int leny, int lenx,
                 uint32_t transcolor){
  if(stab->map->sixelcount == 0){
    logerror("no sixels");
    return -1;
  }
  // FIXME merge these two
  size_t dsize = sizeof(*stab->map->data) * colors * stab->map->sixelcount;
  stab->map->data = malloc(dsize);
  if(stab->map->data == NULL){
    return -1;
  }
  size_t tsize = CENTSIZE * colors;
  stab->map->table = malloc(tsize);
  if(stab->map->table == NULL){
    free(stab->map->data);
    stab->map->data = NULL;
    return -1;
  }
  load_color_table(qs, colors, stab->map->table);
  memset(stab->map->data, 0, dsize);
  stab->map->colors = colors;
  int pos = 0;
//fprintf(stderr, "BUILDING DATA TABLE\n");
  for(int visy = begy ; visy < (begy + leny) ; visy += 6){ // pixel row
    for(int visx = begx ; visx < (begx + lenx) ; visx += 1){ // pixel column
      for(int sy = visy ; sy < (begy + leny) && sy < visy + 6 ; ++sy){ // offset within sprixel
        const uint32_t* rgb = (data + (linesize / 4 * sy) + visx);
        if(rgba_trans_p(*rgb, transcolor)){
          continue;
        }
        int cidx = find_color(qs, *rgb);
        if(cidx < 0){
          free(stab->map->table);
          stab->map->table = NULL;
          free(stab->map->data);
          stab->map->data = NULL;
          return -1;
        }
        stab->map->data[cidx * stab->map->sixelcount + pos] |= (1u << (sy - visy));
      }
      ++pos;
    }
  }
  return 0;
}

// we have a 4096-element array that takes the 4-5-3 MSBs from the RGB
// comoponents. once it's complete, we might need to either merge some
// chunks, or expand them, converging towards the available number of
// color registers.
static inline int
extract_color_table(const uint32_t* data, int linesize, int cols,
                    int leny, int lenx, sixeltable* stab,
                    tament* tam, const blitterargs* bargs){
  uint64_t pixels = 0;
  uint32_t octets = 0;
  qstate qs;
  if(alloc_qstate(bargs->u.pixel.colorregs, &qs)){
    logerror("couldn't allocate qstate");
    return -1;
  }
  const int begx = bargs->begx;
  const int begy = bargs->begy;
  const int cdimy = bargs->u.pixel.cellpxy;
  const int cdimx = bargs->u.pixel.cellpxx;
  int pos = 0; // pixel position
  unsigned char* rmatrix = bargs->u.pixel.spx->needs_refresh;
  for(int visy = begy ; visy < (begy + leny) ; visy += 6){ // pixel row
    for(int visx = begx ; visx < (begx + lenx) ; visx += 1){ // pixel column
      for(int sy = visy ; sy < (begy + leny) && sy < visy + 6 ; ++sy){ // offset within sprixel
        const uint32_t* rgb = (data + (linesize / 4 * sy) + visx);
        int txyidx = (sy / cdimy) * cols + (visx / cdimx);
        // we can't just check if lastidx != txyidx; that's true for each row
        // of the cell. this will only be true once.
        bool firstpix = (sy % cdimy == 0 && visx % cdimx == 0);
        bool lastrow = visy + 6 >= begy + leny;
        // we do *not* exempt already-wiped pixels from palette creation. once
        // we're done, we'll call sixel_wipe() on these cells. so they remain
        // one of SPRIXCELL_ANNIHILATED or SPRIXCELL_ANNIHILATED_TRANS.
        if(tam[txyidx].state != SPRIXCELL_ANNIHILATED && tam[txyidx].state != SPRIXCELL_ANNIHILATED_TRANS){
          if(rgba_trans_p(*rgb, bargs->transcolor)){
            if(firstpix){
              update_rmatrix(rmatrix, txyidx, tam);
              tam[txyidx].state = SPRIXCELL_TRANSPARENT;
            }else if(tam[txyidx].state == SPRIXCELL_OPAQUE_SIXEL){
              tam[txyidx].state = SPRIXCELL_MIXED_SIXEL;
            }
            stab->map->p2 = SIXEL_P2_TRANS; // even one forces P2=1
          }else{
            if(firstpix){
              update_rmatrix(rmatrix, txyidx, tam);
              tam[txyidx].state = SPRIXCELL_OPAQUE_SIXEL;
            }else if(tam[txyidx].state == SPRIXCELL_TRANSPARENT){
              tam[txyidx].state = SPRIXCELL_MIXED_SIXEL;
            }
          }
        }else{
//fprintf(stderr, "TRANS SKIP %d %d %d %d (cell: %d %d)\n", visy, visx, sy, txyidx, sy / cdimy, visx / cdimx);
          if(rgba_trans_p(*rgb, bargs->transcolor)){
            if(firstpix){
              update_rmatrix(rmatrix, txyidx, tam);
              tam[txyidx].state = SPRIXCELL_ANNIHILATED_TRANS;
              free(tam[txyidx].auxvector);
              tam[txyidx].auxvector = NULL;
            }
          }else{
            if(firstpix){
              update_rmatrix(rmatrix, txyidx, tam);
              free(tam[txyidx].auxvector);
              tam[txyidx].auxvector = NULL;
            }
            tam[txyidx].state = SPRIXCELL_ANNIHILATED;
          }
          stab->map->p2 = SIXEL_P2_TRANS; // even one forces P2=1
        }
        if(lastrow){
          bool lastcol = visx + 1 >= begx + lenx;
          if(lastcol){
            // if we're opaque, we needn't clear the old cell with a glyph
            if(tam[txyidx].state == SPRIXCELL_OPAQUE_SIXEL){
              if(rmatrix){
                rmatrix[txyidx] = 0;
              }
            }
          }
        }
        if(rgba_trans_p(*rgb, bargs->transcolor)){
          continue;
        }
        if(insert_color(&qs, *rgb, &octets)){
          free_qstate(&qs);
          return -1;
        }
        ++pixels;
      }
      ++pos;
    }
  }
  loginfo("octree got %"PRIu32" entries on %"PRIu64" pixels", octets, pixels);
  if(merge_color_table(&qs, &octets, stab->colorregs)){
    free_qstate(&qs);
    return -1;
  }
  if(build_data_table(&qs, octets, stab, data, linesize, begy, begx, leny, lenx,
                      bargs->transcolor)){
    free_qstate(&qs);
    return -1;
  }
  loginfo("final palette: %u/%u colors", octets, stab->colorregs);
  free_qstate(&qs);
  return 0;
}

// Emit some number of equivalent, subsequent sixels, using sixel RLE. We've
// seen the sixel |crle| for |seenrle| columns in a row. |seenrle| must > 0.
static int
write_rle(int* printed, int color, fbuf* f, int seenrle, unsigned char crle,
          int* needclosure){
  if(!*printed){
    if(*needclosure){
      if(fbuf_putc(f, '$') != 1){
        return -1;
      }
    }
    if(fbuf_putc(f, '#') != 1){
      return -1;
    }
    if(fbuf_putint(f, color) < 0){
      return -1;
    }
    *printed = 1;
    *needclosure = 0;
  }
  crle += 63;
  if(seenrle == 2){
    if(fbuf_putc(f, crle) != 1){
      return -1;
    }
  }else if(seenrle != 1){
    if(fbuf_putc(f, '!') != 1){
      return -1;
    }
    if(fbuf_putint(f, seenrle) < 0){
      return -1;
    }
  }
  if(fbuf_putc(f, crle) != 1){
    return -1;
  }
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
write_sixel_header(fbuf* f, int leny, int lenx, const sixelmap* smap){
  if(leny % 6){
    return -1;
  }
  // Set Raster Attributes - pan/pad=1 (pixel aspect ratio), Ph=lenx, Pv=leny
  int r = write_sixel_intro(f, smap->p2, leny, lenx);
  if(r < 0){
    return -1;
  }
  for(int i = 0 ; i < smap->colors ; ++i){
    const unsigned char* rgb = smap->table + i * CENTSIZE;
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
write_sixel_payload(fbuf* f, int lenx, const sixelmap* map){
  int p = 0;
  while(p < map->sixelcount){
    int needclosure = 0;
    for(int i = 0 ; i < map->colors ; ++i){
      int seenrle = 0; // number of repetitions
      unsigned char crle = 0; // character being repeated
      int printed = 0;
      for(int m = p ; m < map->sixelcount && m < p + lenx ; ++m){
//fprintf(stderr, "%d ", i * map->sixelcount + m);
//fputc(map->data[i * map->sixelcount + m] + 63, stderr);
        if(seenrle){
          if(map->data[i * map->sixelcount + m] == crle){
            ++seenrle;
          }else{
            if(write_rle(&printed, i, f, seenrle, crle, &needclosure)){
              return -1;
            }
            seenrle = 1;
            crle = map->data[i * map->sixelcount + m];
          }
        }else{
          seenrle = 1;
          crle = map->data[i * map->sixelcount + m];
        }
      }
      if(crle){
        if(write_rle(&printed, i, f, seenrle, crle, &needclosure)){
          return -1;
        }
      }
      needclosure = needclosure | printed;
    }
    if(p + lenx < map->sixelcount){
      if(fbuf_putc(f, '-') != 1){
        return -1;
      }
    }
    p += lenx;
  }
  if(fbuf_puts(f, "\e\\") < 0){
    return -1;
  }
  return 0;
}

// emit the sixel in its entirety, plus escapes to start and end pixel mode.
// only called the first time we encode; after that, the palette remains
// constant, and is simply copied. fclose()s |fp| on success. |outx| and |outy|
// are output geometry.
static int
write_sixel(fbuf* f, int outy, int outx, const sixeltable* stab, int* parse_start){
  *parse_start = write_sixel_header(f, outy, outx, stab->map);
  if(*parse_start < 0){
    return -1;
  }
  if(write_sixel_payload(f, outx, stab->map) < 0){
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
  fbuf f;
  if(fbuf_init(&f)){
    return -1;
  }
  if(fbuf_putn(&f, s->glyph.buf, s->parse_start) != s->parse_start){
    fbuf_free(&f);
    return -1;
  }
  if(write_sixel_payload(&f, s->pixx, s->smap) < 0){
    fbuf_free(&f);
    return -1;
  }
  fbuf_free(&s->glyph);
  memcpy(&s->glyph, &f, sizeof(f));
  change_p2(s->glyph.buf, s->smap->p2);
  return 0;
}

// Sixel blitter. Sixels are stacks 6 pixels high, and 1 pixel wide. RGB colors
// are programmed as a set of registers, which are then referenced by the
// stacks. There is also a RLE component, handled in rasterization.
// A pixel block is indicated by setting cell_pixels_p(). |leny| and |lenx| are
// scaled geometry in pixels. We calculate output geometry herein, and supply
// transparent filler input for any missing rows.
static inline int
sixel_blit_inner(int leny, int lenx, sixeltable* stab, const blitterargs* bargs, tament* tam){
  fbuf f;
  if(fbuf_init(&f)){
    return -1;
  }
  sprixel* s = bargs->u.pixel.spx;
  const int cellpxy = bargs->u.pixel.cellpxy;
  const int cellpxx = bargs->u.pixel.cellpxx;
  int parse_start = 0;
  int outy = leny;
  if(leny % 6){
    outy += 6 - (leny % 6);
    stab->map->p2 = SIXEL_P2_TRANS;
  }
  // calls fclose() on success
  if(write_sixel(&f, outy, lenx, stab, &parse_start)){
    fbuf_free(&f);
    return -1;
  }
  scrub_tam_boundaries(tam, outy, lenx, cellpxy, cellpxx);
  // take ownership of buf on success
  if(plane_blit_sixel(s, &f, outy, lenx, parse_start, tam, SPRIXEL_INVALIDATED) < 0){
    fbuf_free(&f);
    return -1;
  }
  s->smap = stab->map;
  return 1;
}

// |leny| and |lenx| are the scaled output geometry. we take |leny| up to the
// nearest multiple of six greater than or equal to |leny|.
int sixel_blit(ncplane* n, int linesize, const void* data, int leny, int lenx,
               const blitterargs* bargs){
  int colorregs = bargs->u.pixel.colorregs;
  sixeltable stable = {
    .map = sixelmap_create(leny - bargs->begy, lenx - bargs->begx),
    .colorregs = colorregs,
  };
  if(stable.map == NULL){
    sixelmap_free(stable.map);
    return -1;
  }
  int cols = bargs->u.pixel.spx->dimx;
  int rows = bargs->u.pixel.spx->dimy;
  typeof(bargs->u.pixel.spx->needs_refresh) rmatrix;
  rmatrix = malloc(sizeof(*rmatrix) * rows * cols);
  if(rmatrix == NULL){
    sixelmap_free(stable.map);
    return -1;
  }
  bargs->u.pixel.spx->needs_refresh = rmatrix;
  assert(n->tam);
  if(extract_color_table(data, linesize, cols, leny, lenx, &stable, n->tam, bargs)){
    free(bargs->u.pixel.spx->needs_refresh);
    bargs->u.pixel.spx->needs_refresh = NULL;
    sixelmap_free(stable.map);
    return -1;
  }
  // takes ownership of sixelmap on success
  int r = sixel_blit_inner(leny, lenx, &stable, bargs, n->tam);
  if(r < 0){
    sixelmap_free(stable.map);
  }
  scrub_color_table(bargs->u.pixel.spx);
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

// private mode 80 (DECSDM) manages "Sixel Scrolling Mode" vs "Sixel Display
// Mode". when 80 is enabled (i.e. DECSDM mode), images are displayed at the
// upper left, and clipped to the window. we don't want either of those things
// to happen, so we explicitly disable DECSDM.
// private mode 8452 places the cursor at the end of a sixel when it's
//  emitted. we don't need this for rendered mode, but we do want it for
//  direct mode. it causes us no problems, so always set it.
int sixel_init(int fd){
  return tty_emit("\e[?80l\e[?8452h", fd);
}

int sixel_init_inverted(int fd){
  // some terminals, at some versions, invert the sense of DECSDM. for those,
  // we must use 80h rather than the correct 80l. this grows out of a
  // misunderstanding in XTerm through patchlevel 368, which was widely
  // copied into other terminals.
  return tty_emit("\e[?80h\e[?8452h", fd);
}

// only called for cells in SPRIXCELL_ANNIHILATED[_TRANS]. just post to
// wipes_outstanding, so the Sixel gets regenerated the next render cycle,
// just like wiping. this is necessary due to the complex nature of
// modifying a Sixel -- we want to do them all in one batch.
int sixel_rebuild(sprixel* s, int ycell, int xcell, uint8_t* auxvec){
  s->wipes_outstanding = true;
  sixelmap* smap = s->smap;
  const int cellpxx = ncplane_pile(s->n)->cellpxx;
  const int cellpxy = ncplane_pile(s->n)->cellpxy;
  const int startx = xcell * cellpxx;
  const int starty = ycell * cellpxy;
  int endx = ((xcell + 1) * cellpxx) - 1;
  if(endx > s->pixx){
    endx = s->pixx;
  }
  int endy = ((ycell + 1) * cellpxy) - 1;
  if(endy > s->pixy){
    endy = s->pixy;
  }
  int transparent = 0;
//fprintf(stderr, "%d/%d start: %d/%d end: %d/%d bands: %d-%d\n", ycell, xcell, starty, startx, endy, endx, starty / 6, endy / 6);
  for(int x = startx ; x <= endx ; ++x){
    for(int y = starty ; y <= endy ; ++y){
      int auxvecidx = (y - starty) * cellpxx + (x - startx);
      int trans = auxvec[cellpxx * cellpxy + auxvecidx];
      if(!trans){
        int color = auxvec[auxvecidx];
        int coff = smap->sixelcount * color;
        int band = y / 6;
        int boff = coff + band * s->pixx;
        int xoff = boff + x;
//fprintf(stderr, "%d/%d band: %d coff: %d boff: %d rebuild %d/%d with color %d from %d %p xoff: %d\n", ycell, xcell, band, coff, boff, y, x, color, auxvecidx, auxvec, xoff);
        s->smap->data[xoff] |= (1u << (y % 6));
      }else{
        ++transparent;
      }
    }
  }
  sprixcell_e newstate;
  if(transparent == cellpxx * cellpxy){
    newstate = SPRIXCELL_TRANSPARENT;
  }else if(transparent){
    newstate = SPRIXCELL_MIXED_SIXEL;
  }else{
    newstate = SPRIXCELL_OPAQUE_SIXEL;
  }
  s->n->tam[s->dimx * ycell + xcell].state = newstate;
  return 1;
}

int sixel_shutdown(fbuf* f){
  (void)f;
  // no way to know what the state was before; we ought use XTSAVE/XTRESTORE
  return 0;
}

uint8_t* sixel_trans_auxvec(const ncpile* p){
  const size_t slen = 3 * p->cellpxy * p->cellpxx;
  uint8_t* a = malloc(slen);
  if(a){
    memset(a, 0, slen);
  }
  return a;
}
