// functions already exported by bindgen : 35
// ------------------------------------------
// notcurses_at_yx
// notcurses_canchangecolor
// notcurses_canfade
// notcurses_canopen_images
// notcurses_canopen_videos
// notcurses_cansixel
// notcurses_cantruecolor
// notcurses_canutf8
// notcurses_cursor_disable
// notcurses_cursor_enable
// notcurses_debug
// notcurses_drop_planes
// notcurses_getc
// notcurses_init
// notcurses_inputready_fd
// notcurses_lex_blitter
// notcurses_lex_margins
// notcurses_lex_scalemode
// notcurses_mouse_disable
// notcurses_mouse_enable
// notcurses_palette_size
// notcurses_refresh
// notcurses_render
// notcurses_render_to_file
// notcurses_reset_stats
// notcurses_stats
// notcurses_stdplane
// notcurses_stdplane_const
// notcurses_stop
// notcurses_str_blitter
// notcurses_str_scalemode
// notcurses_supported_styles
// notcurses_top
// notcurses_version
// notcurses_version_components
//
// static inline functions to reimplement: 4
// -----------------------------------------
// - finished : 0
// - remaining: 4
// --------------- (+) implemented (#) + unit test (x) wont implement
// notcurses_getc_blocking
// notcurses_getc_nblock
// notcurses_stddim_yx
// notcurses_term_dim_yx


// use crate as ffi;
// use crate::types::{ChannelPair, IntResult};


// // 'ni' may be NULL if the caller is uninterested in event details. If no event
// // is ready, returns 0.
// static inline char32_t
// notcurses_getc_nblock(struct notcurses* n, ncinput* ni){
//   sigset_t sigmask;
//   sigfillset(&sigmask);
//   struct timespec ts = { .tv_sec = 0, .tv_nsec = 0 };
//   return notcurses_getc(n, &ts, &sigmask, ni);
// }
//
// // 'ni' may be NULL if the caller is uninterested in event details. Blocks
// // until an event is processed or a signal is received.
// static inline char32_t
// notcurses_getc_blocking(struct notcurses* n, ncinput* ni){
//   sigset_t sigmask;
//   sigemptyset(&sigmask);
//   return notcurses_getc(n, NULL, &sigmask, ni);
// }
//
// // notcurses_stdplane(), plus free bonus dimensions written to non-NULL y/x!
// static inline struct ncplane*
// notcurses_stddim_yx(struct notcurses* nc, int* RESTRICT y, int* RESTRICT x){
//   struct ncplane* s = notcurses_stdplane(nc); // can't fail
//   ncplane_dim_yx(s, y, x); // accepts NULL
//   return s;
// }
//
// // Return our current idea of the terminal dimensions in rows and cols.
// static inline void
// notcurses_term_dim_yx(const struct notcurses* n, int* RESTRICT rows, int* RESTRICT cols){
//   ncplane_dim_yx(notcurses_stdplane_const(n), rows, cols);
// }

