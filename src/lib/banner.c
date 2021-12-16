#include "internal.h"
#include <curses.h>
#ifdef USE_DEFLATE
#include <libdeflate.h>
#else
#include <zlib.h>
#endif

// only invoked without suppress banners flag. prints various warnings based on
// the environment / terminal definition. returns the number of lines printed.
static int
init_banner_warnings(const notcurses* nc, fbuf* f, const char* clreol){
  term_fg_palindex(nc, f, nc->tcache.caps.colors <= 88 ? 1 : 0xcb);
  if(!notcurses_canutf8(nc)){
    fbuf_puts(f, clreol);
    fbuf_puts(f, " Warning! Encoding is not UTF-8; output may be degraded." NL);
  }
  if(!get_escape(&nc->tcache, ESCAPE_HPA)){
    fbuf_puts(f, clreol);
    fbuf_puts(f, " Warning! No absolute horizontal placement." NL);
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
  if(!(nc->flags & NCOPTION_SUPPRESS_BANNERS)){
    term_fg_palindex(nc, f, 50 % nc->tcache.caps.colors);
    char* osver = notcurses_osversion();
    fbuf_printf(f, "%snotcurses %s on %s %s%s(%s)" NL,
                clreol, notcurses_version(),
                nc->tcache.termname ? nc->tcache.termname : "?",
                nc->tcache.termversion ? nc->tcache.termversion : "",
                nc->tcache.termversion ? " " : "", osver ? osver : "unknown");
    free(osver);
    term_fg_palindex(nc, f, nc->tcache.caps.colors <= 256 ?
                     14 % nc->tcache.caps.colors : 0x2080e0);
    if(nc->tcache.cellpxy && nc->tcache.cellpxx){
      fbuf_printf(f, "%s%d rows (%dpx) %d cols (%dpx) %dx%d ",
                  clreol,
                  nc->stdplane->leny, nc->tcache.cellpxy,
                  nc->stdplane->lenx, nc->tcache.cellpxx,
                  nc->stdplane->leny * nc->tcache.cellpxy,
                  nc->stdplane->lenx * nc->tcache.cellpxx);
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
    // we want the terminfo version, which is tied to ncurses
    const char* ncursesver = curses_version();
    const char* ncver = strchr(ncursesver, ' ');
    ncver = ncver ? ncver + 1 : ncursesver;
#ifdef USE_DEFLATE
    fbuf_printf(f, "%u colors" NL "%s%s%s (%s)" NL "%sterminfo %s libdeflate %s GPM %s" NL,
                nc->tcache.caps.colors, clreol,
#else
    fbuf_printf(f, "%u colors" NL "%s%s%s (%s)" NL "%sterminfo %s zlib %s GPM %s" NL,
                nc->tcache.caps.colors, clreol,
#endif
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
            clreol, ncver,
#ifdef USE_DEFLATE
            LIBDEFLATE_VERSION_STRING,
#else
            ZLIB_VERSION,
#endif
            gpm_version());
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

