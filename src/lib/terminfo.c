#include <fcntl.h>
#include <ncurses.h> // needed for some definitions, see terminfo(3ncurses)
#include "internal.h"

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

int terminfostr(char** gseq, const char* name){
  char* seq;
  if(gseq == NULL){
    gseq = &seq;
  }
  *gseq = tigetstr(name);
  if(*gseq == NULL || *gseq == (char*)-1){
    *gseq = NULL;
    return -1;
  }
  // terminfo syntax allows a number N of milliseconds worth of pause to be
  // specified using $<N> syntax. this is then honored by tputs(). but we don't
  // use tputs(), instead preferring the much faster stdio+tiparm(). to avoid
  // dumping "$<N>" sequences all over stdio, we chop them out.
  char* pause;
  if( (pause = strchr(*gseq, '$')) ){
    *pause = '\0';
  }
  return 0;
}

// Qui si convien lasciare ogne sospetto; ogne viltà convien che qui sia morta.
static int
apply_term_heuristics(tinfo* ti, const char* termname){
  if(!termname){
    // setupterm interprets a missing/empty TERM variable as the special value “unknown”.
    termname = "unknown";
  }
  ti->braille = true;
  if(strstr(termname, "kitty")){ // kitty (https://sw.kovidgoyal.net/kitty/)
    // see https://sw.kovidgoyal.net/kitty/protocol-extensions.html
    // FIXME detect the actual default background color; this assumes it to
    // be RGB(0, 0, 0) (the default). we could also just set it, i guess.
    ti->bg_collides_default = 0x1000000;
    ti->sextants = true; // work since bugfix in 0.19.3
    ti->pixel_query_done = true;
    ti->sixel_supported = true;
    ti->pixel_cell_wipe = sprite_kitty_cell_wipe;
    ti->pixel_destroy = sprite_kitty_annihilate;
    ti->pixel_init = sprite_kitty_init;
    set_pixel_blitter(kitty_blit);
  /*}else if(strstr(termname, "alacritty")){
    ti->sextants = true; // alacritty https://github.com/alacritty/alacritty/issues/4409 */
  }else if(strstr(termname, "vte") || strstr(termname, "gnome") || strstr(termname, "xfce")){
    ti->sextants = true; // VTE has long enjoyed good sextant support
  }else if(strncmp(termname, "foot", 4) == 0){
    ti->sextants = true;
  }else if(strcmp(termname, "linux") == 0){
    ti->braille = false; // no braille, no sextants in linux console
  }
  // run a wcwidth() to guarantee libc Unicode 13 support
  if(wcwidth(L'🬸') < 0){
    ti->sextants = false;
  }
  return 0;
}

void free_terminfo_cache(tinfo* ti){
  pthread_mutex_destroy(&ti->pixel_query);
}

// termname is just the TERM environment variable. some details are not
// exposed via terminfo, and we must make heuristic decisions based on
// the detected terminal type, yuck :/.
int interrogate_terminfo(tinfo* ti, int fd, const char* termname, unsigned utf8){
  memset(ti, 0, sizeof(*ti));
  ti->utf8 = utf8;
  ti->RGBflag = query_rgb();
  int colors = tigetnum("colors");
  if(colors <= 0){
    ti->colors = 1;
    ti->CCCflag = false;
    ti->RGBflag = false;
    ti->initc = NULL;
  }else{
    ti->colors = colors;
    terminfostr(&ti->initc, "initc");
    if(ti->initc){
      ti->CCCflag = tigetflag("ccc") == 1;
    }else{
      ti->CCCflag = false;
    }
  }
  // check that the terminal provides cursor addressing (absolute movement)
  terminfostr(&ti->cup, "cup");
  if(ti->cup == NULL){
    fprintf(stderr, "Required terminfo capability 'cup' not defined\n");
    return -1;
  }
  // check that the terminal provides automatic margins
  ti->AMflag = tigetflag("am") == 1;
  if(!ti->AMflag){
    fprintf(stderr, "Required terminfo capability 'am' not defined\n");
    return -1;
  }
  ti->BCEflag = tigetflag("bce") == 1;
  terminfostr(&ti->civis, "civis"); // cursor invisible
  if(ti->civis == NULL){
    terminfostr(&ti->civis, "chts");// hard-to-see cursor
  }
  terminfostr(&ti->cnorm, "cnorm"); // cursor normal (undo civis/cvvis)
  terminfostr(&ti->standout, "smso"); // begin standout mode
  terminfostr(&ti->uline, "smul");    // begin underline mode
  terminfostr(&ti->reverse, "rev");   // begin reverse video mode
  terminfostr(&ti->blink, "blink");   // turn on blinking
  terminfostr(&ti->dim, "dim");       // turn on half-bright mode
  terminfostr(&ti->bold, "bold");     // turn on extra-bright mode
  terminfostr(&ti->italics, "sitm");  // begin italic mode
  terminfostr(&ti->italoff, "ritm");  // end italic mode
  terminfostr(&ti->sgr, "sgr");       // define video attributes
  terminfostr(&ti->sgr0, "sgr0");     // turn off all video attributes
  terminfostr(&ti->op, "op");         // restore defaults to default pair
  terminfostr(&ti->oc, "oc");         // restore defaults to all colors
  terminfostr(&ti->home, "home");     // home the cursor
  terminfostr(&ti->clearscr, "clear");// clear screen, home cursor
  terminfostr(&ti->cuu, "cuu"); // move N up
  terminfostr(&ti->cud, "cud"); // move N down
  terminfostr(&ti->hpa, "hpa"); // set horizontal position
  terminfostr(&ti->vpa, "vpa"); // set verical position
  terminfostr(&ti->cuf, "cuf"); // n non-destructive spaces
  terminfostr(&ti->cub, "cub"); // n non-destructive backspaces
  terminfostr(&ti->cuf1, "cuf1"); // non-destructive space
  terminfostr(&ti->sc, "sc"); // push ("save") cursor
  terminfostr(&ti->rc, "rc"); // pop ("restore") cursor
  // Some terminals cannot combine certain styles with colors. Don't advertise
  // support for the style in that case.
  int nocolor_stylemask = tigetnum("ncv");
  if(nocolor_stylemask > 0){
    if(nocolor_stylemask & A_STANDOUT){ // ncv is composed of terminfo bits, not ours
      ti->standout = NULL;
    }
    if(nocolor_stylemask & A_UNDERLINE){
      ti->uline = NULL;
    }
    if(nocolor_stylemask & A_REVERSE){
      ti->reverse = NULL;
    }
    if(nocolor_stylemask & A_BLINK){
      ti->blink = NULL;
    }
    if(nocolor_stylemask & A_DIM){
      ti->dim = NULL;
    }
    if(nocolor_stylemask & A_BOLD){
      ti->bold = NULL;
    }
    if(nocolor_stylemask & A_ITALIC){
      ti->italics = NULL;
    }
    // can't do anything about struck! :/
  }
  terminfostr(&ti->getm, "getm"); // get mouse events
  // Not all terminals support setting the fore/background independently
  terminfostr(&ti->setaf, "setaf"); // set forground color
  terminfostr(&ti->setab, "setab"); // set background color
  terminfostr(&ti->smkx, "smkx");   // enable keypad transmit
  terminfostr(&ti->rmkx, "rmkx");   // disable keypad transmit
  terminfostr(&ti->struck, "smxx"); // strikeout
  terminfostr(&ti->struckoff, "rmxx"); // cancel strikeout
  // if the keypad neen't be explicitly enabled, smkx is not present
  if(ti->smkx){
    if(fd >= 0){
      if(tty_emit(tiparm(ti->smkx), fd) < 0){
        fprintf(stderr, "Error entering keypad transmit mode\n");
        return -1;
      }
    }
  }
  // if op is defined as ansi 39 + ansi 49, make the split definitions available
  if(ti->op && strcmp(ti->op, "\x1b[39;49m") == 0){
    ti->fgop = "\x1b[39m";
    ti->bgop = "\x1b[49m";
  }
  pthread_mutex_init(&ti->pixel_query, NULL);
  ti->pixel_query_done = false;
  if(apply_term_heuristics(ti, termname)){
    return -1;
  }
  ti->sprixelnonce = 1;
  return 0;
}

static int
read_xtsmgraphics_reply(int fd, int* val2){
  char in;
  // return is of the form CSI ? Pi ; 0 ; Pv S
  enum {
    WANT_CSI,
    WANT_QMARK,
    WANT_SEMI1,
    WANT_SEMI2,
    WANT_PV1,
    WANT_PV2,
    DONE
  } state = WANT_CSI;
  int pv = 0;
  while(read(fd, &in, 1) == 1){
//fprintf(stderr, "READ: %c 0x%02x\n", in, in);
    switch(state){
      case WANT_CSI:
        if(in == NCKEY_ESC){
          state = WANT_QMARK;
        }
        break;
      case WANT_QMARK:
        if(in == '?'){
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
        if(!val2 && in == 'S'){
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
    if(state == DONE){
      if(pv >= 0 && (!val2 || *val2 >= 0)){
        return pv;
      }
      break;
    }
  }
  return -1;
}

static int
query_xtsmgraphics(int fd, const char* seq, int* val, int* val2){
  ssize_t w = writen(fd, seq, strlen(seq));
  if(w < 0 || (size_t)w != strlen(seq)){
    return -1;
  }
  int r = read_xtsmgraphics_reply(fd, val2);
  if(r <= 0){
    return -1;
  }
  *val = r;
  return 0;
}

// query for Sixel details including the number of color registers and, one day
// perhaps, maximum geometry. xterm binds its return by the current geometry,
// making it useless for a one-time query.
static int
query_sixel_details(tinfo* ti, int fd){
  if(query_xtsmgraphics(fd, "\x1b[?1;1;0S", &ti->color_registers, NULL)){
    return -1;
  }
  if(query_xtsmgraphics(fd, "\x1b[?2;4;0S", &ti->sixel_maxx, &ti->sixel_maxy)){
    return -1;
  }
//fprintf(stderr, "Sixel ColorRegs: %d Max_x: %d Max_y: %d\n", ti->color_registers, ti->sixel_maxx, ti->sixel_maxy);
  return 0;
}

// query for Sixel support
static int
query_sixel(tinfo* ti, int fd){
  if(writen(fd, "\x1b[c", 3) != 3){
    return -1;
  }
  char in;
  // perhaps the most lackadaisical response is that of st, which returns a
  // bare ESC[?6c (note no semicolon).
  enum {
    WANT_CSI,
    WANT_QMARK,
    WANT_SEMI,
    WANT_C,
    DONE
  } state = WANT_CSI;
  while(read(fd, &in, 1) == 1){
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
          state = WANT_C;
        }else if(in == 'c'){
          state = DONE;
        }
        break;
      case WANT_C:
        if(in == 'c'){
          state = DONE;
        }else if(in == '4'){
          if(!ti->sixel_supported){
            ti->sixel_supported = true;
            ti->color_registers = 256;  // assumed default [shrug]
            ti->pixel_destroy = sprite_sixel_annihilate;
            ti->pixel_init = sprite_sixel_init;
            ti->pixel_cell_wipe = sprite_sixel_cell_wipe;
            ti->sixel_maxx = ti->sixel_maxy = 0;
          }
        }
        break;
      case DONE:
      default:
        break;
    }
    if(state == DONE){
      break;
    }
  }
  return 0; // FIXME return error?
}

// fd must be a real terminal. uses the query lock of |ti| to only act once.
int query_term(tinfo* ti, int fd){
  if(fd < 0){
    return -1;
  }
  int flags = fcntl(fd, F_GETFL, 0);
  if(flags < 0){
    return -1;
  }
  int ret = 0;
  pthread_mutex_lock(&ti->pixel_query);
  if(!ti->pixel_query_done){
    if(flags & O_NONBLOCK){
      fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
    }
    ret = query_sixel(ti, fd);
    ti->pixel_query_done = true;
    if(ti->sixel_supported){
      query_sixel_details(ti, fd);
      ti->pixel_init(fd);
    }
    if(flags & O_NONBLOCK){
      fcntl(fd, F_SETFL, flags);
    }
  }
  pthread_mutex_unlock(&ti->pixel_query);
  return ret;
}
