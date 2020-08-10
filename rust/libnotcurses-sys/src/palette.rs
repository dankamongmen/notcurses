// Already exported by bindgen: 3
//
// palette256_free
// palette256_new
// palette256_use
//
//  FUNCTIONS TO REIMPLEMENT:
//  - from notcurses.h: 3
//    - done: 0
//    - remaining: 3
//
// ---------------
// + reimplemented
// # and unit test
// --------------
// palette256_get_rgb
// palette256_set
// palette256_set_rgb


// // Manipulate entries in the palette store 'p'. These are *not* locked.
// static inline int
// palette256_set_rgb(palette256* p, int idx, int r, int g, int b){
//   if(idx < 0 || (size_t)idx > sizeof(p->chans) / sizeof(*p->chans)){
//     return -1;
//   }
//   return channel_set_rgb(&p->chans[idx], r, g, b);
// }
//
// static inline int
// palette256_set(palette256* p, int idx, unsigned rgb){
//   if(idx < 0 || (size_t)idx > sizeof(p->chans) / sizeof(*p->chans)){
//     return -1;
//   }
//   return channel_set(&p->chans[idx], rgb);
// }
//
// static inline int
// palette256_get_rgb(const palette256* p, int idx, unsigned* RESTRICT r, unsigned* RESTRICT g, unsigned* RESTRICT b){
//   if(idx < 0 || (size_t)idx > sizeof(p->chans) / sizeof(*p->chans)){
//     return -1;
//   }
//   return channel_rgb(p->chans[idx], r, g, b);
// }
