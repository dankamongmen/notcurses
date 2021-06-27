#include <fcntl.h>
#include <unistd.h>
#include <ncurses.h> // needed for some definitions, see terminfo(3ncurses)
#include <termios.h>
#include <sys/utsname.h>
#include "internal.h"
#include "input.h"

// we found Sixel support -- set up the API
static inline void
setup_sixel_bitmaps(tinfo* ti, int fd){
  ti->pixel_init = sixel_init;
  ti->pixel_draw = sixel_draw;
  ti->pixel_remove = NULL;
  ti->pixel_destroy = sixel_destroy;
  ti->pixel_wipe = sixel_wipe;
  ti->pixel_shutdown = sixel_shutdown;
  ti->pixel_rebuild = sixel_rebuild;
  ti->sprixel_scale_height = 6;
  sprite_init(ti, fd);
}

static inline void
setup_kitty_bitmaps(tinfo* ti, int fd){
  ti->pixel_wipe = kitty_wipe;
  ti->pixel_destroy = kitty_destroy;
  ti->pixel_remove = kitty_remove;
  ti->pixel_draw = kitty_draw;
  ti->pixel_move = kitty_move;
  ti->pixel_shutdown = kitty_shutdown;
  ti->sprixel_scale_height = 1;
  ti->pixel_rebuild = kitty_rebuild;
  ti->pixel_clear_all = kitty_clear_all;
  set_pixel_blitter(kitty_blit);
  sprite_init(ti, fd);
}

static bool
query_rgb(void){
  bool rgb = tigetflag("RGB") == 1;
  if(!rgb){
    // RGB terminfo capability being a new thing (as of ncurses 6.1), it's not commonly found in
    // terminal entries today. COLORTERM, however, is a de-facto (if imperfect/kludgy) standard way
    // of indicating TrueColor support for a terminal. The variable takes one of two case-sensitive
    // values:
    //
    //   truecolor
    //   24bit
    //
    // https://gist.github.com/XVilka/8346728#true-color-detection gives some more information about
    // the topic
    //
    const char* cterm = getenv("COLORTERM");
    rgb = cterm && (strcmp(cterm, "truecolor") == 0 || strcmp(cterm, "24bit") == 0);
  }
  return rgb;
}

static int
terminfostr(char** gseq, const char* name){
  *gseq = tigetstr(name);
  if(*gseq == NULL || *gseq == (char*)-1){
    *gseq = NULL;
    return -1;
  }
  // terminfo syntax allows a number N of milliseconds worth of pause to be
  // specified using $<N> syntax. this is then honored by tputs(). but we don't
  // use tputs(), instead preferring the much faster stdio+tiparm() (at the
  // expense of terminals which do require these delays). to avoid dumping
  // "$<N>" sequences all over stdio, we chop them out. real text can follow
  // them, so we continue on, copying back once out of the delay.
  char* wnext = NULL; // NULL until we hit a delay, then place to write
  bool indelay = false; // true iff we're in a delay section
  // we consider it a delay as soon as we see '$', and the delay ends at '>'
  for(char* cur = *gseq ; *cur ; ++cur){
    if(!indelay){
      // if we're not in a delay section, make sure we're not starting one,
      // and otherwise copy the current character back (if necessary).
      if(*cur == '$'){
        wnext = cur;
        indelay = true;
      }else{
        if(wnext){
          *wnext++ = *cur;
        }
      }
    }else{
      // we are in a delay section. make sure we're not ending one.
      if(*cur == '>'){
        indelay = false;
      }
    }
  }
  if(wnext){
    *wnext = '\0';
  }
  return 0;
}

// we couldn't get a terminal from interrogation, so let's see if the TERM
// matches any of our known terminals. this can only be as accurate as the
// TERM setting is (and as up-to-date and complete as we are).
static int
match_termname(const char* termname, queried_terminals_e* qterm){
  // https://github.com/alacritty/alacritty/pull/5274 le sigh
  if(strstr(termname, "alacritty")){
    *qterm = TERMINAL_ALACRITTY;
  }
  return 0;
}

void free_terminfo_cache(tinfo* ti){
  ncinputlayer_stop(&ti->input);
  free(ti->termversion);
  free(ti->esctable);
}

// tlen -- size of escape table. tused -- used bytes in same.
// returns -1 if the starting location is >= 65535. otherwise,
// copies tstr into the table, and sets up 1-biased index.
static int
grow_esc_table(tinfo* ti, const char* tstr, escape_e esc,
               size_t* tlen, size_t* tused){
  // the actual table can grow past 64KB, but we can't start there, as
  // we only have 16-bit indices.
  if(*tused >= 65535){
    fprintf(stderr, "Can't add escape %d to full table\n", esc);
    return -1;
  }
  if(ti->escindices[esc] > 0){
    fprintf(stderr, "Already defined escape %d (%s)\n",
            esc, get_escape(ti, esc));
    return -1;
  }
  size_t slen = strlen(tstr) + 1; // count the nul term
  if(*tlen - *tused < slen){
    // guaranteed to give us enough space to add tstr (and then some)
    size_t newsize = *tlen + 4020 + slen; // don't pull two pages ideally
    char* tmp = realloc(ti->esctable, newsize);
    if(tmp == NULL){
      return -1;
    }
    ti->esctable = tmp;
    *tlen = newsize;
  }
  // we now are guaranteed sufficient space to copy tstr
  memcpy(ti->esctable + *tused, tstr, slen);
  ti->escindices[esc] = *tused + 1; // one-bias
  *tused += slen;
  return 0;
}

// Device Attributes; replies with (depending on decTerminalID resource):
//   ⇒  CSI ? 1 ; 2 c  ("VT100 with Advanced Video Option")
//   ⇒  CSI ? 1 ; 0 c  ("VT101 with No Options")
//   ⇒  CSI ? 4 ; 6 c  ("VT132 with Advanced Video and Graphics")
//   ⇒  CSI ? 6 c  ("VT102")
//   ⇒  CSI ? 7 c  ("VT131")
//   ⇒  CSI ? 1 2 ; Ps c  ("VT125")
//   ⇒  CSI ? 6 2 ; Ps c  ("VT220")
//   ⇒  CSI ? 6 3 ; Ps c  ("VT320")
//   ⇒  CSI ? 6 4 ; Ps c  ("VT420")

// query background, replies in X color https://www.x.org/releases/X11R7.7/doc/man/man7/X.7.xhtml#heading11
#define CSI_BGQ "\e]11;?\e\\"

// ought be using the u7 terminfo string here, if it exists. the great thing
// is, if we get a response to this, we know we can use it for u7!
#define DSRCPR "\e[6n"

// we send an XTSMGRAPHICS to set up 256 color registers (the most we can
// currently take advantage of; we need at least 64 to use sixel at all.
// maybe that works, maybe it doesn't. then query both color registers
// and geometry. send XTGETTCAP for terminal name.
static int
send_initial_queries(int fd){
  const char queries[] = CSI_BGQ
                         DSRCPR
                         "\x1b[?2026$p"      // query for App-sync updates
                         "\x1b[=0c"          // Tertiary Device Attributes
                         "\x1b[>0q"          // XTVERSION
                         "\x1bP+q544e\x1b\\" // XTGETTCAP['TN']
                         "\x1b[?1;3;256S"    // try to set 256 cregs
                         "\x1b[?2;1;0S"      // XTSMGRAPHICS (cregs)
                         "\x1b[?1;1;0S"      // XTSMGRAPHICS (geometry)
                         "\x1b[c";           // Device Attributes
  if(blocking_write(fd, queries, strlen(queries))){
    return -1;
  }
  return 0;
}

static int
init_terminfo_esc(tinfo* ti, const char* name, escape_e idx,
                  size_t* tablelen, size_t* tableused){
  char* tstr;
  if(terminfostr(&tstr, name) == 0){
    if(grow_esc_table(ti, tstr, idx, tablelen, tableused)){
      return -1;
    }
  }else{
    ti->escindices[idx] = 0;
  }
  return 0;
}

// if we get a response to the standard cursor locator escape, we know this
// terminal supports it, hah.
static int
add_u7_escape(tinfo* ti, size_t* tablelen, size_t* tableused){
  const char* u7 = get_escape(ti, ESCAPE_DSRCPR);
  if(u7){
    return 0; // already present
  }
  if(grow_esc_table(ti, DSRCPR, ESCAPE_DSRCPR, tablelen, tableused)){
    return -1;
  }
  return 0;
}

static int
add_smulx_escapes(tinfo* ti, size_t* tablelen, size_t* tableused){
  if(grow_esc_table(ti, "\x1b[4:3m", ESCAPE_SMULX, tablelen, tableused) ||
     grow_esc_table(ti, "\x1b[4:0m", ESCAPE_SMULNOX, tablelen, tableused)){
    return -1;
  }
  return 0;
}

static int
add_appsync_escapes(tinfo* ti, size_t* tablelen, size_t* tableused){
  if(grow_esc_table(ti, "\x1bP=1s\x1b\\", ESCAPE_BSU, tablelen, tableused) ||
     grow_esc_table(ti, "\x1bP=2s\x1b\\", ESCAPE_ESU, tablelen, tableused)){
    return -1;
  }
  return 0;
}

// Qui si convien lasciare ogne sospetto; ogne viltà convien che qui sia morta.
static int
apply_term_heuristics(tinfo* ti, const char* termname, int fd,
                      queried_terminals_e qterm,
                      size_t* tablelen, size_t* tableused){
  if(!termname){
    // setupterm interprets a missing/empty TERM variable as the special value “unknown”.
    termname = "unknown";
  }
  if(qterm == TERMINAL_UNKNOWN){
    match_termname(termname, &qterm);
  }
  // st had neithercaps.sextants nor caps.quadrants last i checked (0.8.4)
  ti->caps.braille = true; // most everyone has working caps.braille, even from fonts
  if(qterm == TERMINAL_KITTY){ // kitty (https://sw.kovidgoyal.net/kitty/)
    termname = "Kitty";
    // see https://sw.kovidgoyal.net/kitty/protocol-extensions.html
    ti->bg_collides_default |= 0x1000000;
    ti->caps.sextants = true; // work since bugfix in 0.19.3
    ti->caps.quadrants = true;
    ti->caps.rgb = true;
    setup_kitty_bitmaps(ti, fd);
    if(add_smulx_escapes(ti, tablelen, tableused)){
      return -1;
    }
  }else if(qterm == TERMINAL_ALACRITTY){
    termname = "Alacritty";
    ti->caps.quadrants = true;
    // ti->caps.sextants = true; // alacritty https://github.com/alacritty/alacritty/issues/4409 */
    ti->caps.rgb = true;
  }else if(qterm == TERMINAL_VTE){
    termname = "VTE";
    ti->caps.quadrants = true;
    ti->caps.sextants = true; // VTE has long enjoyed good sextant support
    if(add_smulx_escapes(ti, tablelen, tableused)){
      return -1;
    }
  }else if(qterm == TERMINAL_FOOT){
    termname = "foot";
    ti->caps.sextants = true;
    ti->caps.quadrants = true;
    ti->caps.rgb = true;
  }else if(qterm == TERMINAL_MLTERM){
    termname = "MLterm";
    ti->caps.quadrants = true; // good caps.quadrants, no caps.sextants as of 3.9.0
    ti->sprixel_cursor_hack = true;
  }else if(qterm == TERMINAL_WEZTERM){
    termname = "WezTerm";
    ti->caps.rgb = true;
    ti->caps.quadrants = true;
    if(ti->termversion && strcmp(ti->termversion, "20210610") >= 0){
      ti->caps.sextants = true; // good caps.sextants as of 2021-06-10
      if(add_smulx_escapes(ti, tablelen, tableused)){
        return -1;
      }
    }else{
      termname = "XTerm";
    }
  }else if(qterm == TERMINAL_XTERM){
    termname = "XTerm";
  }else if(qterm == TERMINAL_CONTOUR){
    termname = "Contour";
    ti->caps.quadrants = true;
    ti->caps.rgb = true;
  }else if(qterm == TERMINAL_LINUX){
    struct utsname un;
    if(uname(&un) == 0){
      ti->termversion = strdup(un.release);
    }
    termname = "Linux console";
    ti->caps.braille = false; // no caps.braille, no caps.sextants in linux console
  }
  // run a wcwidth(⣿) to guarantee libc Unicode 3 support, independent of term
  if(wcwidth(L'⣿') < 0){
    ti->caps.braille = false;
  }
  // run a wcwidth(🬸) to guarantee libc Unicode 13 support, independent of term
  if(wcwidth(L'🬸') < 0){
    ti->caps.sextants = false;
  }
  ti->termname = termname;
  return 0;
}

// some terminals cannot combine certain styles with colors, as expressed in
// the "ncv" terminfo capability (using ncurses-style constants). don't
// advertise support for the style in that case. otherwise, if the style is
// supported, OR it into supported_styles (using Notcurses-style constants).
static void
build_supported_styles(tinfo* ti){
  const struct style {
    unsigned s;        // NCSTYLE_* value
    int esc;           // ESCAPE_* value for enable
    const char* tinfo; // terminfo capability for conditional permit
    unsigned ncvbit;   // bit in "ncv" mask for unconditional deny
  } styles[] = {
    { NCSTYLE_BOLD, ESCAPE_BOLD, "bold", A_BOLD },
    { NCSTYLE_UNDERLINE, ESCAPE_SMUL, "smul", A_UNDERLINE },
    { NCSTYLE_ITALIC, ESCAPE_SITM, "sitm", A_ITALIC },
    { NCSTYLE_STRUCK, ESCAPE_SMXX, "smxx", 0 },
    { NCSTYLE_BLINK, ESCAPE_BLINK, "blink", A_BLINK },
    { NCSTYLE_UNDERCURL, ESCAPE_SMULX, "Smulx", 0 },
    { 0, 0, NULL, 0 }
  };
  int nocolor_stylemask = tigetnum("ncv");
  for(typeof(*styles)* s = styles ; s->s ; ++s){
    if(get_escape(ti, s->esc)){
      if(nocolor_stylemask > 0){
        if(nocolor_stylemask & s->ncvbit){
          ti->escindices[s->esc] = 0;
          continue;
        }
      }
      ti->supported_styles |= s->s;
    }
  }
}

// termname is just the TERM environment variable. some details are not
// exposed via terminfo, and we must make heuristic decisions based on
// the detected terminal type, yuck :/.
// the first thing we do is fire off any queries we have (XTSMGRAPHICS, etc.)
// with a trailing Device Attributes. all known terminals will reply to a
// Device Attributes, allowing us to get a negative response if our queries
// aren't supported by the terminal. we fire it off early because we have a
// full round trip before getting the reply, which is likely to pace init.
int interrogate_terminfo(tinfo* ti, int fd, const char* termname, unsigned utf8,
                         unsigned noaltscreen, unsigned nocbreak, unsigned nonewfonts,
                         int* cursor_y, int* cursor_x){
  queried_terminals_e qterm = TERMINAL_UNKNOWN;
  memset(ti, 0, sizeof(*ti));
  // we might or might not program quadrants into the console font
  if(is_linux_console(fd, nonewfonts, &ti->caps.quadrants)){
    qterm = TERMINAL_LINUX;
  }
  if(fd >= 0){
    if(qterm == TERMINAL_UNKNOWN){
      if(send_initial_queries(fd)){
        fprintf(stderr, "Error issuing terminal queries on %d\n", fd);
        return -1;
      }
    }
    if(tcgetattr(fd, &ti->tpreserved)){
      fprintf(stderr, "Couldn't preserve terminal state for %d (%s)\n", fd, strerror(errno));
      return -1;
    }
    // enter cbreak mode regardless of user preference until we've performed
    // terminal interrogation. at that point, we might restore original mode.
    if(cbreak_mode(fd, &ti->tpreserved)){
      return -1;
    }
  }
  ti->caps.utf8 = utf8;
  // allow the "rgb" boolean terminfo capability, a COLORTERM environment
  // variable of either "truecolor" or "24bit", or unconditionally enable it
  // for several terminals known to always support 8bpc rgb setaf/setab.
  int colors = tigetnum("colors");
  if(colors <= 0){
    ti->caps.colors = 1;
  }else{
    ti->caps.colors = colors;
  }
  ti->caps.rgb = query_rgb(); // independent of colors
  // verify that the terminal provides cursor addressing (absolute movement)
  const struct strtdesc {
    escape_e esc;
    const char* tinfo;
  } strtdescs[] = {
    { ESCAPE_CUP, "cup", },
    { ESCAPE_HPA, "hpa", },
    { ESCAPE_VPA, "vpa", },
    // Not all terminals support setting the fore/background independently
    { ESCAPE_SETAF, "setaf", },
    { ESCAPE_SETAB, "setab", },
    { ESCAPE_OP, "op", },
    { ESCAPE_CNORM, "cnorm", },
    { ESCAPE_CIVIS, "civis", },
    { ESCAPE_SGR0, "sgr0", },
    { ESCAPE_SITM, "sitm", },
    { ESCAPE_RITM, "ritm", },
    { ESCAPE_BOLD, "bold", },
    { ESCAPE_BLINK, "blink", },
    { ESCAPE_CUD, "cud", },
    { ESCAPE_CUU, "cuu", },
    { ESCAPE_CUF, "cuf", },
    { ESCAPE_CUF1, "cuf1", },
    { ESCAPE_CUB, "cub", },
    { ESCAPE_INITC, "initc", },
    { ESCAPE_GETM, "getm", },
    { ESCAPE_SMKX, "smkx", },
    { ESCAPE_SMXX, "smxx", },
    { ESCAPE_RMXX, "rmxx", },
    { ESCAPE_SMUL, "smul", },
    { ESCAPE_RMUL, "rmul", },
    { ESCAPE_SC, "sc", },
    { ESCAPE_RC, "rc", },
    { ESCAPE_CLEAR, "clear", },
    { ESCAPE_HOME, "home", },
    { ESCAPE_OC, "oc", },
    { ESCAPE_RMKX, "rmkx", },
    { ESCAPE_DSRCPR, "u7", },
    { ESCAPE_MAX, NULL, },
  };
  size_t tablelen = 0;
  size_t tableused = 0;
  for(typeof(*strtdescs)* strtdesc = strtdescs ; strtdesc->esc < ESCAPE_MAX ; ++strtdesc){
    if(init_terminfo_esc(ti, strtdesc->tinfo, strtdesc->esc, &tablelen, &tableused)){
      goto err;
    }
  }
  if(ti->escindices[ESCAPE_CUP] == 0){
    fprintf(stderr, "Required terminfo capability 'cup' not defined\n");
    goto err;
  }
  if(colors){
    const char* initc = get_escape(ti, ESCAPE_INITC);
    if(initc){
      ti->caps.can_change_colors = tigetflag("ccc") == 1;
    }
  }else{ // disable initc if there's no color support
    ti->escindices[ESCAPE_INITC] = 0;
  }
  // neither of these is supported on e.g. the "linux" virtual console.
  if(!noaltscreen){
    if(init_terminfo_esc(ti, "smcup", ESCAPE_SMCUP, &tablelen, &tableused) ||
       init_terminfo_esc(ti, "rmcup", ESCAPE_RMCUP, &tablelen, &tableused)){
      goto err;
    }
  }else{
    ti->escindices[ESCAPE_SMCUP] = 0;
    ti->escindices[ESCAPE_RMCUP] = 0;
  }
  // ensure that the terminal provides automatic margins
  if(tigetflag("am") != 1){
    fprintf(stderr, "Required terminfo capability 'am' not defined\n");
    goto err;
  }
  if(get_escape(ti, ESCAPE_CIVIS) == NULL){
    char* chts;
    if(terminfostr(&chts, "chts") == 0){
      if(grow_esc_table(ti, chts, ESCAPE_CIVIS, &tablelen, &tableused)){
        goto err;
      }
    }
  }
  if(get_escape(ti, ESCAPE_BOLD)){
    if(grow_esc_table(ti, "\e[22m", ESCAPE_NOBOLD, &tablelen, &tableused)){
      goto err;
    }
  }
  if(get_escape(ti, ESCAPE_BLINK)){
    if(grow_esc_table(ti, "\e[25m", ESCAPE_NOBLINK, &tablelen, &tableused)){
      goto err;
    }
  }
  // if the keypad neen't be explicitly enabled, smkx is not present
  const char* smkx = get_escape(ti, ESCAPE_SMKX);
  if(smkx && fd >= 0){
    if(tty_emit(tiparm(smkx), fd) < 0){
      fprintf(stderr, "Error entering keypad transmit mode\n");
      goto err;
    }
  }
  // if op is defined as ansi 39 + ansi 49, make the split definitions
  // available. this ought be asserted by extension capability "ax", but
  // no terminal i've found seems to do so. =[
  const char* op = get_escape(ti, ESCAPE_OP);
  if(op && strcmp(op, "\x1b[39;49m") == 0){
    if(grow_esc_table(ti, "\x1b[39m", ESCAPE_FGOP, &tablelen, &tableused) ||
       grow_esc_table(ti, "\x1b[49m", ESCAPE_BGOP, &tablelen, &tableused)){
      goto err;
    }
  }
  unsigned appsync_advertised;
  if(cursor_x && cursor_y){
    *cursor_x = *cursor_y = 0;
  }
  if(ncinputlayer_init(ti, stdin, &qterm, &appsync_advertised, cursor_y, cursor_x)){
    goto err;
  }
  if(nocbreak){
    if(fd >= 0){
      if(tcsetattr(fd, TCSANOW, &ti->tpreserved)){
        ncinputlayer_stop(&ti->input);
        goto err;
      }
    }
  }
  if(cursor_x && cursor_y){
    if(*cursor_x >= 0 && *cursor_y >= 0){
      if(add_u7_escape(ti, &tablelen, &tableused)){
        return -1;
      }
    }
  }
  if(appsync_advertised){
    if(add_appsync_escapes(ti, &tablelen, &tableused)){
      goto err;
    }
  }
  if(apply_term_heuristics(ti, termname, fd, qterm, &tablelen, &tableused)){
    ncinputlayer_stop(&ti->input);
    goto err;
  }
  build_supported_styles(ti);
  // our current sixel quantization algorithm requires at least 64 color
  // registers. we make use of no more than 256. this needs to happen
  // after heuristics, since sixel_init() depends on sprixel_cursor_hack.
  if(ti->color_registers >= 64){
    setup_sixel_bitmaps(ti, fd);
  }
  return 0;

err:
  free(ti->esctable);
  free(ti->termversion);
  return -1;
}
