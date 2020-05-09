#include "notcurses/notcurses.h"
#include <limits>

static const struct {
  ncgridgeom_e geom;
  int width;
  int height;
  // the EGCs which form the various levels of a given geometry. if the geometry
  // is wide, things are arranged with the rightmost side increasing most
  // quickly, i.e. it can be indexed as height arrays of 1 + height glyphs. i.e.
  // the first five braille EGCs are all 0 on the left, [0..4] on the right.
  const wchar_t* egcs;
  bool fill;
} geomdata[] = {
  { .geom = NCPLOT_1x1,   .width = 1, .height = 2, .egcs = L" █",                        .fill = false, },
  { .geom = NCPLOT_2x1,   .width = 1, .height = 3, .egcs = L" ▄█",                       .fill = false, },
  { .geom = NCPLOT_1x1x4, .width = 1, .height = 5, .egcs = L" ▒░▓█",                     .fill = false, },
  { .geom = NCPLOT_2x2,   .width = 2, .height = 3, .egcs = L" ▗▐▖▄▟▌▙█",                 .fill = false, },
  { .geom = NCPLOT_4x1,   .width = 1, .height = 5, .egcs = L" ▂▄▆█",                     .fill = false, },
  { .geom = NCPLOT_4x2,   .width = 2, .height = 5, .egcs = L"⠀⡀⡄⡆⡇⢀⣀⣄⣆⣇⢠⣠⣤⣦⣧⢰⣰⣴⣶⣷⢸⣸⣼⣾⣿", .fill = true,  },
  { .geom = NCPLOT_8x1,   .width = 1, .height = 9, .egcs = L" ▁▂▃▄▅▆▇█",                 .fill = false, },
};

template<typename T>
class ncppplot {
 public:

 // these were all originally plain C, sorry for the non-idiomatic usage FIXME
 static bool create(ncppplot<T>* ncpp, ncplane* n, const ncplot_options* opts, T miny, T maxy){
   // if miny == maxy, they both must be equal to 0
   if(miny == maxy && miny){
     return false;
   }
   if(opts->rangex < 0){
     return false;
   }
   if(maxy < miny){
     return false;
   }
   if(opts->gridtype < 0 || opts->gridtype >= sizeof(geomdata) / sizeof(*geomdata)){
     return false;
   }
   int sdimy, sdimx;
   ncplane_dim_yx(n, &sdimy, &sdimx);
   if(sdimx <= 0){
     return false;
   }
   int dimx = sdimx;
   ncpp->rangex = opts->rangex;
   // if we're sizing the plot based off the plane dimensions, scale it by the
   // plot geometry's width for all calculations
   const int scaleddim = dimx * geomdata[opts->gridtype].width;
   const int scaledprefixlen = PREFIXSTRLEN * geomdata[opts->gridtype].width;
   if((ncpp->slotcount = ncpp->rangex) == 0){
     ncpp->slotcount = scaleddim;
   }
   if(dimx < ncpp->rangex){
     ncpp->slotcount = scaleddim;
   }
   if( (ncpp->labelaxisd = opts->flags & NCPLOT_OPTIONS_LABELAXISD) ){
     if(ncpp->slotcount + scaledprefixlen > scaleddim){
       if(scaleddim > scaledprefixlen){
         ncpp->slotcount = scaleddim - scaledprefixlen;
       }
     }
   }
   size_t slotsize = sizeof(*ncpp->slots) * ncpp->slotcount;
   ncpp->slots = static_cast<T*>(malloc(slotsize));
   if(ncpp->slots){
     memset(ncpp->slots, 0, slotsize);
     ncpp->ncp = n;
     ncpp->maxchannel = opts->maxchannel;
     ncpp->minchannel = opts->minchannel;
     ncpp->miny = miny;
     ncpp->maxy = maxy;
     ncpp->vertical_indep = opts->flags & NCPLOT_OPTIONS_VERINDEP;
     ncpp->gridtype = opts->gridtype;
     ncpp->exponentially = opts->flags & NCPLOT_OPTIONS_EXPONENTIALY;
     if( (ncpp->detectdomain = (miny == maxy)) ){
       ncpp->maxy = 0;
       ncpp->miny = std::numeric_limits<T>::max();
     }
     ncpp->slotstart = 0;
     ncpp->slotx = 0;
     ncpp->redraw_plot();
     return true;
   }
   return false;
 }

 // Add to or set the value corresponding to this x. If x is beyond the current
 // x window, the x window is advanced to include x, and values passing beyond
 // the window are lost. The first call will place the initial window. The plot
 // will be redrawn, but notcurses_render() is not called.
 int add_sample(uint64_t x, T y){
   if(window_slide(x)){
     return -1;
   }
   update_sample(x, y, false);
   if(update_domain(x)){
     return -1;
   }
   return redraw_plot();
 }
 
 int set_sample(uint64_t x, T y){
   if(window_slide(x)){
     return -1;
   }
   update_sample(x, y, true);
   if(update_domain(x)){
     return -1;
   }
   return redraw_plot();
 }
 
 void destroy(){
   free(slots);
 }

 // FIXME everything below here ought be private, but it busts unit tests
 int redraw_plot(){
   ncplane_erase(ncp);
   const int scale = geomdata[gridtype].width;
   int dimy, dimx;
   ncplane_dim_yx(ncp, &dimy, &dimx);
   const int scaleddim = dimx * scale;
   // each transition is worth this much change in value
   const size_t states = geomdata[gridtype].height;
   // FIXME can we not rid ourselves of this meddlesome double?
   double interval = maxy < miny ? 0 : (maxy - miny) / ((double)dimy * states);
   const int startx = labelaxisd ? PREFIXSTRLEN : 0; // plot cols begin here
   // if we want fewer slots than there are available columns, our final column
   // will be other than the plane's final column. most recent x goes here.
   const int finalx = (slotcount < scaleddim - 1 - (startx * scale) ? startx + (slotcount / scale) - 1 : dimx - 1);
   if(labelaxisd){
     // show the *top* of each interval range
     for(int y = 0 ; y < dimy ; ++y){
       char buf[PREFIXSTRLEN + 1];
       ncmetric(interval * states * (y + 1) * 100, 100, buf, 0, 1000, '\0');
       ncplane_putstr_yx(ncp, dimy - y - 1, PREFIXSTRLEN - strlen(buf), buf);
     }
   }
   if(finalx < startx){ // exit on pathologically narrow planes
     return 0;
   }
   if(!interval){
     interval = 1;
   }
   #define MAXWIDTH 2
   int idx = slotstart; // idx holds the real slot index; we move backwards
   for(int x = finalx ; x >= startx ; --x){
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
     double intervalbase = miny;
     const wchar_t* egc = geomdata[gridtype].egcs;
     for(int y = 0 ; y < dimy ; ++y){
       size_t egcidx, sumidx = 0;
       // if we've got at least one interval's worth on the number of positions
       // times the number of intervals per position plus the starting offset,
       // we're going to print *something*
       bool done = !geomdata[gridtype].fill;
       for(int i = 0 ; i < scale ; ++i){
         sumidx *= states;
         if(intervalbase < gvals[i]){
           egcidx = (gvals[i] - intervalbase) / interval;
           if(egcidx >= states){
             egcidx = states - 1;
           }
           done = false;
           sumidx += egcidx;
         }else{
           egcidx = 0;
         }
       }
       if(done){
         break;
       }
       if(ncplane_putwc_yx(ncp, dimy - y - 1, x, egc[sumidx]) <= 0){
         return -1;
       }
       intervalbase += (states * interval);
     }
   }
   if(ncplane_cursor_move_yx(ncp, 0, 0)){
     return -1;
   }
   if(ncplane_stain(ncp, dimy - 1, dimx - 1, maxchannel, maxchannel,
                    minchannel, minchannel) <= 0){
     return -1;
   }
   return 0;
 }

 // if we're doing domain detection, update the domain to reflect the value we
 // just set. if we're not, check the result against the known ranges, and
 // return -1 if the value is outside of that range.
 int update_domain(uint64_t x){
   const uint64_t val = slots[x % slotcount];
   if(detectdomain){
     if(val > maxy){
       maxy = val;
     }
     if(val < miny){
       miny = val;
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
 int window_slide(int64_t x){
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
 uint64_t maxchannel;
 uint64_t minchannel;
 bool vertical_indep; // not yet implemented FIXME
 ncgridgeom_e gridtype;
 // requested number of slots. 0 for automatically setting the number of slots
 // to span the horizontal area. if there are more slots than there are
 // columns, we prefer showing more recent slots to less recent. if there are
 // fewer slots than there are columns, they prefer the left side.
 int rangex;
 // domain minimum and maximum. if detectdomain is true, these are
 // progressively enlarged/shrunk to fit the sample set. if not, samples
 // outside these bounds are counted, but the displayed range covers only this.
 T miny, maxy;
 // sloutcount-element circular buffer of samples. the newest one (rightmost)
 // is at slots[slotstart]; they get older as you go back (and around).
 // elements. slotcount is max(columns, rangex), less label room.
 T* slots;
 int slotcount;
 int slotstart; // index of most recently-written slot
 int64_t slotx; // x value corresponding to slots[slotstart] (newest x)
 bool labelaxisd; // label dependent axis (consumes PREFIXSTRLEN columns)
 bool exponentially; // not yet implemented FIXME
 bool detectdomain; // is domain detection in effect (stretch the domain)?

};
