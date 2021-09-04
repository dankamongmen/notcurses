#include <zlib.h>
#include "internal.h"

// only invoked without suppress banners flag. prints various warnings based on
// the environment / terminal definition. returns the number of lines printed.
static int
init_banner_warnings(const notcurses* nc, fbuf* f, const char* clreol){
  term_fg_palindex(nc, f, nc->tcache.caps.colors <= 88 ? 1 : 0xcb);
  if(!notcurses_canutf8(nc)){
    fbuf_puts(f, clreol);
    fbuf_puts(f, " Warning! Encoding is not UTF-8; output may be degraded.\n");
  }
  if(!get_escape(&nc->tcache, ESCAPE_HPA)){
    fbuf_puts(f, clreol);
    fbuf_puts(f, " Warning! No absolute horizontal placement.\n");
  }
  return 0;
}

// unless the suppress_banner flag was set, print some version information and
// (if applicable) warnings to ttyfp. we are not yet on the alternate screen.
int init_banner(const notcurses* nc, fbuf* f){
  const char* clreol = get_escape(&nc->tcache, ESCAPE_EL);
  if(clreol == NULL){
    clreol = "";
  }
  if(!nc->suppress_banner){
    term_fg_palindex(nc, f, 50 % nc->tcache.caps.colors);
    fbuf_printf(f, "%snotcurses %s on %s %s\n", notcurses_version(),
                clreol,
                nc->tcache.termname ? nc->tcache.termname : "?",
                nc->tcache.termversion ? nc->tcache.termversion : "");
    term_fg_palindex(nc, f, nc->tcache.caps.colors <= 256 ?
                     14 % nc->tcache.caps.colors : 0x2080e0);
    if(nc->tcache.cellpixy && nc->tcache.cellpixx){
      fbuf_printf(f, "%s%d rows (%dpx) %d cols (%dpx) %dx%d ",
                  clreol,
                  nc->stdplane->leny, nc->tcache.cellpixy,
                  nc->stdplane->lenx, nc->tcache.cellpixx,
                  nc->stdplane->leny * nc->tcache.cellpixy,
                  nc->stdplane->lenx * nc->tcache.cellpixx);
    }else{
      fbuf_printf(f, "%d rows %d cols ",
                  nc->stdplane->leny, nc->stdplane->lenx);
    }
    if(nc->tcache.caps.rgb && get_escape(&nc->tcache, ESCAPE_SETAF)){
      term_fg_rgb8(&nc->tcache, f, 0xe0, 0x60, 0x60);
      fbuf_putc(f, 'r');
      term_fg_rgb8(&nc->tcache, f, 0x60, 0xe0, 0x60);
      fbuf_putc(f, 'g');
      term_fg_rgb8(&nc->tcache, f, 0x20, 0x80, 0xff);
      fbuf_putc(f, 'b');
      term_fg_palindex(nc, f, nc->tcache.caps.colors <= 256 ?
                       14 % nc->tcache.caps.colors : 0x2080e0);
      fbuf_putc(f, '+');
    }
    fbuf_printf(f, "%u colors\n%s%s%s (%s)\n%sterminfo from %s zlib %s GPM %s\n",
                nc->tcache.caps.colors, clreol,
#ifdef __clang__
            "", // name is contained in __VERSION__
#else
#ifdef __GNUC__
            "gcc-",
#else
#error "Unknown compiler"
#endif
#endif
            __VERSION__,
#ifdef __BYTE_ORDER__
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            "LE",
#else
            "BE",
#endif
#else
#error "No __BYTE_ORDER__ definition"
#endif
            clreol, curses_version(), zlibVersion(), gpm_version());
    fbuf_puts(f, clreol); // for ncvisual banner
    ncvisual_printbanner(f);
    init_banner_warnings(nc, f, clreol);
    const char* esc;
    if( (esc = get_escape(&nc->tcache, ESCAPE_SGR0)) ||
        (esc = get_escape(&nc->tcache, ESCAPE_OP))){
      fbuf_emit(f, esc);
    }
  }
  return 0;
}

