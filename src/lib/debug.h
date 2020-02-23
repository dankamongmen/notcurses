// this file ought not be installed, nor referenced in releases.
// strictly debugging hacks.

#ifndef NOTCURSES_DEBUG
#define NOTCURSES_DEBUG

#ifdef __cplusplus
extern "C" {
#endif

static inline void
dump_plane_info(const struct notcurses* nc){
  const struct ncplane* p = nc->top;
  int d = 0;
  while(p){
    fprintf(stderr, "%02d] abs: %d/%d len: %d/%d cursor: %d/%d\n", d, p->absy,
            p->absx, p->leny, p->lenx, p->y, p->x);
    ++d;
    p = p->z;
  }
}

#ifdef __cplusplus
}
#endif

#endif
