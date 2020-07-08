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
  ncplane* border;
  struct nctablet* next;
  struct nctablet* prev;
  tabletcb cbfxn;              // application callback to draw tablet
  void* curry;                 // application data provided to cbfxn
} nctablet;

// The visible screen can be reconstructed from four things:
//  * which tablet is focused (pointed at by tablets)
//  * which row the focused tablet starts at (derived from focused window)
//  * the list of tablets (available from the focused tablet)
//  * from which direction we arrived at the focused window
// Things which can happen between ncreel_redraw() calls:
//  * new focused tablet added (only when no tablets exist)
//  * new unfocused tablet added
//  * tablet removed (may be focused)
//  * tablets may grow or shrink
//  * focus can change
// On tablet remove:
//  * destroy plane, remove from list
//  * if tablet was focused, change focus
// On tablet add:
//  * add to list (do not create plane)
//  * if no tablet existed, change focus to new tablet
// On focus change:
//  * change focus, update travel direction
// On redraw:
//  * if no tablets, we're done (deleted planes are already gone)
//  * resize focused tablet to maximum
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
  ncreel_options ropts; // copied in ncreel_create()
  // doubly-linked list, a circular one when infinity scrolling is in effect.
  // points at the focused tablet (when at least one tablet exists, one must be
  // focused), which might be anywhere on the screen (but is always visible).
  nctablet* tablets;
  // these values could all be derived at any time, but keeping them computed
  // makes other things easier, or saves us time (at the cost of complexity).
  int tabletcount;         // could be derived, but we keep it o(1)
  // last direction in which we moved. positive if we moved down ("next"),
  // negative if we moved up ("prev"), 0 for non-linear operation. we start
  // drawing unfocused tablets opposite the direction of our last movement, so
  // that movement in an unfilled reel doesn't reorient our tablets.
  int last_traveled_direction;
} ncreel;

// Returns the starting coordinates (relative to the screen) of the specified
// tablet, and its length. End is (begx + lenx - 1, begy + leny - 1).
static inline void
tablet_coordinates(ncplane* w, int* begy, int* begx, int* leny, int* lenx){
  ncplane_yx(w, begy, begx);
  ncplane_dim_yx(w, leny, lenx);
}

// FIXME compatability wrapper for libpanel
int wresize(ncplane* n, int leny, int lenx){
assert(leny > 0);
assert(lenx > 0);
  int dimy, dimx;
  ncplane_dim_yx(n, &dimy, &dimx);
  int keepy = dimy > leny ? leny : dimy;
  return ncplane_resize(n, 0, 0, keepy, dimx, 0, 0, leny, lenx);
}

// bchrs: 6-element array of wide border characters + attributes FIXME
static int
draw_borders(ncplane* w, unsigned mask, uint64_t channel,
             bool cliphead, bool clipfoot){
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
/*fprintf(stderr, "drawing borders %p ->%d/%d, mask: %04x, clipping: %c%c\n",
        w, maxx, maxy, mask,
        cliphead ? 'T' : 't', clipfoot ? 'F' : 'f');*/
  if(!cliphead){
    // lenx is the number of columns we have, but drop 2 due to
    // corners. we thus want lenx horizontal lines.
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
  }
  int y;
  for(y = !cliphead ; y < maxy + !!clipfoot ; ++y){
    if(!(mask & NCBOXMASK_LEFT)){
      ret |= ncplane_cursor_move_yx(w, y, 0);
      ncplane_putc(w, &vl);
    }
    if(!(mask & NCBOXMASK_RIGHT)){
      ret |= ncplane_cursor_move_yx(w, y, maxx);
      ncplane_putc(w, &vl);
    }
  }
  if(!clipfoot){
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
  }
  cell_release(w, &ul); cell_release(w, &ur); cell_release(w, &hl);
  cell_release(w, &ll); cell_release(w, &lr); cell_release(w, &vl);
// fprintf(stderr, "||--borders %d %d clip: %c%c ret: %d\n",
//    maxx, maxy, cliphead ? 'y' : 'n', clipfoot ? 'y' : 'n', ret);
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
  return draw_borders(nr->p, nr->ropts.bordermask, nr->ropts.borderchan, false, false);
}

// Calculate the starting and ending coordinates available for occupation by
// the tablet, relative to the ncreel's ncplane. Returns non-zero if the
// tablet cannot be made visible as specified. If this is the focused tablet
// (direction == 0), it can take the entire reel -- frontiery is only a
// suggestion in this case -- so give it the full breadth.
static int
tablet_columns(const ncreel* nr, int* begx, int* begy, int* lenx, int* leny,
               int frontiery, int direction){
  *begy = 0;
  *begx = 0;
  ncplane_dim_yx(nr->p, leny, lenx);
  int maxy = *leny + *begy - 1;
  int begindraw = *begy + !(nr->ropts.bordermask & NCBOXMASK_TOP);
  int enddraw = maxy - !(nr->ropts.bordermask & NCBOXMASK_TOP);
  if(direction <= 0){
    if(frontiery < begindraw){
      return -1;
    }
  }else{
    if(frontiery > enddraw){
  // fprintf(stderr, "FRONTIER: %d ENDDRAW: %d\n", frontiery, enddraw);
      return -1;
    }
  }
  // account for the ncreel borders
  if(direction <= 0 && !(nr->ropts.bordermask & NCBOXMASK_TOP)){
    ++*begy;
    --*leny;
  }
  if(direction >= 0 && !(nr->ropts.bordermask & NCBOXMASK_BOTTOM)){
    --*leny;
  }
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
  if(direction > 0){
    *leny -= (frontiery - *begy);
    *begy = frontiery;
  }else if(direction < 0){
    *leny = frontiery - *begy + 1;
  }
  return 0;
}

// Draw the specified tablet, if possible. A direction less than 0 means we're
// laying out towards the top. Greater than zero means towards the bottom. 0
// means this is the focused tablet, always the first one to be drawn.
// frontiery is the line on which we're placing the tablet (in the case of the
// focused window, this is only an ideal, subject to change). For direction
// greater than or equal to 0, it's the top line of the tablet. For direction
// less than 0, it's the bottom line. Gives the tablet all possible space to
// work with (i.e. up to the edge we're approaching, or the entire panel for
// the focused tablet). If the callback uses less space, shrinks the panel back
// down before displaying it. Destroys any panel if it ought be hidden.
// Returns 0 if the tablet was able to be wholly rendered, non-zero otherwise.
static int
ncreel_draw_tablet(const ncreel* nr, nctablet* t, int frontiery,
                      int direction){
  int lenx, leny, begy, begx;
  ncplane* fp = t->p;
  if(tablet_columns(nr, &begx, &begy, &lenx, &leny, frontiery, direction)){
//fprintf(stderr, "no room: %p:%p base %d/%d len %d/%d dir %d\n", t, fp, begy, begx, leny, lenx, direction);
//fprintf(stderr, "FRONTIER DONE!!!!!!\n");
    if(fp){
//fprintf(stderr, "HIDING %p at frontier %d (dir %d) with %d\n", t, frontiery, direction, leny);
      ncplane_destroy(fp);
      t->p = NULL;
    }
    return -1;
  }
//fprintf(stderr, "tplacement: %p:%p base %d/%d len %d/%d\n", t, fp, begx, begy, lenx, leny);
//fprintf(stderr, "DRAWING %p at frontier %d (dir %d) with %d\n", t, frontiery, direction, leny);
  if(fp == NULL){ // create a panel for the tablet
    t->p = ncplane_bound(nr->p, leny + 1, lenx, begy, begx, NULL);
    if((fp = t->p) == NULL){
      return -1;
    }
  }else{
    int trueby, truebx;
    ncplane_yx(fp, &trueby, &truebx);
    int truey, truex;
    ncplane_dim_yx(fp, &truey, &truex);
    if(truey != leny){
//fprintf(stderr, "RESIZE TRUEY: %d BEGY: %d LENY: %d\n", truey, begy, leny);
      if(wresize(fp, leny, truex)){
        return -1;
      }
      truey = leny;
    }
    if(begy != trueby){
      ncplane_move_yx(fp, begy, begx);
    }
  }
  if(wresize(fp, leny, lenx)){
    return -1;
  }
  bool cliphead = false;
  bool clipfoot = false;
  // We pass the coordinates in which the callback may freely write. That's
  // the full width (minus tablet borders), and the full range of open space
  // in the direction we're moving. We're not passing *lenghts* to the callback,
  // but *coordinates* within the window--everywhere save tabletborders.
  int cby = 0, cbx = 0, cbmaxy = leny, cbmaxx = lenx;
  --cbmaxy;
  --cbmaxx;
  // If we're drawing up, we'll always have a bottom border unless it's masked
  if(direction < 0 && !(nr->ropts.tabletmask & NCBOXMASK_BOTTOM)){
    --cbmaxy;
  }
  // If we're drawing down, we'll always have a top border unless it's masked
  if(direction >= 0 && !(nr->ropts.tabletmask & NCBOXMASK_TOP)){
    ++cby;
  }
  // Adjust the x-bounds for side borders, which we always have if unmasked
  cbmaxx -= !(nr->ropts.tabletmask & NCBOXMASK_RIGHT);
  cbx += !(nr->ropts.tabletmask & NCBOXMASK_LEFT);
  bool cbdir = direction < 0 ? true : false;
// fprintf(stderr, "calling! lenx/leny: %d/%d cbx/cby: %d/%d cbmaxx/cbmaxy: %d/%d dir: %d\n",
//    lenx, leny, cbx, cby, cbmaxx, cbmaxy, direction);
  int ll = t->cbfxn(t, cbx, cby, cbmaxx, cbmaxy, cbdir);
//fprintf(stderr, "RETURNRETURNRETURN %p %d (%d, %d, %d) DIR %d\n",
//        t, ll, cby, cbmaxy, leny, direction);
  if(ll != leny){
    if(ll == leny - 1){ // only has one border visible (partially off-screen)
      if(cbdir){
        ll += !(nr->ropts.tabletmask & NCBOXMASK_BOTTOM);
      }else{
        ll += !(nr->ropts.tabletmask & NCBOXMASK_TOP);
      }
      wresize(fp, ll, lenx);
      if(direction < 0){
        cliphead = true;
        ncplane_move_yx(fp, begy + leny - ll, begx);
//fprintf(stderr, "MOVEDOWN CLIPPED RESIZED (-1) from %d to %d\n", leny, ll);
      }else{
        clipfoot = true;
//fprintf(stderr, "RESIZED (-1) from %d to %d\n", leny, ll);
      }
    }else if(ll < leny - 1){ // both borders are visible
      ll += !(nr->ropts.tabletmask & NCBOXMASK_BOTTOM) +
            !(nr->ropts.tabletmask & NCBOXMASK_TOP);
//fprintf(stderr, "RESIZING (-2) from %d to %d\n", leny, ll);
      wresize(fp, ll, lenx);
      if(direction < 0){
//fprintf(stderr, "MOVEDOWN UNCLIPPED (skip %d)\n", leny - ll);
        ncplane_move_yx(fp, begy + leny - ll, begx);
      }
    }else{
      assert(ll == leny); // you fucked up! FIXME
    }
    // The focused tablet will have been resized properly above, but it might
    // be out of position (the focused tablet ought move as little as possible). 
    // Move it back to the frontier, or the nearest line above if it has grown.
    if(direction == 0){
      if(leny - frontiery + 1 < ll){
//fprintf(stderr, "frontieryIZING ADJ %d %d %d %d NEW %d\n", cbmaxy, leny,
//         frontiery, ll, frontiery - ll + 1);
        ncplane_yx(nr->p, &frontiery, NULL);
        frontiery += (leny - ll);
      }
      ncplane_move_yx(fp, frontiery, begx);
    }
  }
  draw_borders(fp, nr->ropts.tabletmask,
               direction == 0 ? nr->ropts.focusedchan : nr->ropts.tabletchan,
               cliphead, clipfoot);
  return cliphead || clipfoot;
}

// draw and size the focused tablet, which must exist (nr->tablets may not be
// NULL). it can occupy the entire ncreel.
static int
draw_focused_tablet(const ncreel* nr){
  int pbegy, pbegx, plenx, pleny; // ncreel window coordinates
  tablet_coordinates(nr->p, &pbegy, &pbegx, &pleny, &plenx);
  int fulcrum;
  if(nr->tablets->p == NULL){
    if(nr->last_traveled_direction >= 0){
      fulcrum = pleny + pbegy - !(nr->ropts.bordermask & NCBOXMASK_BOTTOM);
    }else{
      fulcrum = pbegy + !(nr->ropts.bordermask & NCBOXMASK_TOP);
    }
  }else{ // focused was already present. want to stay where we are, if possible
    ncplane_yx(nr->tablets->p, &fulcrum, NULL);
    // FIXME ugh can't we just remember the previous fulcrum?
    if(nr->last_traveled_direction > 0){
      if(nr->tablets->prev->p){
        int prevfulcrum;
        ncplane_yx(nr->tablets->prev->p, &prevfulcrum, NULL);
        if(fulcrum < prevfulcrum){
          fulcrum = pleny + pbegy - !(nr->ropts.bordermask & NCBOXMASK_BOTTOM);
        }
      }
    }else if(nr->last_traveled_direction < 0){
      if(nr->tablets->next->p){
        int nextfulcrum;
        ncplane_yx(nr->tablets->next->p, &nextfulcrum, NULL);
        if(fulcrum > nextfulcrum){
          fulcrum = pbegy + !(nr->ropts.bordermask & NCBOXMASK_TOP);
        }
      }
    }
  }
//fprintf(stderr, "PR dims: %d/%d + %d/%d fulcrum: %d\n", pbegy, pbegx, pleny, plenx, fulcrum);
  ncreel_draw_tablet(nr, nr->tablets, fulcrum, 0 /* nr->last_traveled_direction*/);
  return 0;
}

// move down below the focused tablet, filling up the reel to the bottom.
// returns the last tablet drawn.
static nctablet*
draw_following_tablets(const ncreel* nr, const nctablet* otherend){
  int wmaxy, wbegy, wbegx, wlenx, wleny; // working tablet window coordinates
  nctablet* working = nr->tablets;
  int frontiery;
  // move down past the focused tablet, filling up the reel to the bottom
  do{
//fprintf(stderr, "following otherend: %p ->p: %p\n", otherend, otherend->p);
    // modify frontier based off the one we're at
    tablet_coordinates(working->p, &wbegy, &wbegx, &wleny, &wlenx);
    wmaxy = wbegy + wleny - 1;
    frontiery = wmaxy + 2;
//fprintf(stderr, "EASTBOUND AND DOWN: %p->%p %d %d\n", working, working->next, frontiery, wmaxy + 2);
    working = working->next;
    if(working == otherend && otherend->p){
//fprintf(stderr, "BREAKOUT ON OTHEREND %p:%p\n", working, working->p);
      break;
    }
    ncreel_draw_tablet(nr, working, frontiery, 1);
    if(working == otherend){
      otherend = otherend->next;
    }
  }while(working->p); // FIXME might be more to hide
  // FIXME keep going forward, hiding those no longer visible
  return working;
}

// move up above the focused tablet, filling up the reel to the top.
// returns the last tablet drawn.
static nctablet*
draw_previous_tablets(const ncreel* nr, const nctablet* otherend){
//fprintf(stderr, "preceding otherend: %p ->p: %p\n", otherend, otherend->p);
  int wbegy, wbegx, wlenx, wleny; // working tablet window coordinates
  nctablet* upworking = nr->tablets;
  int frontiery;
  // modify frontier based off the one we're at
  tablet_coordinates(upworking->p, &wbegy, &wbegx, &wleny, &wlenx);
  frontiery = wbegy - 2;
  while(upworking->prev != otherend || otherend->p == NULL){
//fprintf(stderr, "MOVIN' ON UP: %p->%p %d %d\n", upworking, upworking->prev, frontiery, wbegy - 2);
    upworking = upworking->prev;
    ncreel_draw_tablet(nr, upworking, frontiery, -1);
    if(upworking->p){
      tablet_coordinates(upworking->p, &wbegy, &wbegx, &wleny, &wlenx);
//fprintf(stderr, "new up coords: %d/%d + %d/%d, %d\n", wbegy, wbegx, wleny, wlenx, frontiery);
      frontiery = wbegy - 2;
    }else{
      break;
    }
    if(upworking == otherend){
      otherend = otherend->prev;
    }
  }
  // FIXME keep going backwards, hiding those no longer visible
  return upworking;
}

// run at the end of redraw, this aligns the top tablet with the top
// of the reel. we prefer empty space at the bottom (FIXME but not
// really -- we ought prefer space away from the last direction of
// movement. rather than this postprocessing, draw things to the
// right places!).
static int
tighten_reel(ncreel* r){
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
  return 0;
}

// Arrange the panels, starting with the focused window, wherever it may be.
// If necessary, resize it to the full size of the reel--focus has its
// privileges. We then work in the opposite direction of travel, filling out
// the reel above and below. If we moved down to get here, do the tablets above
// first. If we moved up, do the tablets below. This ensures tablets stay in
// place relative to the new focus; they could otherwise pivot around the new
// focus, if we're not filling out the reel.
//
// This can still leave a gap plus a partially-onscreen tablet FIXME
int ncreel_redraw(ncreel* nr){
//fprintf(stderr, "--------> BEGIN REDRAW <--------\n");
  if(draw_ncreel_borders(nr)){
    return -1; // enforces specified dimensional minima
  }
  nctablet* focused = nr->tablets;
  if(focused == NULL){
//fprintf(stderr, "no focus!\n");
    return 0; // if none are focused, none exist
  }
//fprintf(stderr, "focused %p!\n", focused);
//fprintf(stderr, "drawing focused tablet %p dir: %d!\n", focused, nr->last_traveled_direction);
  draw_focused_tablet(nr);
//fprintf(stderr, "drew focused tablet %p dir: %d!\n", focused, nr->last_traveled_direction);
  nctablet* otherend = focused;
  if(nr->last_traveled_direction >= 0){
    otherend = draw_previous_tablets(nr, otherend);
    otherend = draw_following_tablets(nr, otherend);
    draw_previous_tablets(nr, otherend);
  }else{
    otherend = draw_following_tablets(nr, otherend);
    otherend = draw_previous_tablets(nr, otherend);
    draw_following_tablets(nr, otherend);
  }
  tighten_reel(nr);
//fprintf(stderr, "DONE ARRANGING\n");
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
  nr->last_traveled_direction = -1; // draw down after the initial tablet
  memcpy(&nr->ropts, ropts, sizeof(*ropts));
  nr->p = w;
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
    nr->last_traveled_direction = 1;
  }
  return nr->tablets;
}

nctablet* ncreel_prev(ncreel* nr){
  if(nr->tablets){
    nr->tablets = nr->tablets->prev;
//fprintf(stderr, "----------------> moved to prev, %p to %p <----------\n",
//        nr->tablets->next, nr->tablets);
    nr->last_traveled_direction = -1;
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
  // FIXME there are a few more
  return false;
}
