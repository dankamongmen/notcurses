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

// The visible screen can be reconstructed from three things:
//  * which tablet is focused (pointed at by tablets)
//  * which row the focused tablet starts at (derived from focused window)
//  * the list of tablets (available from the focused tablet)
typedef struct ncreel {
  ncplane* p;              // ncplane this ncreel occupies, under tablets
  ncreel_options ropts; // copied in ncreel_create()
  int efd;                 // eventfd/pipe, signaled in ncreel_touch()
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
  // are all of our tablets currently visible? our arrangement algorithm works
  // differently when the reel is not completely filled. ideally we'd unite the
  // two modes, but for now, check this bool and take one of two paths.
  bool all_visible;
} ncreel;

// Returns the starting coordinates (relative to the screen) of the specified
// window, and its length. End is (begx + lenx - 1, begy + leny - 1).
static inline void
window_coordinates(ncplane* w, int* begy, int* begx, int* leny, int* lenx){
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
  int begx, begy, lenx, leny;
  int ret = 0;
  window_coordinates(w, &begy, &begx, &leny, &lenx);
  begx = 0;
  begy = 0;
  int maxx = begx + lenx - 1;
  int maxy = begy + leny - 1;
  cell ul, ur, ll, lr, hl, vl;
  cell_init(&ul); cell_init(&ur); cell_init(&hl);
  cell_init(&ll); cell_init(&lr); cell_init(&vl);
  if(cells_rounded_box(w, 0, channel, &ul, &ur, &ll, &lr, &hl, &vl)){
    return -1;
  }
/*fprintf(stderr, "drawing borders %p %d/%d->%d/%d, mask: %04x, clipping: %c%c\n",
        w, begx, begy, maxx, maxy, mask,
        cliphead ? 'T' : 't', clipfoot ? 'F' : 'f');*/
  if(!cliphead){
    // lenx - begx + 1 is the number of columns we have, but drop 2 due to
    // corners. we thus want lenx - begx - 1 horizontal lines.
    if(!(mask & NCBOXMASK_TOP)){
      ret |= ncplane_cursor_move_yx(w, begy, begx);
      ncplane_putc(w, &ul);
      ncplane_hline(w, &hl, lenx - 2);
      ncplane_putc(w, &ur);
    }else{
      if(!(mask & NCBOXMASK_LEFT)){
        ret |= ncplane_cursor_move_yx(w, begy, begx);
        ncplane_putc(w, &ul);
      }
      if(!(mask & NCBOXMASK_RIGHT)){
        ret |= ncplane_cursor_move_yx(w, begy, maxx);
        ncplane_putc(w, &ur);
      }
    }
  }
  int y;
  for(y = begy + !cliphead ; y < maxy + !!clipfoot ; ++y){
    if(!(mask & NCBOXMASK_LEFT)){
      ret |= ncplane_cursor_move_yx(w, y, begx);
      ncplane_putc(w, &vl);
    }
    if(!(mask & NCBOXMASK_RIGHT)){
      ret |= ncplane_cursor_move_yx(w, y, maxx);
      ncplane_putc(w, &vl);
    }
  }
  if(!clipfoot){
    if(!(mask & NCBOXMASK_BOTTOM)){
      ret |= ncplane_cursor_move_yx(w, maxy, begx);
      ncplane_putc(w, &ll);
      ncplane_hline(w, &hl, lenx - 2);
      ncplane_putc(w, &lr);
    }else{
      if(!(mask & NCBOXMASK_LEFT)){
        if(ncplane_cursor_move_yx(w, maxy, begx) || ncplane_putc(w, &ll) < 0){
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
// fprintf(stderr, "||--borders %d %d %d %d clip: %c%c ret: %d\n",
//    begx, begy, maxx, maxy, cliphead ? 'y' : 'n', clipfoot ? 'y' : 'n', ret);
  return ret;
}

// Draws the border (if one should be drawn) around the ncreel, and enforces
// any provided restrictions on visible window size.
static int
draw_ncreel_borders(const ncreel* nr){
  int begx, begy;
  int maxx, maxy;
  window_coordinates(nr->p, &begy, &begx, &maxy, &maxx);
  assert(maxy >= 0 && maxx >= 0);
  --maxx; // last column we can safely write to
  --maxy; // last line we can safely write to
  if(begx >= maxx || maxx - begx + 1 < nr->ropts.min_supported_rows){
    return 0; // no room
  }
  if(begy >= maxy || maxy - begy + 1 < nr->ropts.min_supported_cols){
    return 0; // no room
  }
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
  window_coordinates(nr->p, begy, begx, leny, lenx);
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
    t->p = ncplane_new(nr->p->nc, leny + 1, lenx, begy, begx, NULL);
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
  window_coordinates(nr->p, &pbegy, &pbegx, &pleny, &plenx);
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
    window_coordinates(working->p, &wbegy, &wbegx, &wleny, &wlenx);
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
  window_coordinates(upworking->p, &wbegy, &wbegx, &wleny, &wlenx);
  frontiery = wbegy - 2;
  while(upworking->prev != otherend || otherend->p == NULL){
//fprintf(stderr, "MOVIN' ON UP: %p->%p %d %d\n", upworking, upworking->prev, frontiery, wbegy - 2);
    upworking = upworking->prev;
    ncreel_draw_tablet(nr, upworking, frontiery, -1);
    if(upworking->p){
      window_coordinates(upworking->p, &wbegy, &wbegx, &wleny, &wlenx);
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

// all tablets must be visible (valid ->p), and at least one tablet must exist
static nctablet*
find_topmost(ncreel* nr){
  nctablet* t = nr->tablets;
  int curline;
  ncplane_yx(t->p, &curline, NULL);
  int trialline;
  ncplane_yx(t->prev->p, &trialline, NULL);
  while(trialline < curline){
    t = t->prev;
    curline = trialline;
    ncplane_yx(t->prev->p, &trialline, NULL);
  }
// fprintf(stderr, "topmost: %p @ %d\n", t, curline);
  return t;
}

// all the tablets are believed to be wholly visible. in this case, we only want
// to fill up the necessary top rows, even if it means moving everything up at
// the end. large gaps should always be at the bottom to avoid ui discontinuity.
// this must only be called if we actually have at least one tablet. note that
// as a result of this function, we might not longer all be wholly visible.
// good god almighty, this is some fucking garbage.
static int
ncreel_arrange_denormalized(ncreel* nr){
//fprintf(stderr, "denormalized devolution (are we men?)\n");
  // we'll need the starting line of the tablet which just lost focus, and the
  // starting line of the tablet which just gained focus.
  int fromline, nowline;
  ncplane_yx(nr->tablets->p, &nowline, NULL);
  // we've moved to the next or previous tablet. either we were not at the end,
  // in which case we can just move the focus, or we were at the end, in which
  // case we need bring the target tablet to our end, and draw in the direction
  // opposite travel (a single tablet is a trivial case of the latter case).
  // how do we know whether we were at the end? if the new line is not in the
  // direction of movement relative to the old one, of course!
  nctablet* topmost = find_topmost(nr);
  int wbegy, wbegx, wleny, wlenx;
  window_coordinates(nr->p, &wbegy, &wbegx, &wleny, &wlenx);
  int frontiery = wbegy + !(nr->ropts.bordermask & NCBOXMASK_TOP);
  if(nr->last_traveled_direction >= 0){
    ncplane_yx(nr->tablets->prev->p, &fromline, NULL);
    if(fromline > nowline){ // keep the order we had
      topmost = topmost->next;
    }
  }else{
    ncplane_yx(nr->tablets->next->p, &fromline, NULL);
    if(fromline < nowline){ // keep the order we had
      topmost = topmost->prev;
    }
  }
//fprintf(stderr, "gotta draw 'em all FROM: %d NOW: %d!\n", fromline, nowline);
  nctablet* t = topmost;
  do{
    int broken;
    if(t == nr->tablets){
      broken = ncreel_draw_tablet(nr, t, frontiery, 0);
    }else{
      broken = ncreel_draw_tablet(nr, t, frontiery, 1);
    }
    if(t->p == NULL || broken){
      nr->all_visible = false;
      break;
    }
    int basey;
    ncplane_dim_yx(t->p, &frontiery, NULL);
    ncplane_yx(t->p, &basey, NULL);
    frontiery += basey + 1;
  }while((t = t->next) != topmost);
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
  // FIXME we special-cased this because i'm dumb and couldn't think of a more
  // elegant way to do this. we keep 'all_visible' as boolean state to avoid
  // having to do an o(n) iteration each round, but this is still grotesque, and
  // feels fragile...
  if(nr->all_visible){
//fprintf(stderr, "all are visible!\n");
    return ncreel_arrange_denormalized(nr);
  }
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

ncreel* ncreel_create(ncplane* w, const ncreel_options* ropts, int efd){
  ncreel* nr;

  if(!validate_ncreel_opts(w, ropts)){
    return NULL;
  }
  if((nr = malloc(sizeof(*nr))) == NULL){
    return NULL;
  }
  nr->efd = efd;
  nr->tablets = NULL;
  nr->tabletcount = 0;
  nr->all_visible = true;
  nr->last_traveled_direction = -1; // draw down after the initial tablet
  memcpy(&nr->ropts, ropts, sizeof(*ropts));
  int maxx, maxy, wx, wy;
  window_coordinates(w, &wy, &wx, &maxy, &maxx);
  --maxy;
  --maxx;
  int ylen, xlen;
  ylen = maxy - ropts->boff - ropts->toff + 1;
  if(ylen < 0){
    ylen = maxy - ropts->toff;
    if(ylen < 0){
      ylen = 0; // but this translates to a full-screen window...FIXME
    }
  }
  xlen = maxx - ropts->roff - ropts->loff + 1;
  if(xlen < 0){
    xlen = maxx - ropts->loff;
    if(xlen < 0){
      xlen = 0; // FIXME see above...
    }
  }
  if((nr->p = ncplane_new(w->nc, ylen, xlen, ropts->toff + wy, ropts->loff + wx, NULL)) == NULL){
    free(nr);
    return NULL;
  }
  ncplane_set_base(nr->p, "", 0, ropts->bgchannel);
  if(ncreel_redraw(nr)){
    ncplane_destroy(nr->p);
    free(nr);
    return NULL;
  }
  return nr;
}

// we've just added a new tablet. it needs be inserted at the correct place in
// the reel. this will naturally fall out of things if the ncreel is full; we
// can just call ncreel_redraw(). otherwise, we need make ourselves at least
// minimally visible, to satisfy the preconditions of
// ncreel_arrange_denormalized(). this function, and approach, is shit.
// FIXME get rid of nc param here
static nctablet*
insert_new_panel(struct notcurses* nc, ncreel* nr, nctablet* t){
  if(!nr->all_visible){
    return t;
  }
  int wbegy, wbegx, wleny, wlenx; // params of PR
  window_coordinates(nr->p, &wbegy, &wbegx, &wleny, &wlenx);
  // are we the only tablet?
  int begx, begy, lenx, leny, frontiery;
  if(t->prev == t){
    frontiery = wbegy + !(nr->ropts.bordermask & NCBOXMASK_TOP);
    if(tablet_columns(nr, &begx, &begy, &lenx, &leny, frontiery, 1)){
      nr->all_visible = false;
      return t;
    }
// fprintf(stderr, "newwin: %d/%d + %d/%d\n", begy, begx, leny, lenx);
    if((t->p = ncplane_new(nc, leny, lenx, begy, begx, NULL)) == NULL){
      nr->all_visible = false;
      return t;
    }
    return t;
  }
  // we're not the only tablet, alas.
  // our new window needs to be after our prev
  ncplane_yx(t->prev->p, &frontiery, NULL);
  int dimprevy, dimprevx;
  ncplane_dim_yx(t->prev->p, &dimprevy, &dimprevx);
  frontiery += dimprevy + 2;
  frontiery += 2;
  if(tablet_columns(nr, &begx, &begy, &lenx, &leny, frontiery, 1)){
    nr->all_visible = false;
    return t;
  }
// fprintf(stderr, "newwin: %d/%d + %d/%d\n", begy, begx, 2, lenx);
  if((t->p = ncplane_new(nc, 2, lenx, begy, begx, NULL)) == NULL){
    nr->all_visible = false;
    return t;
  }
  // FIXME push the other ones down by 4
  return t;
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
  // if we have room, it needs become visible immediately, in the proper place,
  // lest we invalidate the preconditions of ncreel_arrange_denormalized().
  insert_new_panel(nr->p->nc, nr, t);
  ncreel_redraw(nr); // don't return failure; tablet was still created...
  return t;
}

int ncreel_del_focused(ncreel* nr){
  return ncreel_del(nr, nr->tablets);
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
  ncreel_redraw(nr);
  return 0;
}

int ncreel_destroy(ncreel* nreel){
  int ret = 0;
  if(nreel){
    nctablet* t = nreel->tablets;
    while(t){
      t->prev->next = NULL;
      nctablet* tmp = t->next;
      ncreel_del(nreel, t);
      t = tmp;
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

int ncreel_touch(ncreel* nr, nctablet* t){
  (void)t; // FIXME make these more granular eventually
  int ret = 0;
  if(nr->efd >= 0){
    uint64_t val = 1;
    if(write(nr->efd, &val, sizeof(val)) != sizeof(val)){
// fprintf(stderr, "Error writing to eventfd %d (%s)\n", nr->efd, strerror(errno));
      ret = -1;
    }
  }
  return ret;
}

// Move to some position relative to the current position
static int
move_tablet(ncplane* p, int deltax, int deltay){
  int oldx, oldy;
  ncplane_yx(p, &oldy, &oldx);
  int x = oldx + deltax;
  int y = oldy + deltay;
  ncplane_move_yx(p, y, x);
  return 0;
}

nctablet* ncreel_focused(ncreel* nr){
  return nr->tablets;
}

int ncreel_move(ncreel* nreel, int x, int y){
  ncplane* w = nreel->p;
  int oldx, oldy;
  ncplane_yx(w, &oldy, &oldx);
  const int deltax = x - oldx;
  const int deltay = y - oldy;
  if(move_tablet(nreel->p, deltax, deltay)){
    ncplane_move_yx(nreel->p, oldy, oldx);
    ncreel_redraw(nreel);
    return -1;
  }
  if(nreel->tablets){
    nctablet* t = nreel->tablets;
    do{
      if(t->p == NULL){
        break;
      }
      move_tablet(t->p, deltax, deltay);
    }while((t = t->prev) != nreel->tablets);
    if(t != nreel->tablets){ // don't repeat if we covered all tablets
      for(t = nreel->tablets->next ; t != nreel->tablets ; t = t->next){
        if(t->p == NULL){
          break;
        }
        move_tablet(t->p, deltax, deltay);
      }
    }
  }
  ncreel_redraw(nreel);
  return 0;
}

nctablet* ncreel_next(ncreel* nr){
  if(nr->tablets){
    nr->tablets = nr->tablets->next;
//fprintf(stderr, "---------------> moved to next, %p to %p <----------\n",
//        nr->tablets->prev, nr->tablets);
    nr->last_traveled_direction = 1;
  }
  ncreel_redraw(nr);
  return nr->tablets;
}

nctablet* ncreel_prev(ncreel* nr){
  if(nr->tablets){
    nr->tablets = nr->tablets->prev;
//fprintf(stderr, "----------------> moved to prev, %p to %p <----------\n",
//        nr->tablets->next, nr->tablets);
    nr->last_traveled_direction = -1;
  }
  ncreel_redraw(nr);
  return nr->tablets;
}
