// Already exported by bindgen: 0
//
//  FUNCTIONS TO REIMPLEMENT:
//  - from notcurses.h: 2
//    - done: 0
//    - remaining: 2
//
// ---------------
// + reimplemented
// # and unit test
// --------------
// nckey_mouse_p
// nckey_supppuab_p


// use crate as ffi;
// use crate::types::{ChannelPair, IntResult};


// // Is this char32_t a Supplementary Private Use Area-B codepoint?
// static inline bool
// nckey_supppuab_p(char32_t w){
//   return w >= 0x100000 && w <= 0x10fffd;
// }
//
// // Is the event a synthesized mouse event?
// static inline bool
// nckey_mouse_p(char32_t r){
//   return r >= NCKEY_BUTTON1 && r <= NCKEY_RELEASE;
// }
