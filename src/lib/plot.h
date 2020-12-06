#ifndef NOTCURSES_PLOT
#define NOTCURSES_PLOT

#include <cmath>
#include <array>
#include <limits>
#include <string>
#include "internal.h"
#include "notcurses/notcurses.h"

template<typename T>
class ncppplot {
 public:

 // these were all originally plain C, sorry for the non-idiomatic usage FIXME
 // ought admit nullptr opts FIXME
 // reenable logging once #703 is done
 static bool create(ncppplot<T>* ncpp, ncplane* n, const ncplot_options* opts, T miny, T maxy) {
   ncplot_options zeroed = {};
   if(!opts){
     opts = &zeroed;
   }
   if(opts->flags > NCPLOT_OPTION_DETECTMAXONLY){
     logwarn(ncplane_notcurses(n), "Provided unsupported flags %016jx\n", (uintmax_t)opts->flags);
   }
   //struct notcurses* nc = n->nc;
   // if miny == maxy (enabling domain detection), they both must be equal to 0
   if(miny == maxy && miny){
     //logerror(nc, "Supplied non-zero domain detection param %d\n", miny);
     ncplane_destroy(n);
     return false;
   }
   if(opts->rangex < 0){
     //logerror(nc, "Supplied negative independent range %d\n", opts->rangex);
     ncplane_destroy(n);
     return false;
   }
   if(maxy < miny){
     ncplane_destroy(n);
     return false;
   }
   // DETECTMAXONLY can't be used without domain detection
   if(opts->flags & NCPLOT_OPTION_DETECTMAXONLY && (miny != maxy)){
     //logerror(nc, "Supplied DETECTMAXONLY without domain detection");
     ncplane_destroy(n);
     return false;
   }
   ncblitter_e blitter = opts ? opts->gridtype : NCBLIT_DEFAULT;
   if(blitter == NCBLIT_DEFAULT){
     if(notcurses_canutf8(ncplane_notcurses(n))){
       blitter = NCBLIT_8x1;
     }else{
       blitter = NCBLIT_1x1;
     }
   }
   bool degrade_blitter = !(opts && (opts->flags & NCPLOT_OPTION_NODEGRADE));
   auto bset = lookup_blitset(notcurses_canutf8(ncplane_notcurses(n)),
                              blitter, degrade_blitter);
   if(bset == nullptr){
     ncplane_destroy(n);
     return false;
   }
   int sdimy, sdimx;
   ncplane_dim_yx(n, &sdimy, &sdimx);
   if(sdimx <= 0){
     ncplane_destroy(n);
     return false;
   }
   int dimx = sdimx;
   if(opts->title){
     ncpp->title = std::string(opts->title);
   }
   ncpp->rangex = opts->rangex;
   // if we're sizing the plot based off the plane dimensions, scale it by the
   // plot geometry's width for all calculations
   const int scaleddim = dimx * bset->width;
   const int scaledprefixlen = PREFIXCOLUMNS * bset->width;
   if((ncpp->slotcount = ncpp->rangex) == 0){
     ncpp->slotcount = scaleddim;
   }
   if(dimx < ncpp->rangex){
     ncpp->slotcount = scaleddim;
   }
   ncpp->legendstyle = opts->legendstyle;
   if( (ncpp->labelaxisd = opts->flags & NCPLOT_OPTION_LABELTICKSD) ){
     if(ncpp->slotcount + scaledprefixlen > scaleddim){
       if(scaleddim > scaledprefixlen){
         ncpp->slotcount = scaleddim - scaledprefixlen;
       }
     }
   }
   size_t slotsize = sizeof(*ncpp->slots) * ncpp->slotcount;
   ncpp->slots = static_cast<T*>(malloc(slotsize));
   if(ncpp->slots == nullptr){
     ncplane_destroy(n);
     return false;
   }
   memset(ncpp->slots, 0, slotsize);
   ncpp->ncp = n;
   ncpp->maxchannels = opts->maxchannels;
   ncpp->minchannels = opts->minchannels;
   ncpp->bset = bset;
   ncpp->miny = miny;
   ncpp->maxy = maxy;
   ncpp->vertical_indep = opts->flags & NCPLOT_OPTION_VERTICALI;
   ncpp->exponentiali = opts->flags & NCPLOT_OPTION_EXPONENTIALD;
   ncpp->detectonlymax = opts->flags & NCPLOT_OPTION_DETECTMAXONLY;
   if( (ncpp->detectdomain = (miny == maxy)) ){
     ncpp->maxy = 0;
     ncpp->miny = std::numeric_limits<T>::max();
   }
   ncpp->slotstart = 0;
   ncpp->slotx = 0;
   ncpp->redraw_plot();
   return true;
 }

 void destroy(){
   free(slots);
   ncplane_destroy(ncp);
 }

 auto redraw_plot() -> int {
   ncplane_erase(ncp);
   const int scale = bset->width;
   int dimy, dimx;
   ncplane_dim_yx(ncp, &dimy, &dimx);
   const int scaleddim = dimx * scale;
   // each transition is worth this much change in value
   const size_t states = bset->height + 1;
   // FIXME can we not rid ourselves of this meddlesome double? either way, the
   // interval is one row's range (for linear plots), or the base (base^slots==
   // maxy-miny) of the range (for exponential plots).
   double interval;
   if(exponentiali){
     if(maxy > miny){
       interval = pow(maxy - miny, (double)1 / (dimy * states));
 //fprintf(stderr, "miny: %ju maxy: %ju dimy: %d states: %zu\n", miny, maxy, dimy, states);
     }else{
       interval = 0;
     }
   }else{
     interval = maxy < miny ? 0 : (maxy - miny) / ((double)dimy * states);
   }
   const int startx = labelaxisd ? PREFIXCOLUMNS : 0; // plot cols begin here
   // if we want fewer slots than there are available columns, our final column
   // will be other than the plane's final column. most recent x goes here.
   const int finalx = (slotcount < scaleddim - 1 - (startx * scale) ? startx + (slotcount / scale) - 1 : dimx - 1);
   ncplane_set_styles(ncp, legendstyle);
   if(labelaxisd){
     // show the *top* of each interval range
     for(int y = 0 ; y < dimy ; ++y){
       uint64_t channels = 0;
       calc_gradient_channels(&channels, minchannels, minchannels,
                              maxchannels, maxchannels, y, 0, dimy, dimx);
       ncplane_set_channels(ncp, channels);
       char buf[PREFIXSTRLEN + 1];
       if(exponentiali){
         if(y == dimy - 1){ // we cheat on the top row to exactly match maxy
           ncmetric(maxy * 100, 100, buf, 0, 1000, '\0');
         }else{
           ncmetric(pow(interval, (y + 1) * states) * 100, 100, buf, 0, 1000, '\0');
         }
       }else{
         ncmetric((maxy - interval * states * (dimy - y - 1)) * 100, 100, buf, 0, 1000, '\0');
       }
       if(y == dimy - 1 && !title.empty()){
         ncplane_printf_yx(ncp, dimy - y - 1, PREFIXCOLUMNS - strlen(buf), "%s %s", buf, title.c_str());
       }else{
         ncplane_printf_yx(ncp, dimy - y - 1, PREFIXCOLUMNS - strlen(buf), "%s", buf);
       }
     }
   }else if(!title.empty()){
      uint64_t channels = 0;
      calc_gradient_channels(&channels, minchannels, minchannels,
                             maxchannels, maxchannels, dimy - 1, 0, dimy, dimx);
      ncplane_set_channels(ncp, channels);
      ncplane_printf_yx(ncp, 0, PREFIXCOLUMNS - title.length(), "%s", title.c_str());
   }
   ncplane_set_styles(ncp, NCSTYLE_NONE);
   if(finalx < startx){ // exit on pathologically narrow planes
     return 0;
   }
   if(!interval){
     interval = 1;
   }
   #define MAXWIDTH 2
   int idx = slotstart; // idx holds the real slot index; we move backwards
   for(int x = finalx ; x >= startx ; --x){
     // a single column might correspond to more than 1 ('scale', up to
     // MAXWIDTH) slot's worth of samples. prepare the working gval set.
     T gvals[MAXWIDTH];
     // load it retaining the same ordering we have in the actual array
     for(int i = scale - 1 ; i >= 0 ; --i){
       gvals[i] = slots[idx]; // clip the value at the limits of the graph
       if(gvals[i] < miny){
         gvals[i] = miny;
       }
       if(gvals[i] > maxy){
         gvals[i] = maxy;
       }
       // FIXME if there are an odd number, only go up through the valid ones...
       if(--idx < 0){
         idx = slotcount - 1;
       }
     }
     // starting from the least-significant row, progress in the more significant
     // direction, drawing egcs from the grid specification, aborting early if
     // we can't draw anything in a given cell.
     T intervalbase = miny;
     const wchar_t* egc = bset->egcs;
     for(int y = 0 ; y < dimy ; ++y){
       uint64_t channels = 0;
       calc_gradient_channels(&channels, minchannels, minchannels,
                              maxchannels, maxchannels, y, x, dimy, dimx);
       ncplane_set_channels(ncp, channels);
       size_t egcidx = 0, sumidx = 0;
       // if we've got at least one interval's worth on the number of positions
       // times the number of intervals per position plus the starting offset,
       // we're going to print *something*
       bool done = !bset->fill;
       for(int i = 0 ; i < scale ; ++i){
         sumidx *= states;
         if(intervalbase < gvals[i]){
           if(exponentiali){
             // we want the log-base-interval of gvals[i]
             double scaled = log(gvals[i] - miny) / log(interval);
             double sival = intervalbase ? log(intervalbase) / log(interval) : 0;
             egcidx = scaled - sival;
           }else{
             egcidx = (gvals[i] - intervalbase) / interval;
           }
           if(egcidx >= states){
             egcidx = states - 1;
             done = false;
           }
           sumidx += egcidx;
         }else{
           egcidx = 0;
         }
       }
       // if we're not UTF8, we can only arrive here via NCBLIT_1x1 (otherwise
       // we would have errored out during construction). even then, however,
       // we need handle ASCII differently, since it can't print full block.
       // in ASCII mode, egcidx != means swap colors and use space.
       if(sumidx){
//fprintf(stderr, "dimy: %d y: %d x: %d sumidx: %zu egc[%zu]: %lc\n", dimy, y, x, sumidx, sumidx, egc[sumidx]);
         if(notcurses_canutf8(ncplane_notcurses(ncp))){
           char utf8[MB_CUR_MAX + 1];
           int bytes = wctomb(utf8, egc[sumidx]);
           if(bytes < 0){
             return -1;
           }
           utf8[bytes] = '\0';
           cell* c = ncplane_cell_ref_yx(ncp, dimy - y - 1, x);
           cell_set_bchannel(c, channels_bchannel(channels));
           cell_set_fchannel(c, channels_fchannel(channels));
           cell_set_styles(c, NCSTYLE_NONE);
           if(pool_blit_direct(&ncp->pool, c, utf8, bytes, 1) <= 0){
             return -1;
           }
         }else{
           const uint64_t swapbg = channels_bchannel(channels);
           const uint64_t swapfg = channels_fchannel(channels);
           channels_set_bchannel(&channels, swapfg);
           channels_set_fchannel(&channels, swapbg);
           ncplane_set_channels(ncp, channels);
           if(ncplane_putchar_yx(ncp, dimy - y - 1, x, ' ') <= 0){
             return -1;
           }
           channels_set_bchannel(&channels, swapbg);
           channels_set_fchannel(&channels, swapfg);
           ncplane_set_channels(ncp, channels);
         }
       }
       if(done){
         break;
       }
       if(exponentiali){
         intervalbase = miny + pow(interval, (y + 1) * states - 1);
       }else{
         intervalbase += (states * interval);
       }
     }
   }
   ncplane_home(ncp);
   return 0;
 }

 // Add to or set the value corresponding to this x. If x is beyond the current
 // x window, the x window is advanced to include x, and values passing beyond
 // the window are lost. The first call will place the initial window. The plot
 // will be redrawn, but notcurses_render() is not called.
 auto add_sample(uint64_t x, T y) -> int {
   if(window_slide(x)){
     return -1;
   }
   update_sample(x, y, false);
   if(update_domain(x)){
     return -1;
   }
   return redraw_plot();
 }

 auto set_sample(uint64_t x, T y) -> int {
   if(window_slide(x)){
     return -1;
   }
   update_sample(x, y, true);
   if(update_domain(x)){
     return -1;
   }
   return redraw_plot();
 }

 auto sample(int64_t x, T* y) const -> int {
   if(x < slotx - (slotcount - 1)){ // x is behind window
     return -1;
   }else if(x > slotx){ // x is ahead of window
     return -1;
   }
   *y = slots[x % slotcount];
   return 0;
 }

 // if we're doing domain detection, update the domain to reflect the value we
 // just set. if we're not, check the result against the known ranges, and
 // return -1 if the value is outside of that range.
 auto update_domain(uint64_t x) -> int {
   const T val = slots[x % slotcount];
   if(detectdomain){
     if(val > maxy){
       maxy = val;
     }
     if(!detectonlymax){
       if(val < miny){
         miny = val;
       }
     }
     return 0;
   }
   if(val > maxy || val < miny){
     return -1;
   }
   return 0;
 }

 // if x is less than the window, return -1, as the sample will be thrown away.
 // if the x is within the current window, find the proper slot and update it.
 // otherwise, the x is the newest sample. if it is obsoletes all existing slots,
 // reset them, and write the new sample anywhere. otherwise, write it to the
 // proper slot based on the current newest slot.
 auto window_slide(int64_t x) -> int {
   if(x < slotx - (slotcount - 1)){ // x is behind window, won't be counted
     return -1;
   }else if(x <= slotx){ // x is within window, do nothing
     return 0;
   } // x is newest; we might be keeping some, might not
   int64_t xdiff = x - slotx; // the raw amount we're advancing
   slotx = x;
   if(xdiff >= slotcount){ // we're throwing away all old samples, write to 0
     memset(slots, 0, sizeof(*slots) * slotcount); // FIXME need a STL operation?
     slotstart = 0;
     return 0;
   }
   // we're throwing away only xdiff slots, which is less than slotcount.
   // first, we'll try to clear to the right...number to reset on the right of
   // the circular buffer. min of (available at current or to right, xdiff)
   int slotsreset = slotcount - slotstart - 1;
   if(slotsreset > xdiff){
     slotsreset = xdiff;
   }
   if(slotsreset){
     memset(slots + slotstart + 1, 0, slotsreset * sizeof(*slots));
   }
   slotstart = (slotstart + xdiff) % slotcount;
   xdiff -= slotsreset;
   if(xdiff){ // throw away some at the beginning
     memset(slots, 0, xdiff * sizeof(*slots));
   }
   return 0;
 }

 // x must be within n's window at this point
 inline void update_sample(int64_t x, T y, bool reset){
   const int64_t diff = slotx - x; // amount behind
   const int idx = (slotstart + slotcount - diff) % slotcount;
   if(reset){
     slots[idx] = y;
   }else{
     slots[idx] += y;
   }
 }

 ncplane* ncp;
 // sloutcount-element circular buffer of samples. the newest one (rightmost)
 // is at slots[slotstart]; they get older as you go back (and around).
 // elements. slotcount is max(columns, rangex), less label room.
 T* slots;
 int64_t slotx; // x value corresponding to slots[slotstart] (newest x)

 private:

 uint64_t maxchannels;
 uint64_t minchannels;
 uint16_t legendstyle;
 bool vertical_indep; // not yet implemented FIXME
 const struct blitset* bset;
 std::string title;
 // requested number of slots. 0 for automatically setting the number of slots
 // to span the horizontal area. if there are more slots than there are
 // columns, we prefer showing more recent slots to less recent. if there are
 // fewer slots than there are columns, they prefer the left side.
 int rangex;
 // domain minimum and maximum. if detectdomain is true, these are
 // progressively enlarged/shrunk to fit the sample set. if not, samples
 // outside these bounds are counted, but the displayed range covers only this.
 T miny, maxy;
 int slotcount;
 int slotstart; // index of most recently-written slot
 bool labelaxisd; // label dependent axis (consumes PREFIXCOLUMNS columns)
 bool exponentiali; // exponential independent axis
 bool detectdomain;   // is domain detection in effect (stretch the domain)?
 bool detectonlymax;  // domain detection applies only to max, not min

};

using ncuplot = struct ncuplot {
  ncppplot<uint64_t> n;
};

using ncdplot = struct ncdplot {
  ncppplot<double> n;
};

#endif
