#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "notcurses.h"
#include "internal.h"

// Tablets are the toplevel entitites within a panelreel. Each corresponds to
// a single, distinct ncplane.
typedef struct tablet {
  ncplane* p;                    // visible panel, NULL when offscreen
  struct tablet* next;
  struct tablet* prev;
  tabletcb cbfxn;              // application callback to draw tablet
  void* curry;                 // application data provided to cbfxn
} tablet;

// The visible screen can be reconstructed from three things:
//  * which tablet is focused (pointed at by tablets)
//  * which row the focused tablet starts at (derived from focused window)
//  * the list of tablets (available from the focused tablet)
typedef struct panelreel {
  ncplane* p;                // ncplane this panelreel occupies, under tablets
  panelreel_options popts; // copied in panelreel_create()
  // doubly-linked list, a circular one when infinity scrolling is in effect.
  // points at the focused tablet (when at least one tablet exists, one must be
  // focused), which might be anywhere on the screen (but is always visible).
  int efd;                 // eventfd, signaled in panelreel_touch() if >= 0
  tablet* tablets;
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
} panelreel;

// Returns the starting coordinates (relative to the screen) of the specified
// window, and its length. End is (begx + lenx - 1, begy + leny - 1).
static inline void
window_coordinates(const ncplane* w, int* begy, int* begx, int* leny, int* lenx){
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

// Draws the border (if one should be drawn) around the panelreel, and enforces
// any provided restrictions on visible window size.
static int
draw_panelreel_borders(const panelreel* pr){
  int begx, begy;
  int maxx, maxy;
  window_coordinates(pr->p, &begy, &begx, &maxy, &maxx);
  assert(maxy >= 0 && maxx >= 0);
  --maxx; // last column we can safely write to
  --maxy; // last line we can safely write to
  if(begx >= maxx || maxx - begx + 1 < pr->popts.min_supported_rows){
    return 0; // no room
  }
  if(begy >= maxy || maxy - begy + 1 < pr->popts.min_supported_cols){
    return 0; // no room
  }
  return draw_borders(pr->p, pr->popts.bordermask, pr->popts.borderchan, false, false);
}

// Calculate the starting and ending coordinates available for occupation by
// the tablet, relative to the panelreel's ncplane. Returns non-zero if the
// tablet cannot be made visible as specified. If this is the focused tablet
// (direction == 0), it can take the entire reel -- frontiery is only a
// suggestion in this case -- so give it the full breadth.
static int
tablet_columns(const panelreel* pr, int* begx, int* begy, int* lenx, int* leny,
               int frontiery, int direction){
  window_coordinates(pr->p, begy, begx, leny, lenx);
  int maxy = *leny + *begy - 1;
  int begindraw = *begy + !(pr->popts.bordermask & NCBOXMASK_TOP);
  int enddraw = maxy - !(pr->popts.bordermask & NCBOXMASK_TOP);
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
  // account for the panelreel borders
  if(direction <= 0 && !(pr->popts.bordermask & NCBOXMASK_TOP)){
    ++*begy;
    --*leny;
  }
  if(direction >= 0 && !(pr->popts.bordermask & NCBOXMASK_BOTTOM)){
    --*leny;
  }
  if(!(pr->popts.bordermask & NCBOXMASK_LEFT)){
    ++*begx;
    --*lenx;
  }
  if(!(pr->popts.bordermask & NCBOXMASK_RIGHT)){
    --*lenx;
  }
  // at this point, our coordinates describe the largest possible tablet for
  // this panelreel. this is the correct solution for the focused tablet. other
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
panelreel_draw_tablet(const panelreel* pr, tablet* t, int frontiery,
                      int direction){
  int lenx, leny, begy, begx;
  ncplane* fp = t->p;
  if(tablet_columns(pr, &begx, &begy, &lenx, &leny, frontiery, direction)){
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
    t->p = notcurses_newplane(pr->p->nc, leny + 1, lenx, begy, begx, NULL);
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
  if(direction < 0 && !(pr->popts.tabletmask & NCBOXMASK_BOTTOM)){
    --cbmaxy;
  }
  // If we're drawing down, we'll always have a top border unless it's masked
  if(direction >= 0 && !(pr->popts.tabletmask & NCBOXMASK_TOP)){
    ++cby;
  }
  // Adjust the x-bounds for side borders, which we always have if unmasked
  cbmaxx -= !(pr->popts.tabletmask & NCBOXMASK_RIGHT);
  cbx += !(pr->popts.tabletmask & NCBOXMASK_LEFT);
  bool cbdir = direction < 0 ? true : false;
// fprintf(stderr, "calling! lenx/leny: %d/%d cbx/cby: %d/%d cbmaxx/cbmaxy: %d/%d dir: %d\n",
//    lenx, leny, cbx, cby, cbmaxx, cbmaxy, direction);
  int ll = t->cbfxn(t, cbx, cby, cbmaxx, cbmaxy, cbdir);
//fprintf(stderr, "RETURNRETURNRETURN %p %d (%d, %d, %d) DIR %d\n",
//        t, ll, cby, cbmaxy, leny, direction);
  if(ll != leny){
    if(ll == leny - 1){ // only has one border visible (partially off-screen)
      if(cbdir){
        ll += !(pr->popts.tabletmask & NCBOXMASK_BOTTOM);
      }else{
        ll += !(pr->popts.tabletmask & NCBOXMASK_TOP);
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
      ll += !(pr->popts.tabletmask & NCBOXMASK_BOTTOM) +
            !(pr->popts.tabletmask & NCBOXMASK_TOP);
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
        int dontcarex;
        ncplane_yx(pr->p, &frontiery, &dontcarex);
        frontiery += (leny - ll + 1);
      }
      ncplane_move_yx(fp, frontiery, begx);
    }
  }
  draw_borders(fp, pr->popts.tabletmask,
               direction == 0 ? pr->popts.focusedchan : pr->popts.tabletchan,
               cliphead, clipfoot);
  return cliphead || clipfoot;
}

// draw and size the focused tablet, which must exist (pr->tablets may not be
// NULL). it can occupy the entire panelreel.
static int
draw_focused_tablet(const panelreel* pr){
  int pbegy, pbegx, plenx, pleny; // panelreel window coordinates
  window_coordinates(pr->p, &pbegy, &pbegx, &pleny, &plenx);
  int fulcrum;
  if(pr->tablets->p == NULL){
    if(pr->last_traveled_direction >= 0){
      fulcrum = pleny + pbegy - !(pr->popts.bordermask & NCBOXMASK_BOTTOM);
    }else{
      fulcrum = pbegy + !(pr->popts.bordermask & NCBOXMASK_TOP);
    }
  }else{ // focused was already present. want to stay where we are, if possible
    int dontcarex;
    ncplane_yx(pr->tablets->p, &fulcrum, &dontcarex);
    // FIXME ugh can't we just remember the previous fulcrum?
    if(pr->last_traveled_direction > 0){
      if(pr->tablets->prev->p){
        int prevfulcrum;
        ncplane_yx(pr->tablets->prev->p, &prevfulcrum, &dontcarex);
        if(fulcrum < prevfulcrum){
          fulcrum = pleny + pbegy - !(pr->popts.bordermask & NCBOXMASK_BOTTOM);
        }
      }
    }else if(pr->last_traveled_direction < 0){
      if(pr->tablets->next->p){
        int nextfulcrum;
        ncplane_yx(pr->tablets->next->p, &nextfulcrum, &dontcarex);
        if(fulcrum > nextfulcrum){
          fulcrum = pbegy + !(pr->popts.bordermask & NCBOXMASK_TOP);
        }
      }
    }
  }
//fprintf(stderr, "PR dims: %d/%d + %d/%d fulcrum: %d\n", pbegy, pbegx, pleny, plenx, fulcrum);
  panelreel_draw_tablet(pr, pr->tablets, fulcrum, 0 /* pr->last_traveled_direction*/);
  return 0;
}

// move down below the focused tablet, filling up the reel to the bottom.
// returns the last tablet drawn.
static tablet*
draw_following_tablets(const panelreel* pr, const tablet* otherend){
  int wmaxy, wbegy, wbegx, wlenx, wleny; // working tablet window coordinates
  tablet* working = pr->tablets;
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
    panelreel_draw_tablet(pr, working, frontiery, 1);
    if(working == otherend){
      otherend = otherend->next;
    }
  }while(working->p); // FIXME might be more to hide
  // FIXME keep going forward, hiding those no longer visible
  return working;
}

// move up above the focused tablet, filling up the reel to the top.
// returns the last tablet drawn.
static tablet*
draw_previous_tablets(const panelreel* pr, const tablet* otherend){
//fprintf(stderr, "preceding otherend: %p ->p: %p\n", otherend, otherend->p);
  int wbegy, wbegx, wlenx, wleny; // working tablet window coordinates
  tablet* upworking = pr->tablets;
  int frontiery;
  // modify frontier based off the one we're at
  window_coordinates(upworking->p, &wbegy, &wbegx, &wleny, &wlenx);
  frontiery = wbegy - 2;
  while(upworking->prev != otherend || otherend->p == NULL){
//fprintf(stderr, "MOVIN' ON UP: %p->%p %d %d\n", upworking, upworking->prev, frontiery, wbegy - 2);
    upworking = upworking->prev;
    panelreel_draw_tablet(pr, upworking, frontiery, -1);
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
static tablet*
find_topmost(panelreel* pr){
  tablet* t = pr->tablets;
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
panelreel_arrange_denormalized(panelreel* pr){
//fprintf(stderr, "denormalized devolution (are we men?)\n");
  // we'll need the starting line of the tablet which just lost focus, and the
  // starting line of the tablet which just gained focus.
  int fromline, nowline;
  ncplane_yx(pr->tablets->p, &nowline, NULL);
  // we've moved to the next or previous tablet. either we were not at the end,
  // in which case we can just move the focus, or we were at the end, in which
  // case we need bring the target tablet to our end, and draw in the direction
  // opposite travel (a single tablet is a trivial case of the latter case).
  // how do we know whether we were at the end? if the new line is not in the
  // direction of movement relative to the old one, of course!
  tablet* topmost = find_topmost(pr);
  int wbegy, wbegx, wleny, wlenx;
  window_coordinates(pr->p, &wbegy, &wbegx, &wleny, &wlenx);
  int frontiery = wbegy + !(pr->popts.bordermask & NCBOXMASK_TOP);
  if(pr->last_traveled_direction >= 0){
    ncplane_yx(pr->tablets->prev->p, &fromline, NULL);
    if(fromline > nowline){ // keep the order we had
      topmost = topmost->next;
    }
  }else{
    ncplane_yx(pr->tablets->next->p, &fromline, NULL);
    if(fromline < nowline){ // keep the order we had
      topmost = topmost->prev;
    }
  }
//fprintf(stderr, "gotta draw 'em all FROM: %d NOW: %d!\n", fromline, nowline);
  tablet* t = topmost;
  do{
    int broken;
    if(t == pr->tablets){
      broken = panelreel_draw_tablet(pr, t, frontiery, 0);
    }else{
      broken = panelreel_draw_tablet(pr, t, frontiery, 1);
    }
    if(t->p == NULL || broken){
      pr->all_visible = false;
      break;
    }
    int dontcarex, basey;
    ncplane_dim_yx(t->p, &frontiery, &dontcarex);
    ncplane_yx(t->p, &basey, &dontcarex);
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
int panelreel_redraw(panelreel* pr){
//fprintf(stderr, "--------> BEGIN REDRAW <--------\n");
  if(draw_panelreel_borders(pr)){
    return -1; // enforces specified dimensional minima
  }
  tablet* focused = pr->tablets;
  if(focused == NULL){
//fprintf(stderr, "no focus!\n");
    return 0; // if none are focused, none exist
  }
//fprintf(stderr, "focused %p!\n", focused);
  // FIXME we special-cased this because i'm dumb and couldn't think of a more
  // elegant way to do this. we keep 'all_visible' as boolean state to avoid
  // having to do an o(n) iteration each round, but this is still grotesque, and
  // feels fragile...
  if(pr->all_visible){
//fprintf(stderr, "all are visible!\n");
    return panelreel_arrange_denormalized(pr);
  }
//fprintf(stderr, "drawing focused tablet %p dir: %d!\n", focused, pr->last_traveled_direction);
  draw_focused_tablet(pr);
//fprintf(stderr, "drew focused tablet %p dir: %d!\n", focused, pr->last_traveled_direction);
  tablet* otherend = focused;
  if(pr->last_traveled_direction >= 0){
    otherend = draw_previous_tablets(pr, otherend);
    otherend = draw_following_tablets(pr, otherend);
    otherend = draw_previous_tablets(pr, otherend);
  }else{
    otherend = draw_following_tablets(pr, otherend);
    otherend = draw_previous_tablets(pr, otherend);
    otherend = draw_following_tablets(pr, otherend);
  }
//fprintf(stderr, "DONE ARRANGING\n");
  return 0;
}

static bool
validate_panelreel_opts(ncplane* w, const panelreel_options* popts){
  if(w == NULL){
    return false;
  }
  if(!popts->infinitescroll){
    if(popts->circular){
      return false; // can't set circular without infinitescroll
    }
  }
  const unsigned fullmask = NCBOXMASK_LEFT |
                            NCBOXMASK_RIGHT |
                            NCBOXMASK_TOP |
                            NCBOXMASK_BOTTOM;
  if(popts->bordermask > fullmask){
    return false;
  }
  if(popts->tabletmask > fullmask){
    return false;
  }
  return true;
}

ncplane* tablet_ncplane(tablet* t){
  return t->p;
}

const ncplane* tablet_ncplane_const(const tablet* t){
  return t->p;
}

ncplane* panelreel_plane(panelreel* pr){
  return pr->p;
}

panelreel* panelreel_create(ncplane* w, const panelreel_options* popts, int efd){
  panelreel* pr;

  if(!validate_panelreel_opts(w, popts)){
    return NULL;
  }
  if((pr = malloc(sizeof(*pr))) == NULL){
    return NULL;
  }
  pr->efd = efd;
  pr->tablets = NULL;
  pr->tabletcount = 0;
  pr->all_visible = true;
  pr->last_traveled_direction = -1; // draw down after the initial tablet
  memcpy(&pr->popts, popts, sizeof(*popts));
  int maxx, maxy, wx, wy;
  window_coordinates(w, &wy, &wx, &maxy, &maxx);
  --maxy;
  --maxx;
  int ylen, xlen;
  ylen = maxy - popts->boff - popts->toff + 1;
  if(ylen < 0){
    ylen = maxy - popts->toff;
    if(ylen < 0){
      ylen = 0; // but this translates to a full-screen window...FIXME
    }
  }
  xlen = maxx - popts->roff - popts->loff + 1;
  if(xlen < 0){
    xlen = maxx - popts->loff;
    if(xlen < 0){
      xlen = 0; // FIXME see above...
    }
  }
  if((pr->p = notcurses_newplane(w->nc, ylen, xlen, popts->toff + wy, popts->loff + wx, NULL)) == NULL){
    free(pr);
    return NULL;
  }
  cell bgc = CELL_TRIVIAL_INITIALIZER;
  bgc.channels = popts->bgchannel;
  ncplane_set_default(pr->p, &bgc);
  cell_release(pr->p, &bgc);
  if(panelreel_redraw(pr)){
    ncplane_destroy(pr->p);
    free(pr);
    return NULL;
  }
  return pr;
}

// we've just added a new tablet. it needs be inserted at the correct place in
// the reel. this will naturally fall out of things if the panelreel is full; we
// can just call panelreel_redraw(). otherwise, we need make ourselves at least
// minimally visible, to satisfy the preconditions of
// panelreel_arrange_denormalized(). this function, and approach, is shit.
// FIXME get rid of nc param here
static tablet*
insert_new_panel(struct notcurses* nc, panelreel* pr, tablet* t){
  if(!pr->all_visible){
    return t;
  }
  int wbegy, wbegx, wleny, wlenx; // params of PR
  window_coordinates(pr->p, &wbegy, &wbegx, &wleny, &wlenx);
  // are we the only tablet?
  int begx, begy, lenx, leny, frontiery;
  if(t->prev == t){
    frontiery = wbegy + !(pr->popts.bordermask & NCBOXMASK_TOP);
    if(tablet_columns(pr, &begx, &begy, &lenx, &leny, frontiery, 1)){
      pr->all_visible = false;
      return t;
    }
// fprintf(stderr, "newwin: %d/%d + %d/%d\n", begy, begx, leny, lenx);
    if((t->p = notcurses_newplane(nc, leny, lenx, begy, begx, NULL)) == NULL){
      pr->all_visible = false;
      return t;
    }
    return t;
  }
  // we're not the only tablet, alas.
  // our new window needs to be after our prev
  int dontcarex;
  ncplane_yx(t->prev->p, &frontiery, &dontcarex);
  int dimprevy, dimprevx;
  ncplane_dim_yx(t->prev->p, &dimprevy, &dimprevx);
  frontiery += dimprevy + 2;
  frontiery += 2;
  if(tablet_columns(pr, &begx, &begy, &lenx, &leny, frontiery, 1)){
    pr->all_visible = false;
    return t;
  }
// fprintf(stderr, "newwin: %d/%d + %d/%d\n", begy, begx, 2, lenx);
  if((t->p = notcurses_newplane(nc, 2, lenx, begy, begx, NULL)) == NULL){
    pr->all_visible = false;
    return t;
  }
  // FIXME push the other ones down by 4
  return t;
}

tablet* panelreel_add(panelreel* pr, tablet* after, tablet *before,
                      tabletcb cbfxn, void* opaque){
  tablet* t;
  if(after && before){
    if(after->prev != before || before->next != after){
      return NULL;
    }
  }else if(!after && !before){
    // This way, without user interaction or any specification, new tablets are
    // inserted at the "end" relative to the focus. The first one to be added
    // gets and keeps the focus. New ones will go on the bottom, until we run
    // out of space. New tablets are then created off-screen.
    before = pr->tablets;
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
    pr->tablets = t;
  }
  t->cbfxn = cbfxn;
  t->curry = opaque;
  ++pr->tabletcount;
  t->p = NULL;
  // if we have room, it needs become visible immediately, in the proper place,
  // lest we invalidate the preconditions of panelreel_arrange_denormalized().
  insert_new_panel(pr->p->nc, pr, t);
  panelreel_redraw(pr); // don't return failure; tablet was still created...
  return t;
}

int panelreel_del_focused(panelreel* pr){
  return panelreel_del(pr, pr->tablets);
}

int panelreel_del(panelreel* pr, struct tablet* t){
  if(pr == NULL || t == NULL){
    return -1;
  }
  t->prev->next = t->next;
  if(pr->tablets == t){
    if((pr->tablets = t->next) == t){
      pr->tablets = NULL;
    }
  }
  t->next->prev = t->prev;
  if(t->p){
    ncplane_destroy(t->p);
  }
  free(t);
  --pr->tabletcount;
  panelreel_redraw(pr);
  return 0;
}

int panelreel_destroy(panelreel* preel){
  int ret = 0;
  if(preel){
    tablet* t = preel->tablets;
    while(t){
      t->prev->next = NULL;
      tablet* tmp = t->next;
      panelreel_del(preel, t);
      t = tmp;
    }
    ncplane_destroy(preel->p);
    free(preel);
  }
  return ret;
}

void* tablet_userptr(tablet* t){
  return t->curry;
}

const void* tablet_userptr_const(const tablet* t){
  return t->curry;
}

int panelreel_tabletcount(const panelreel* preel){
  return preel->tabletcount;
}

int panelreel_touch(panelreel* pr, tablet* t){
  (void)t; // FIXME make these more granular eventually
  int ret = 0;
  if(pr->efd >= 0){
    uint64_t val = 1;
    if(write(pr->efd, &val, sizeof(val)) != sizeof(val)){
      fprintf(stderr, "Error writing to eventfd %d (%s)\n",
              pr->efd, strerror(errno));
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

tablet* panelreel_focused(panelreel* pr){
  return pr->tablets;
}

int panelreel_move(panelreel* preel, int x, int y){
  ncplane* w = preel->p;
  int oldx, oldy;
  ncplane_yx(w, &oldy, &oldx);
  const int deltax = x - oldx;
  const int deltay = y - oldy;
  if(move_tablet(preel->p, deltax, deltay)){
    ncplane_move_yx(preel->p, oldy, oldx);
    panelreel_redraw(preel);
    return -1;
  }
  if(preel->tablets){
    tablet* t = preel->tablets;
    do{
      if(t->p == NULL){
        break;
      }
      move_tablet(t->p, deltax, deltay);
    }while((t = t->prev) != preel->tablets);
    if(t != preel->tablets){ // don't repeat if we covered all tablets
      for(t = preel->tablets->next ; t != preel->tablets ; t = t->next){
        if(t->p == NULL){
          break;
        }
        move_tablet(t->p, deltax, deltay);
      }
    }
  }
  panelreel_redraw(preel);
  return 0;
}

tablet* panelreel_next(panelreel* pr){
  if(pr->tablets){
    pr->tablets = pr->tablets->next;
//fprintf(stderr, "---------------> moved to next, %p to %p <----------\n",
//        pr->tablets->prev, pr->tablets);
    pr->last_traveled_direction = 1;
  }
  panelreel_redraw(pr);
  return pr->tablets;
}

tablet* panelreel_prev(panelreel* pr){
  if(pr->tablets){
    pr->tablets = pr->tablets->prev;
//fprintf(stderr, "----------------> moved to prev, %p to %p <----------\n",
//        pr->tablets->next, pr->tablets);
    pr->last_traveled_direction = -1;
  }
  panelreel_redraw(pr);
  return pr->tablets;
}
