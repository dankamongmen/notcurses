// Already exported by bindgen: 0
//
//  FUNCTIONS TO REIMPLEMENT:
//  - from notcurses.h: 10
//    - done: 0
//    - remaining: 10
//
// ---------------
// + reimplemented
// # and unit test
// --------------
//
// ncpixel
// ncpixel_a
// ncpixel_b
// ncpixel_g
// ncpixel_r
// ncpixel_set_a
// ncpixel_set_b
// ncpixel_set_g
// ncpixel_set_r
// ncpixel_set_rgb


// use std::ffi::CString;
//
// use crate as ffi;
// use crate::types::{ChannelPair, IntResult};


// // The ncpixel API facilitates direct management of the pixels within an
// // ncvisual (ncvisuals keep a backing store of 32-bit RGBA pixels, and render
// // them down to terminal graphics in ncvisual_render()).
// static inline uint32_t
// ncpixel(int r, int g, int b){
//   if(r < 0) r = 0;
//   if(r > 255) r = 255;
//   if(g < 0) g = 0;
//   if(g > 255) g = 255;
//   if(b < 0) b = 0;
//   if(b > 255) b = 255;
//   return 0xff000000ul | r | (b << 8u) | (g << 16u);
// }
//
// static inline unsigned
// ncpixel_a(uint32_t pixel){
//   return (pixel & 0xff0000fful) >> 24u;
// }
//
// static inline unsigned
// ncpixel_r(uint32_t pixel){
//   return (pixel & 0x000000fful);
// }
//
// static inline int
// ncpixel_g(uint32_t pixel){
//   return (pixel & 0x00ff0000ul) >> 16u;
// }
//
// static inline int
// ncpixel_b(uint32_t pixel){
//   return (pixel & 0x0000ff00ul) >> 8u;
// }
//
// static inline int
// ncpixel_set_a(uint32_t* pixel, int a){
//   if(a > 255 || a < 0){
//     return -1;
//   }
//   *pixel = (*pixel & 0x00fffffful) | (a << 24u);
//   return 0;
// }
//
// static inline int
// ncpixel_set_r(uint32_t* pixel, int r){
//   if(r > 255 || r < 0){
//     return -1;
//   }
//   *pixel = (*pixel & 0xffffff00ul) | r;
//   return 0;
// }
//
// static inline int
// ncpixel_set_g(uint32_t* pixel, int g){
//   if(g > 255 || g < 0){
//     return -1;
//   }
//   *pixel = (*pixel & 0xff00fffful) | (g << 16u);
//   return 0;
// }
//
// static inline int
// ncpixel_set_b(uint32_t* pixel, int b){
//   if(b > 255 || b < 0){
//     return -1;
//   }
//   *pixel = (*pixel & 0xffff00fful) | (b << 8u);
//   return 0;
// }
//
// // set the RGB values of an RGB pixel
// static inline int
// ncpixel_set_rgb(uint32_t* pixel, int r, int g, int b){
//   if(ncpixel_set_r(pixel, r) || ncpixel_set_g(pixel, g) || ncpixel_set_b(pixel, b)){
//     return -1;
//   }
//   return 0;
// }
