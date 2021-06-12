#include <fcntl.h>
#include <ncurses.h> // needed for some definitions, see terminfo(3ncurses)
#include "internal.h"

// we found Sixel support -- set up the API
static inline void
setup_sixel_bitmaps(tinfo* ti){
  ti->bitmap_supported = true;
  ti->color_registers = 256;  // assumed default [shrug]
  ti->pixel_init = sixel_init;
  ti->pixel_draw = sixel_draw;
  ti->sixel_maxx = 4096; // whee!
  ti->sixel_maxy = 4096;
  ti->pixel_remove = NULL;
  ti->pixel_destroy = sixel_destroy;
  ti->pixel_wipe = sixel_wipe;
  ti->pixel_shutdown = sixel_shutdown;
  ti->pixel_rebuild = sixel_rebuild;
  ti->sprixel_scale_height = 6;
}

static inline void
setup_kitty_bitmaps(tinfo* ti, int fd){
  ti->bitmap_supported = true;
  ti->pixel_wipe = kitty_wipe;
  ti->pixel_destroy = kitty_destroy;
  ti->pixel_remove = kitty_remove;
  ti->pixel_draw = kitty_draw;
  ti->pixel_shutdown = kitty_shutdown;
  ti->sprixel_scale_height = 1;
  ti->pixel_rebuild = kitty_rebuild;
  ti->pixel_clear_all = kitty_clear_all;
  ti->bitmap_lowest_line = true;
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
  // "$<N>" sequences all over stdio, we chop them out.
  char* pause;
  if( (pause = strchr(*gseq, '$')) ){
    // FIXME can there ever be further content following a pause?
    // tighten this up to match the precise spec in terminfo(5)!
    *pause = '\0';
  }
  return 0;
}

// Qui si convien lasciare ogne sospetto; ogne viltà convien che qui sia morta.
static int
apply_term_heuristics(tinfo* ti, const char* termname, int fd){
  if(!termname){
    // setupterm interprets a missing/empty TERM variable as the special value “unknown”.
    termname = "unknown";
  }
  ti->braille = true; // most everyone has working braille, even from fonts
  if(strstr(termname, "kitty")){ // kitty (https://sw.kovidgoyal.net/kitty/)
    termname = "Kitty";
    // see https://sw.kovidgoyal.net/kitty/protocol-extensions.html
    // FIXME detect the actual default background color; this assumes it to
    // be RGB(0, 0, 0) (the default). we could also just set it, i guess.
    ti->bg_collides_default = 0x1000000;
    ti->sextants = true; // work since bugfix in 0.19.3
    ti->quadrants = true;
    ti->RGBflag = true;
    setup_kitty_bitmaps(ti, fd);
  }else if(strstr(termname, "alacritty")){
    termname = "Alacritty";
    ti->alacritty_sixel_hack = true;
    ti->quadrants = true;
    // ti->sextants = true; // alacritty https://github.com/alacritty/alacritty/issues/4409 */
    ti->RGBflag = true;
  }else if(strstr(termname, "vte") || strstr(termname, "gnome") || strstr(termname, "xfce")){
    termname = "VTE";
    ti->sextants = true; // VTE has long enjoyed good sextant support
    ti->quadrants = true;
  }else if(strncmp(termname, "foot", 4) == 0){
    termname = "foot";
    ti->sextants = true;
    ti->quadrants = true;
    ti->RGBflag = true;
  }else if(strncmp(termname, "st", 2) == 0){
    termname = "simple terminal";
    // st had neithersextants nor quadrants last i checked (0.8.4)
  }else if(strstr(termname, "mlterm")){
    termname = "MLterm";
    ti->quadrants = true; // good quadrants, no sextants as of 3.9.0
    ti->sprixel_cursor_hack = true;
  }else if(strstr(termname, "xterm")){
    // xterm has nothing beyond halfblocks. this is going to catch all kinds
    // of people using xterm when they shouldn't be, or even real database
    // entries like "xterm-kitty" (if we don't catch them above), giving a
    // pretty minimal (but safe) experience. set your TERM correctly!
    // wezterm wants a TERM of xterm-256color, and identifies itself based
    // off TERM_PROGRAM and TERM_PROGRAM_VERSION.
    const char* term_program = getenv("TERM_PROGRAM");
    if(term_program && strcmp(term_program, "WezTerm") == 0){
      termname = "WezTerm";
      ti->quadrants = true;
      const char* termver = getenv("TERM_PROGRAM_VERSION");
      if(termver && strcmp(termver, "20210610") >= 0){
        ti->sextants = true; // good sextants as of 2021-06-10
      }
    }else{
      termname = "XTerm";
    }
  }else if(strcmp(termname, "linux") == 0){
    termname = "Linux console";
    ti->braille = false; // no braille, no sextants in linux console
    // FIXME if the NCOPTION_NO_FONT_CHANGES, this isn't true
    // FIXME we probably want to do this based off ioctl()s in linux.c
    ti->quadrants = true; // we program quadrants on the console
  }
  // run a wcwidth(⣿) to guarantee libc Unicode 3 support, independent of term
  if(wcwidth(L'⣿') < 0){
    ti->braille = false;
  }
  // run a wcwidth(🬸) to guarantee libc Unicode 13 support, independent of term
  if(wcwidth(L'🬸') < 0){
    ti->sextants = false;
  }
  ti->termname = termname;
  return 0;
}

void free_terminfo_cache(tinfo* ti){
  free(ti->esctable);
  ncinputlayer_stop(&ti->input);
}

// tlen -- size of escape table. tused -- used bytes in same.
// returns -1 if the starting location is >= 65535. otherwise,
// copies tstr into the table, and sets up 1-biased index.
static int
grow_esc_table(tinfo* ti, const char* tstr, escape_e esc,
               size_t* tlen, size_t* tused){
  if(*tused >= 65535){
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
#define ESC_DA "\e[c"

/*
// query for Sixel details including the number of color registers and, one day
// perhaps, maximum geometry. xterm binds its return by the current geometry,
// making it useless for a one-time query.
static int
query_sixel_details(tinfo* ti, int fd){
  if(query_xtsmgraphics(fd, "\x1b[?2;1;0S" ESC_DA, &ti->sixel_maxx, &ti->sixel_maxy)){
    return -1;
  }
  if(query_xtsmgraphics(fd, "\x1b[?1;1;0S" ESC_DA, &ti->color_registers, NULL)){
    return -1;
  }
//fprintf(stderr, "Sixel ColorRegs: %d Max_x: %d Max_y: %d\n", ti->color_registers, ti->sixel_maxx, ti->sixel_maxy);
  if(ti->color_registers < 64){ // FIXME try to drive it higher
    return -1;
  }
  return 0;
}
*/

// we send an XTSMGRAPHICS to set up 256 color registers (the most we can
// currently take advantage of; we need at least 64 to use sixel at all.
// maybe that works, maybe it doesn't. then query both color registers
// and geometry. send XTGETTCAP for terminal name.
static int
send_initial_queries(int fd){
  const char queries[] = "\x1b[=0c\x1b[>c\x1b[>q\x1bP+q544e\x1b\\\x1b[?1;3;256S\x1b[?2;1;0S\x1b[?1;1;0S" ESC_DA;
  if(blocking_write(fd, queries, strlen(queries))){
    return -1;
  }
  return 0;
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
                         unsigned noaltscreen, unsigned nocbreak){
  memset(ti, 0, sizeof(*ti));
  if(fd >= 0){
    if(send_initial_queries(fd)){
      fprintf(stderr, "Error issuing terminal queries on %d\n", fd);
      return -1;
    }
    if(tcgetattr(fd, &ti->tpreserved)){
      fprintf(stderr, "Couldn't preserve terminal state for %d (%s)\n", fd, strerror(errno));
      return -1;
    }
  }
  ti->utf8 = utf8;
  if(!nocbreak){
    if(cbreak_mode(fd, &ti->tpreserved)){
      return -1;
    }
  }
  // allow the "rgb" boolean terminfo capability, a COLORTERM environment
  // variable of either "truecolor" or "24bit", or unconditionally enable it
  // for several terminals known to always support 8bpc rgb setaf/setab.
  int colors = tigetnum("colors");
  if(colors <= 0){
    ti->colors = 1;
  }else{
    ti->colors = colors;
  }
  ti->RGBflag = query_rgb(); // independent of colors
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
    { ESCAPE_SGR, "sgr", },
    { ESCAPE_SGR0, "sgr0", },
    { ESCAPE_SITM, "sitm", },
    { ESCAPE_RITM, "ritm", },
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
    { ESCAPE_SC, "sc", },
    { ESCAPE_RC, "rc", },
    { ESCAPE_CLEAR, "clear", },
    { ESCAPE_HOME, "home", },
    { ESCAPE_OC, "oc", },
    { ESCAPE_RMKX, "rmkx", },
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
      ti->CCCflag = tigetflag("ccc") == 1;
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
  // check that the terminal provides automatic margins
  ti->AMflag = tigetflag("am") == 1;
  if(!ti->AMflag){
    fprintf(stderr, "Required terminfo capability 'am' not defined\n");
    goto err;
  }
  ti->BCEflag = tigetflag("bce") == 1;
  if(get_escape(ti, ESCAPE_CIVIS) == NULL){
    char* chts;
    if(terminfostr(&chts, "chts") == 0){
      if(grow_esc_table(ti, chts, ESCAPE_CIVIS, &tablelen, &tableused)){
        goto err;
      }
    }
  }
  // we don't actually use the bold capability -- we use sgr exclusively.
  // but we use the presence of the bold capability to determine whether
  // we think sgr supports bold, which...might be valid? i'm unsure. futher,
  // some terminals cannot combine certain styles with colors. don't
  // advertise support for the style in that case.
  const struct style {
    unsigned s;        // NCSTYLE_* value
    const char* tinfo; // terminfo capability for conditional permit
    unsigned ncvbit;   // bit in "ncv" mask for unconditional deny
  } styles[] = {
    { NCSTYLE_BOLD, "bold", A_BOLD },
    { NCSTYLE_STANDOUT, "smso", A_STANDOUT },
    { NCSTYLE_REVERSE, "rev", A_REVERSE },
    { NCSTYLE_UNDERLINE, "smul", A_UNDERLINE },
    { NCSTYLE_BLINK, "blink", A_BLINK },
    { NCSTYLE_DIM, "dim", A_DIM },
    { NCSTYLE_ITALIC, "sitm", A_ITALIC },
    { NCSTYLE_INVIS, "invis", A_INVIS },
    { NCSTYLE_PROTECT, "prot", A_PROTECT },
    { NCSTYLE_STRUCK, "smxx", 0 },
    { 0, NULL, 0 }
  };
  int nocolor_stylemask = tigetnum("ncv");
  for(typeof(*styles)* s = styles ; s->s ; ++s){
    if(nocolor_stylemask > 0){
      if(nocolor_stylemask & s->ncvbit){
        ti->supported_styles &= ~s->ncvbit;
        continue;
      }
    }
    char* style;
    if(terminfostr(&style, s->tinfo) == 0){
      ti->supported_styles |= s->s;
    }
  }
  // italics are never handled by sgr, but *can* be locked out by ncv. if
  // they are, we need clear the escapes we already loaded.
  if(!(ti->supported_styles & NCSTYLE_ITALIC)){
    ti->escindices[ESCAPE_SITM] = 0;
    ti->escindices[ESCAPE_RITM] = 0;
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
  if(ncinputlayer_init(&ti->input, stdin)){
    return -1;
  }
  if(apply_term_heuristics(ti, termname, fd)){
    goto err;
  }
  return 0;

err:
  free(ti->esctable);
  return -1;
}

/*
// FIXME need unit tests on this
// FIXME can read a character not intended for it
// we'll get a trailing Device Attributes response, because we write
// XTSMGRAPHICS followed by a DA query, in case the former isn't supported
// (we'd otherwise hang looking for input).
static int
read_xtsmgraphics_reply(int fd, int* val2){
  char in;
  // return is of the form CSI ? Pi ; Ps ; Pv S
  // Pi: 1 color registers, 2 sixel, 3 regis
  // Pa: 1 read, 2 reset, 3 set to Pv, 4 read maximum
  // Pv: n for color registers, width;height for geometry
  // Ps: 0 success, 1 bad Pi, 2 bad Pa, 3 failure
  enum {
    WANT_CSI,
    WANT_QMARK,
    WANT_PI,
    WANT_SEMI1,
    WANT_SEMI2,
    WANT_PV1,
    WANT_PV2,
    DONE,
  } state = WANT_CSI;
  int pv = 0;
  int pi = 0;
  while(read(fd, &in, 1) == 1){
//fprintf(stderr, "READ: %c 0x%02x\n", in, in);
    if(in == 'c'){ // should match the end of DA
      break;
    }
    switch(state){
      case WANT_CSI:
        if(in == NCKEY_ESC){
          state = WANT_QMARK;
        }
        break;
      case WANT_QMARK:
        if(in == '?'){
          state = WANT_PI;
        }
        break;
      case WANT_PI:
        if(!isdigit(in)){
          break;
        }
        pi *= 10;
        pi += in - '0';
        if(pi >= 1 && pi <= 3){
          state = WANT_SEMI1;
        }
        break;
      case WANT_SEMI1:
        if(in == ';'){
          state = WANT_SEMI2;
        }
        break;
      case WANT_SEMI2:
        if(in == ';'){
          state = WANT_PV1;
        }
        break;
      case WANT_PV1:
        if(in == 'S'){
          state = DONE;
        }else if(val2 && in == ';'){
          *val2 = 0;
          state = WANT_PV2;
        }else if(isdigit(in)){
          pv *= 10;
          pv += in - '0';
        }
        break;
      case WANT_PV2:
        if(in == 'S'){
          state = DONE;
        }else if(isdigit(in)){
          *val2 *= 10;
          *val2 += in - '0';
        }
        break;
      case DONE:
      default:
        break;
    }
  }
  if(state == DONE){
    if(pv >= 0 && (!val2 || *val2 >= 0)){
      return pv;
    }
  }
  return -1;
}

static int
query_xtsmgraphics(int fd, const char* seq, int* val, int* val2){
  if(blocking_write(fd, seq, strlen(seq))){
    return -1;
  }
  int r = read_xtsmgraphics_reply(fd, val2);
  if(r <= 0){
    return -1;
  }
  *val = r;
  return 0;
}

// query for Sixel support
static int
query_sixel(tinfo* ti, int fd){
  if(blocking_write(fd, ESC_DA, strlen(ESC_DA))){
    return -1;
  }
  char in;
  // perhaps the most lackadaisical response is that of st, which returns a
  // bare ESC[?6c (note no semicolon). this is equivalent to alacritty's
  // return, both suggesting a VT102. alacritty's miraculous technicolor VT102
  // can display sixel, but real VT102s don't even reply to XTSMGRAPHICS, so we
  // detect VT102 + TERM including alacritty, and special-case that.
  // FIXME need unit tests on this
  enum {
    WANT_CSI,
    WANT_QMARK,
    WANT_SEMI,
    WANT_C4, // want '4' to indicate sixel or 'c' to terminate
    WANT_VT102_C,
    DONE
  } state = WANT_CSI;
  bool in4 = false; // set true after seeing ";4", clear on semi
  // we're looking for a 4 following a semicolon
  while(state != DONE && read(fd, &in, 1) == 1){
    switch(state){
      case WANT_CSI:
        if(in == NCKEY_ESC){
          state = WANT_QMARK;
        }
        break;
      case WANT_QMARK:
        if(in == '?'){
          state = WANT_SEMI;
        }
        break;
      case WANT_SEMI:
        if(in == ';'){
          if(in4){
            setup_sixel_bitmaps(ti);
          }
          state = WANT_C4;
        }else if(in == 'c'){
          if(in4){
            setup_sixel_bitmaps(ti);
          }
          state = DONE;
        }else if(in == '6'){
          state = WANT_VT102_C;
        }
        in4 = false;
        break;
      case WANT_VT102_C:
        if(in == 'c'){
          if(ti->alacritty_sixel_hack){
            setup_sixel_bitmaps(ti);
          }
          state = DONE;
        }else if(in == ';'){
          state = WANT_C4;
        }
        break;
      case WANT_C4:
        if(in == 'c'){
          state = DONE;
        }else if(in == '4'){
          in4 = true;
          state = WANT_SEMI;
        }else{
          in4 = false;
        }
        break;
      case DONE:
      default:
        break;
    }
  }
  if(ti->bitmap_supported){
    if(ti->color_registers < 64){
      ti->bitmap_supported = false;
    }
  }
  return 0;
}
*/
