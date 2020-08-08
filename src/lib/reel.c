#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "internal.h"

// Tablets are the toplevel entitites within an ncreel. Each corresponds to
// a single, distinct ncplane.
typedef struct nctablet {
  ncplane* p;                    // visible panel, NULL when offscreen
  struct nctablet* next;
  struct nctablet* prev;
  tabletcb cbfxn;              // application callback to draw tablet
  void* curry;                 // application data provided to cbfxn
} nctablet;

typedef enum {
  DIRECTION_UP,
  DIRECTION_DOWN,
} direction_e;

// A UNIFIED THEORY OF NCREELS
// (which are more complex than they may seem)
//
// We only redraw when ncreel_redraw() is explicitly called (and at creation).
// Redrawing consists of arranging the tablets, and calling the user's
// callbacks to fill them in. In general, drawing a tablet involves creating a
// plane having all available size, calling the callback, and then trimming the
// plane if it was not filled.
//
// Things which can happen between ncreel_redraw() calls:
//
//  * new focused tablet added (only when no tablets exist)
//  * new unfocused tablet added
//  * tablet removed (may be the focused tablet)
//  * tablets may grow or shrink
//  * focus can change due to new selection
//
// Any number and mix of these events can occur between draws.
//
// First rule: there must not be 2+ consecutive lines of blank space if there is
//             data which could be presented there (always fill the reel).
// Second rule: if there must be 2+ consecutive lines of blank space, they must
//             all be at the bottom of the reel (connect and anchor the reel).
// Third rule: the focused tablet gets all the space it can use.
// Fourth rule: thou shalt never wrap a tablet [across a border]
// Fifth rule: the focused tablet should remain where it is across redraws,
//             except as necessary to accommodate the prior rules.
//
// At any ncreel_redraw(), you can make three types of moves:
//
//  - i. moving up and replacing the topmost tablet (spinning operation)
//  - ii. moving down and replacing the bottommost tablet (spinning operation)
//  - iii. don't move / move otherwise (no necessary spin)
//
// The first two are simple -- draw the focused tablet next to the appropriate
// border of the reel, and then draw what we can in the other direction until
// running out of space (and then shift up if there is more than one line of
// gap at the top, or if we were moving up from the topmost tablet). This can
// be done independently of all other tablets; it is immaterial if some were
// removed, added, etc. We can detect these two cases thus:
//
//  - store a direction_e, written to by ncreel_next(), ncreel_prev(), and
//     ncreel_del() (when the focused tablet is deleted), defaulting to DOWN.
//  - store a pointer to the "visibly focused" tablet, written and read only by
//     ncreel_redraw(). this identifies the focused tablet upon last redraw. if
//     the visibly-focused tablet is removed, it instead takes the next tablet,
//     iff that tablet is visible. it otherwise takes the prev tablet, iff that
//     tablet is visible. it otherwise takes NULL.
//  - with these, in ncreel_redraw(), we have:
//    - case i iff the last direction was UP, and either the focused tablet is
//       not visible, or below the visibly-focused tablet, or there is no
//       visibly-focused tablet.
//    - case ii iff the last direction was DOWN, and either the focused tablet
//       is not visible, or above the visibly-focused tablet, or there is no
//       visibly-focused tablet.
//
// We otherwise have case iii. The focused tablet must be on-screen (if it was
// off-screen, we matched one of case i or case ii). We want to draw it as near
// to its current position as possible, subject to the first four Rules.
//
// ncreel_redraw() thus starts by determining the case. This must be done
// before any changes are made to the arrangement. It then clears the reel.
// The focused tablet is drawn as close to its desired line as possible. For
// case i, then draw the tablets below the focused tablet. For case ii, then
// draw the tablets above the focused tablet (and move them up, if necessary).
// Both of these cases are then handled.
//
// For case iii, see below...
//
// On tablet remove:
//  * destroy plane, remove from list
//  * if tablet was focused, change focus, update travel direction
//  * if tablet was visibly focused, change visible focus
// On tablet add:
//  * add to list (do not create plane)
//  * if no tablet existed, change focus to new tablet
// On focus change:
//  * change focus, update travel direction
// On redraw:
//  * if no tablets, we're done (deleted planes are already gone)
//  * resize focused tablet to maximum, preserving nothing
//  * call back for focused tablet redraw
//  * shrink focused tablet if applicable
//  * place focused tablet according to:
//    * the focused tablet should be wholly visible, or if not, use all space
//    * the focused tablet should be as close to its old position as possible.
//    * if focused tablet was not wholly on screen, it goes to the side
//       corresponding to the direction of movement.
//  * if out of space or tablets, we're done
//  FIXME *maybe* just draw up followed by down, rather than direction of travel?
//  * walk the list in the direction of travel, foc->focw
//    * designate tablet against the walk as 'focagainst', might be NULL
//    * if focagainst || focagainst, focw only through edge, otherwise
//    * focw can be as large as all remaining space
//    * if there is space, draw what we can of next tablet
//    * move the focused tablet againt the direction of travel if necessary
//    * prefer the space in the direction of walking
//    * last tablet drawn is 'backstop'
//  * if out of space or tablets, we're done
//  * walk the list in the direction against travel, foc->focw
//    * if focw == backstop, we're done
//    * draw through edge
typedef struct ncreel {
  ncplane* p;           // ncplane this ncreel occupies, under tablets
  // doubly-linked list, a circular one when infinity scrolling is in effect.
  // points at the focused tablet (when at least one tablet exists, one must be
  // focused). it will be visibly focused following the next redraw.
  nctablet* tablets;
  nctablet* vft;        // the visibly-focused tablet
  direction_e direction;// last direction of travel
  int tabletcount;      // could be derived, but we keep it o(1)
  ncreel_options ropts; // copied in ncreel_create()
} ncreel;

// Returns the starting coordinates (relative to the screen) of the specified
// tablet, and its length. End is (begx + lenx - 1, begy + leny - 1).
static inline void
tablet_coordinates(ncplane* w, int* begy, int* begx, int* leny, int* lenx){
  ncplane_yx(w, begy, begx);
  ncplane_dim_yx(w, leny, lenx);
}

static int
draw_borders(ncplane* w, unsigned mask, uint64_t channel){
  int lenx, leny;
  int ret = 0;
  ncplane_dim_yx(w, &leny, &lenx);
  int maxx = lenx - 1;
  int maxy = leny - 1;
  cell ul, ur, ll, lr, hl, vl;
  cell_init(&ul); cell_init(&ur); cell_init(&hl);
  cell_init(&ll); cell_init(&lr); cell_init(&vl);
  if(cells_rounded_box(w, 0, channel, &ul, &ur, &ll, &lr, &hl, &vl)){
    return -1;
  }
//fprintf(stderr, "drawing borders %p ->%d/%d, mask: %04x\n", w, maxx, maxy, mask);
  // lenx is the number of columns we have, but drop 2 due to corners. we thus
  // want lenx horizontal lines.
  if(!(mask & NCBOXMASK_TOP)){
    ncplane_home(w);
    ncplane_putc(w, &ul);
    ncplane_hline(w, &hl, lenx - 2);
    ncplane_putc(w, &ur);
  }else{
    if(!(mask & NCBOXMASK_LEFT)){
      ncplane_home(w);
      ncplane_putc(w, &ul);
    }
    if(!(mask & NCBOXMASK_RIGHT)){
      ncplane_cursor_move_yx(w, 0, lenx - 1);
      ncplane_putc(w, &ur);
    }
  }
  int y;
  for(y = 1 ; y < maxy ; ++y){
    if(!(mask & NCBOXMASK_LEFT)){
      ret |= ncplane_cursor_move_yx(w, y, 0);
      ncplane_putc(w, &vl);
    }
    if(!(mask & NCBOXMASK_RIGHT)){
      ret |= ncplane_cursor_move_yx(w, y, maxx);
      ncplane_putc(w, &vl);
    }
  }
  if(!(mask & NCBOXMASK_BOTTOM)){
    ret |= ncplane_cursor_move_yx(w, maxy, 0);
    ncplane_putc(w, &ll);
    ncplane_hline(w, &hl, lenx - 2);
    ncplane_putc(w, &lr);
  }else{
    if(!(mask & NCBOXMASK_LEFT)){
      if(ncplane_cursor_move_yx(w, maxy, 0) || ncplane_putc(w, &ll) < 0){
        ret = -1;
      }
    }
    if(!(mask & NCBOXMASK_RIGHT)){
      // mvwadd_wch returns error if we print to the lowermost+rightmost
      // character cell. maybe we can make this go away with scrolling controls
      // at setup? until then, don't check for error here FIXME.
      if(ncplane_cursor_move_yx(w, maxy, maxx) || ncplane_putc(w, &lr) < 0){
        ret = -1;
      }
    }
  }
  cell_release(w, &ul); cell_release(w, &ur); cell_release(w, &hl);
  cell_release(w, &ll); cell_release(w, &lr); cell_release(w, &vl);
  return ret;
}

// Draws the border (if one should be drawn) around the ncreel, and enforces
// any provided restrictions on visible window size.
static int
draw_ncreel_borders(const ncreel* nr){
  int maxx, maxy;
  ncplane_dim_yx(nr->p, &maxy, &maxx);
  assert(maxy >= 0 && maxx >= 0);
  --maxx; // last column we can safely write to
  --maxy; // last line we can safely write to
  return draw_borders(nr->p, nr->ropts.bordermask, nr->ropts.borderchan);
}

// Calculate the starting and ending coordinates available for occupation by
// the tablet, relative to the ncreel's ncplane. Returns non-zero if the
// tablet cannot be made visible as specified. If this is the focused tablet
// (nr->tablets == t), it can take the entire reel -- frontiery is only a
// suggestion in this case -- so give it the full breadth.
static int
tablet_columns(const ncreel* nr, nctablet* t, int* begx, int* begy,
               int* lenx, int* leny, int frontiertop, int frontierbottom,
               direction_e direction){
fprintf(stderr, "jigsawing %p with %d/%d dir %d\n", t, frontiertop, frontierbottom, direction);
  *begy = 0;
  *begx = 0;
  ncplane_dim_yx(nr->p, leny, lenx);
  if(direction == DIRECTION_UP && frontiertop < 0){
    return -1;
  }else if(direction == DIRECTION_DOWN && frontierbottom >= *leny){
    return -1;
  }
  // account for the ncreel borders on the sides
  if(!(nr->ropts.bordermask & NCBOXMASK_LEFT)){
    ++*begx;
    --*lenx;
  }
  if(!(nr->ropts.bordermask & NCBOXMASK_RIGHT)){
    --*lenx;
  }
  // at this point, our coordinates describe the largest possible tablet for
  // this ncreel. this is the correct solution for the focused tablet. other
  // tablets can only grow in one of two directions, so tighten them up.
  if(nr->tablets != t){
    *leny -= (frontierbottom - frontiertop);
    if(direction == DIRECTION_DOWN){
      *begy = frontierbottom;
    }
  }
  return 0;
}

// Draw the specified tablet, if possible. DIRECTION_UP means we're laying out
// bottom-to-top. DIRECTION_DOWN means top-to-bottom. 'frontier{top, bottom}'
// are the lines to which we'll be fitting the tablet ('frontiertop' to our
// last row for DIRECTION_UP, and 'frontierbottom' to our first row for
// DIRECTION_DOWN). Gives the tablet all possible space to work with (i.e.
// everything beyond the frontiers, or the entire reel for the focused tablet).
// If the callback uses less space, shrinks the plane to that size.
static int
ncreel_draw_tablet(const ncreel* nr, nctablet* t, int frontiertop,
                   int frontierbottom, direction_e direction){
  assert(!t->p);
  int lenx, leny, begy, begx;
  if(tablet_columns(nr, t, &begx, &begy, &lenx, &leny, frontiertop, frontierbottom, direction)){
fprintf(stderr, "no room: %p base %d/%d len %d/%d dir %d\n", t, begy, begx, leny, lenx, direction);
    return -1;
  }
//fprintf(stderr, "tplacement: %p base %d/%d len %d/%d frontiery %d dir %d\n", t, begx, begy, lenx, leny, frontiery, direction);
  ncplane* fp = ncplane_bound(nr->p, leny, lenx, begy, begx, NULL);
  if((t->p = fp) == NULL){
    return -1;
  }
  // We pass the coordinates in which the callback may freely write. That's
  // the full width (minus tablet borders), and the full range of open space
  // in the direction we're moving. We're not passing *lengths* to the callback,
  // but *coordinates* within the window--everywhere save tabletborders.
  int cby = 0, cbx = 0, cbmaxy = leny, cbmaxx = lenx;
  // If we're drawing up, we'll always have a bottom border unless it's masked
  if((nr->tablets != t && direction == DIRECTION_UP) && !(nr->ropts.tabletmask & NCBOXMASK_BOTTOM)){
    --cbmaxy;
  }
  // If we're drawing down, we'll always have a top border unless it's masked
  if(direction == DIRECTION_DOWN && !(nr->ropts.tabletmask & NCBOXMASK_TOP)){
    ++cby;
  }
  // Adjust the x-bounds for side borders, which we always have if unmasked
  cbmaxx -= !(nr->ropts.tabletmask & NCBOXMASK_RIGHT);
  cbx += !(nr->ropts.tabletmask & NCBOXMASK_LEFT);
  bool cbdir = (nr->tablets != t && direction == DIRECTION_UP) ? true : false;
// fprintf(stderr, "calling! lenx/leny: %d/%d cbx/cby: %d/%d cbmaxx/cbmaxy: %d/%d dir: %d\n",
//    lenx, leny, cbx, cby, cbmaxx, cbmaxy, direction);
  int ll = t->cbfxn(t, cbdir);
//fprintf(stderr, "RETURNRETURNRETURN %p %d (%d, %d, %d) DIR %d\n", t, ll, cby, cbmaxy, leny, direction);
  if(ll != leny){
    if(ll == leny - 1){ // only has one border visible (partially off-screen)
      if(cbdir){
        ll += !(nr->ropts.tabletmask & NCBOXMASK_BOTTOM);
      }else{
        ll += !(nr->ropts.tabletmask & NCBOXMASK_TOP);
      }
      ncplane_resize_simple(fp, ll, lenx);
      if(direction == DIRECTION_UP && nr->tablets != t){
        ncplane_move_yx(fp, begy + leny - ll, begx);
//fprintf(stderr, "MOVEDOWN CLIPPED RESIZED (-1) from %d to %d\n", leny, ll);
//fprintf(stderr, "RESIZED (-1) from %d to %d\n", leny, ll);
      }
    }else if(ll < leny - 1){ // both borders are visible
      ll += !(nr->ropts.tabletmask & NCBOXMASK_BOTTOM) +
            !(nr->ropts.tabletmask & NCBOXMASK_TOP);
//fprintf(stderr, "RESIZING (-2) from %d to %d\n", leny, ll);
      ncplane_resize_simple(fp, ll, lenx);
      if(direction == DIRECTION_UP && nr->tablets != t){
//fprintf(stderr, "MOVEDOWN UNCLIPPED (skip %d)\n", leny - ll);
        ncplane_move_yx(fp, begy + leny - ll, begx);
      }
    }else{
      assert(ll == leny); // you fucked up! FIXME
    }
    // The focused tablet will have been resized properly above, but it might
    // be out of position (the focused tablet ought move as little as possible). 
    // Move it back to the frontier, or the nearest line above if it has grown.
    if(nr->tablets == t){
      if(leny - frontiertop + 1 < ll){
//fprintf(stderr, "frontieryIZING ADJ %d %d %d %d NEW %d\n", cbmaxy, leny, frontiery, ll, frontiery - ll + 1);
        ncplane_yx(fp, &frontiertop, NULL);
        frontiertop += (leny - ll);
      }
//fprintf(stderr, "moving to frontiery %d\n", frontiery);
      ncplane_move_yx(fp, frontiertop, begx);
    }
  }
  draw_borders(fp, nr->ropts.tabletmask,
               nr->tablets == t ? nr->ropts.focusedchan : nr->ropts.tabletchan);
  return 0;
}

// move down below the focused tablet, filling up the reel to the bottom.
// returns the last tablet drawn.
static nctablet*
draw_following_tablets(const ncreel* nr, nctablet* otherend,
                       int frontiertop, int* frontierbottom){
fprintf(stderr, "following otherend: %p ->p: %p %d/%d\n", otherend, otherend->p, frontiertop, *frontierbottom);
  nctablet* working = nr->tablets->next;
  const int maxx = ncplane_dim_y(nr->p) - 1;
  // move down past the focused tablet, filling up the reel to the bottom
  while(*frontierbottom <= maxx && (working != otherend || !otherend->p)){
//fprintf(stderr, "following otherend: %p ->p: %p\n", otherend, otherend->p);
    // modify frontier based off the one we're at
fprintf(stderr, "EASTBOUND AND DOWN: %p->%p %d %d\n", working, working->next, frontiertop, *frontierbottom);
    if(ncreel_draw_tablet(nr, working, frontiertop, *frontierbottom, DIRECTION_DOWN)){
      return NULL;
    }
    if(working == otherend){
      otherend = otherend->next;
    }
    *frontierbottom += ncplane_dim_y(working->p) + 2;
    working = working->next;
  }
fprintf(stderr, "done with prevs: %p->%p %d %d\n", working, working->prev, frontiertop, *frontierbottom);
  return working;
}

// move up above the focused tablet, filling up the reel to the top.
// returns the last tablet drawn.
static nctablet*
draw_previous_tablets(const ncreel* nr, nctablet* otherend,
                      int* frontiertop, int frontierbottom){
fprintf(stderr, "preceding otherend: %p ->p: %p\n", otherend, otherend->p);
  nctablet* upworking = nr->tablets->prev;
  // modify frontier based off the one we're at
  while(*frontiertop >= 0 && (upworking->prev != otherend || !otherend->p)){
fprintf(stderr, "MOVIN' ON UP: %p->%p %d %d\n", upworking, upworking->prev, *frontiertop, frontierbottom);
    if(ncreel_draw_tablet(nr, upworking, *frontiertop, frontierbottom, DIRECTION_UP)){
      return NULL;
    }
    if(upworking == otherend){
      otherend = otherend->prev;
    }
    *frontiertop -= ncplane_dim_y(upworking->p) + 1;
    upworking = upworking->prev;
  }
fprintf(stderr, "done with prevs: %p->%p %d %d\n", upworking, upworking->prev, *frontiertop, frontierbottom);
  return upworking;
}

static int
tighten_reel_down(ncreel* r, int ybot){
  nctablet* cur = r->tablets;
  while(cur){
    if(cur->p == NULL){
      break;
    }
    int cury, curx, ylen;
    ncplane_yx(cur->p, &cury, &curx);
    ncplane_dim_yx(cur->p, &ylen, NULL);
    if(cury == ybot - ylen){
      break;
    }
    cury = ybot - ylen;
    ncplane_move_yx(cur->p, cury, curx);
    ybot = cury - 1;
    if((cur = cur->prev) == r->tablets){
      break;
    }
  }
  return 0;
}

// run at the end of redraw, this aligns the top tablet with the top
// of the reel. we prefer empty space at the bottom (FIXME but not
// really -- we ought prefer space away from the last direction of
// movement. rather than this postprocessing, draw things to the
// right places!).
static int
tighten_reel(ncreel* r){
fprintf(stderr, "tightening it up\n");
  nctablet* top = r->tablets;
  nctablet* cur = top;
  int ytop = INT_MAX;
  while(cur){
    if(cur->p == NULL){
      break;
    }
    int cury;
    ncplane_yx(cur->p, &cury, NULL);
    if(cury >= ytop){
      break;
    }
    ytop = cury;
    top = cur;
    cur = cur->prev;
  }
  int expected = !(r->ropts.bordermask & NCBOXMASK_TOP);
  cur = top;
  while(cur){
    if(cur->p == NULL){
      break;
    }
    int cury, curx;
    ncplane_yx(cur->p, &cury, &curx);
    if(cury != expected){
      if(ncplane_move_yx(cur->p, expected, curx)){
        return -1;
      }
    }else{
      break;
    }
    int ylen;
    ncplane_dim_yx(cur->p, &ylen, NULL);
    expected += ylen + 1;
    cur = cur->next;
    if(cur == top){
      break;
    }
  }
  cur = r->tablets;
  if(cur){
    const ncplane* n = cur->p;
    int yoff, ylen, rylen;
    ncplane_dim_yx(r->p, &rylen, NULL);
    ncplane_yx(n, &yoff, NULL);
    ncplane_dim_yx(n, &ylen, NULL);
    const int ybot = rylen - 1 - !!(r->ropts.bordermask & NCBOXMASK_BOTTOM);
    // FIXME want to tighten down whenever we're at the bottom, and the reel
    // is full, not just in this case (this can leave a gap of more than 1 row)
    if(yoff + ylen + 1 >= ybot){
      return tighten_reel_down(r, ybot);
    }
  }
  return 0;
}

// destroy all existing tablet planes pursuant to redraw
static void
clean_reel(ncreel* r){
  nctablet* vft = r->vft;
  if(vft){
    for(nctablet* n = vft->next ; n->p && n != vft ; n = n->next){
//fprintf(stderr, "CLEANING NEXT: %p (%p)\n", n, n->p);
      ncplane_destroy(n->p);
      n->p = NULL;
    }
    for(nctablet* n = vft->prev ; n->p && n != vft ; n = n->prev){
//fprintf(stderr, "CLEANING PREV: %p (%p)\n", n, n->p);
      ncplane_destroy(n->p);
      n->p = NULL;
    }
//fprintf(stderr, "CLEANING VFT: %p (%p)\n", vft, vft->p);
    ncplane_destroy(vft->p);
    vft->p = NULL;
  }
}

// Arrange the panels, starting with the focused window, wherever it may be.
// If necessary, resize it to the full size of the reel--focus has its
// privileges. We then work in the opposite direction of travel, filling out
// the reel above and below. If we moved down to get here, do the tablets above
// first. If we moved up, do the tablets below. This ensures tablets stay in
// place relative to the new focus; they could otherwise pivot around the new
// focus, if we're not filling out the reel.
int ncreel_redraw(ncreel* nr){
//fprintf(stderr, "--------> BEGIN REDRAW <--------\n");
  nctablet* focused = nr->tablets;
  int fulcrum; // target line
  if(nr->direction == DIRECTION_UP){
    // case i iff the last direction was UP, and either the focused tablet is
    // not visible, or below the visibly-focused tablet, or there is no
    // visibly-focused tablet.
    if((focused && !focused->p) || !nr->vft){
      fulcrum = 0;
    }else{
      int focy, vfty;
      ncplane_yx(focused->p, &focy, NULL);
      ncplane_yx(nr->vft->p, &vfty, NULL);
      if(focy > vfty){
        fulcrum = 0;
      }else{
        ncplane_yx(focused->p, &fulcrum, NULL); // case iii
      }
    }
  }else{
    // case ii iff the last direction was DOWN, and either the focused tablet
    // is not visible, or above the visibly-focused tablet, or there is no
    // visibly-focused tablet.
    if((focused && !focused->p) || !nr->vft){
      fulcrum = ncplane_dim_y(nr->p) - 1;
    }else{
      int focy, vfty;
      ncplane_yx(focused->p, &focy, NULL);
      ncplane_yx(nr->vft->p, &vfty, NULL);
      if(focy < vfty){
        fulcrum = ncplane_dim_y(nr->p) - 1;
      }else{
        ncplane_yx(focused->p, &fulcrum, NULL); // case iii
      }
    }
  }
  clean_reel(nr);
  if(focused){
fprintf(stderr, "drawing focused tablet %p dir: %d fulcrum: %d!\n", focused, nr->direction, fulcrum);
    if(ncreel_draw_tablet(nr, focused, fulcrum, fulcrum, DIRECTION_DOWN) == 0){
fprintf(stderr, "drew focused tablet %p -> %p dir: %d!\n", focused, focused->p, nr->direction);
      nctablet* otherend = focused;
      int frontiertop, frontierbottom;
      ncplane_yx(nr->tablets->p, &frontiertop, NULL);
      frontierbottom = frontiertop + ncplane_dim_y(nr->tablets->p) + 1;
      frontiertop -= 2;
      if(nr->direction == DIRECTION_DOWN){
        otherend = draw_previous_tablets(nr, otherend, &frontiertop, frontierbottom);
        if(otherend == NULL){
          return -1;
        }
        otherend = draw_following_tablets(nr, otherend, frontiertop, &frontierbottom);
      }else{ // DIRECTION_UP
        otherend = draw_following_tablets(nr, otherend, frontiertop, &frontierbottom);
        if(otherend == NULL){
          return -1;
        }
        otherend = draw_previous_tablets(nr, otherend, &frontiertop, frontierbottom);
      }
      if(otherend == NULL){
        return -1;
      }
      tighten_reel(nr);
fprintf(stderr, "done tightening\n");
    }
  }
  nr->vft = nr->tablets; // update the visually-focused tablet pointer
//fprintf(stderr, "DONE ARRANGING\n");
  if(draw_ncreel_borders(nr)){
    return -1; // enforces specified dimensional minima
  }
  return 0;
}

static bool
validate_ncreel_opts(ncplane* w, const ncreel_options* ropts){
  if(w == NULL){
    return false;
  }
  if(ropts->flags & NCREEL_OPTION_CIRCULAR){
    if(!(ropts->flags & NCREEL_OPTION_INFINITESCROLL)){
      return false; // can't set circular without infinitescroll
    }
  }
  // there exist higher NCBOX defines, but they're not valid in this context
  const unsigned fullmask = NCBOXMASK_LEFT |
                            NCBOXMASK_RIGHT |
                            NCBOXMASK_TOP |
                            NCBOXMASK_BOTTOM;
  if(ropts->bordermask > fullmask){
    return false;
  }
  if(ropts->tabletmask > fullmask){
    return false;
  }
  return true;
}

ncplane* nctablet_ncplane(nctablet* t){
  return t->p;
}

ncplane* ncreel_plane(ncreel* nr){
  return nr->p;
}

ncreel* ncreel_create(ncplane* w, const ncreel_options* ropts){
  ncreel* nr;

  if(!validate_ncreel_opts(w, ropts)){
    return NULL;
  }
  if((nr = malloc(sizeof(*nr))) == NULL){
    return NULL;
  }
  nr->tablets = NULL;
  nr->tabletcount = 0;
  nr->direction = DIRECTION_DOWN; // draw down after the initial tablet
  memcpy(&nr->ropts, ropts, sizeof(*ropts));
  nr->p = w;
  nr->vft = NULL;
  ncplane_set_base(nr->p, "", 0, ropts->bgchannel);
  if(ncreel_redraw(nr)){
    ncplane_destroy(nr->p);
    free(nr);
    return NULL;
  }
  return nr;
}

nctablet* ncreel_add(ncreel* nr, nctablet* after, nctablet *before,
                     tabletcb cbfxn, void* opaque){
  nctablet* t;
  if(after && before){
    if(after->prev != before || before->next != after){
      logerror(nr->p->nc, "bad before (%p) / after (%p) spec\n", before, after);
      return NULL;
    }
  }else if(!after && !before){
    // This way, without user interaction or any specification, new tablets are
    // inserted at the "end" relative to the focus. The first one to be added
    // gets and keeps the focus. New ones will go on the bottom, until we run
    // out of space. New tablets are then created off-screen.
    before = nr->tablets;
  }
  if((t = malloc(sizeof(*t))) == NULL){
    return NULL;
  }
//fprintf(stderr, "--------->NEW TABLET %p\n", t);
  if(after){
    t->next = after->next;
    after->next = t;
    t->prev = after;
    t->next->prev = t;
  }else if(before){
    t->prev = before->prev;
    before->prev = t;
    t->next = before;
    t->prev->next = t;
  }else{ // we're the first tablet
    t->prev = t->next = t;
    nr->tablets = t;
  }
  t->cbfxn = cbfxn;
  t->curry = opaque;
  ++nr->tabletcount;
  t->p = NULL;
  return t;
}

int ncreel_del(ncreel* nr, struct nctablet* t){
  if(nr == NULL || t == NULL){
    return -1;
  }
  t->prev->next = t->next;
  if(nr->tablets == t){
    if((nr->tablets = t->next) == t){
      nr->tablets = NULL;
    }
    // FIXME ought set direction based on actual location of replacement t
    nr->direction = DIRECTION_DOWN;
  }
  t->next->prev = t->prev;
  if(t->p){
    ncplane_destroy(t->p);
  }
  free(t);
  --nr->tabletcount;
  return 0;
}

int ncreel_destroy(ncreel* nreel){
  int ret = 0;
  if(nreel){
    nctablet* t;
    while( (t = nreel->tablets) ){
      ncreel_del(nreel, t);
    }
    ncplane_destroy(nreel->p);
    free(nreel);
  }
  return ret;
}

void* nctablet_userptr(nctablet* t){
  return t->curry;
}

int ncreel_tabletcount(const ncreel* nreel){
  return nreel->tabletcount;
}

nctablet* ncreel_focused(ncreel* nr){
  return nr->tablets;
}

nctablet* ncreel_next(ncreel* nr){
  if(nr->tablets){
    nr->tablets = nr->tablets->next;
//fprintf(stderr, "---------------> moved to next, %p to %p <----------\n",
//        nr->tablets->prev, nr->tablets);
    nr->direction = DIRECTION_DOWN;
  }
  return nr->tablets;
}

nctablet* ncreel_prev(ncreel* nr){
  if(nr->tablets){
    nr->tablets = nr->tablets->prev;
//fprintf(stderr, "----------------> moved to prev, %p to %p <----------\n",
//        nr->tablets->next, nr->tablets);
    nr->direction = DIRECTION_UP;
  }
  return nr->tablets;
}

bool ncreel_offer_input(ncreel* n, const ncinput* nc){
  if(nc->id == NCKEY_UP){
    ncreel_prev(n);
    return true;
  }else if(nc->id == NCKEY_DOWN){
    ncreel_next(n);
    return true;
  }else if(nc->id == NCKEY_SCROLL_UP){
    ncreel_prev(n);
    return true;
  }else if(nc->id == NCKEY_SCROLL_DOWN){
    ncreel_next(n);
    return true;
  }
  // FIXME page up/page down
  // FIXME end/home (when not using infinite scrolling)
  return false;
}
