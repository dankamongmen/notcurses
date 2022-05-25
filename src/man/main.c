#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <notcurses/notcurses.h>
#include "structure.h"
#include "builddef.h"
#include "parse.h"

static void
usage(const char* argv0, FILE* o){
  fprintf(o, "usage: %s [ -hVq ] files\n", argv0);
  fprintf(o, " -h: print help and return success\n");
  fprintf(o, " -V: print version and return success\n");
  fprintf(o, " -q: don't wait for any input\n");
}

static int
parse_args(int argc, char** argv, unsigned* noui){
  const char* argv0 = *argv;
  int longindex;
  int c;
  struct option longopts[] = {
    { .name = "help", .has_arg = 0, .flag = NULL, .val = 'h', },
    { .name = NULL, .has_arg = 0, .flag = NULL, .val = 0, }
  };
  while((c = getopt_long(argc, argv, "hVq", longopts, &longindex)) != -1){
    switch(c){
      case 'h': usage(argv0, stdout);
                exit(EXIT_SUCCESS);
                break;
      case 'V': fprintf(stderr, "%s version %s\n", argv[0], notcurses_version());
                exit(EXIT_SUCCESS);
                break;
      case 'q': *noui = true;
                break;
      default: usage(argv0, stderr);
               return -1;
               break;
    }
  }
  if(argv[optind] == NULL){
    usage(argv0, stderr);
    return -1;
  }
  return optind;
}

#ifdef USE_DEFLATE // libdeflate implementation
#include <libdeflate.h>
// assume that |buf| is |*len| bytes of deflated data, and try to inflate
// it. if successful, the inflated map will be returned. either way, the
// input map will be unmapped (we take ownership). |*len| will be updated
// if an inflated map is successfully returned.
static unsigned char*
map_gzipped_data(unsigned char* buf, size_t* len, unsigned char* ubuf, uint32_t ulen){
  struct libdeflate_decompressor* inflate = libdeflate_alloc_decompressor();
  if(inflate == NULL){
    fprintf(stderr, "couldn't get libdeflate inflator\n");
    munmap(buf, *len);
    return NULL;
  }
  size_t outbytes;
  enum libdeflate_result r;
  r = libdeflate_gzip_decompress(inflate, buf, *len, ubuf, ulen, &outbytes);
  munmap(buf, *len);
  libdeflate_free_decompressor(inflate);
  if(r != LIBDEFLATE_SUCCESS){
    fprintf(stderr, "error inflating %"PRIuPTR" (%d)\n", *len, r);
    return NULL;
  }
  *len = ulen;
  return ubuf;
}
#else // libz implementation
#include <zlib.h>
static unsigned char*
map_gzipped_data(unsigned char* buf, size_t* len, unsigned char* ubuf, uint32_t ulen){
  z_stream z = {0};
  int r = inflateInit2(&z, 15 | 16);
  if(r != Z_OK){
    fprintf(stderr, "error getting zlib inflator (%d)\n", r);
    munmap(buf, *len);
    return NULL;
  }
  z.next_in = buf;
  z.avail_in = *len;
  z.next_out = ubuf;
  z.avail_out = ulen;
  r = inflate(&z, Z_FINISH);
  munmap(buf, *len);
  if(r != Z_STREAM_END){
    fprintf(stderr, "error inflating (%d) (%s?)\n", r, z.msg);
    inflateEnd(&z);
    return NULL;
  }
  inflateEnd(&z);
  munmap(buf, *len);
  *len = ulen;
  return ubuf;
}
#endif

static unsigned char*
map_troff_data(int fd, size_t* len){
  struct stat sbuf;
  if(fstat(fd, &sbuf)){
    return NULL;
  }
  // gzip has a 10-byte mandatory header and an 8-byte mandatory footer
  if(sbuf.st_size < 18){
    return NULL;
  }
  *len = sbuf.st_size;
  unsigned char* buf = mmap(NULL, *len, PROT_READ,
#ifdef MAP_POPULATE
                            MAP_POPULATE |
#endif
                            MAP_PRIVATE, fd, 0);
  if(buf == MAP_FAILED){
    fprintf(stderr, "error mapping %"PRIuPTR" (%s?)\n", *len, strerror(errno));
    return NULL;
  }
  if(buf[0] == 0x1f && buf[1] == 0x8b && buf[2] == 0x08){
    // the last four bytes have the uncompressed length
    uint32_t ulen;
    memcpy(&ulen, buf + *len - 4, 4);
    long sc = sysconf(_SC_PAGESIZE);
    if(sc <= 0){
      fprintf(stderr, "couldn't get page size\n");
      return NULL;
    }
    size_t pgsize = sc;
    void* ubuf = mmap(NULL, (ulen + pgsize - 1) / pgsize * pgsize,
                      PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if(ubuf == MAP_FAILED){
      fprintf(stderr, "error mapping %"PRIu32" (%s?)\n", ulen, strerror(errno));
      munmap(buf, *len);
      return NULL;
    }
    if(map_gzipped_data(buf, len, ubuf, ulen) == NULL){
      munmap(ubuf, ulen);
      return NULL;
    }
    return ubuf;
  }
  return buf;
}

// find the man page, and inflate it if deflated
static unsigned char*
get_troff_data(const char *arg, size_t* len){
  // FIXME we'll want to use the mandb. for now, require a full path.
  int fd = open(arg, O_RDONLY | O_CLOEXEC);
  if(fd < 0){
    fprintf(stderr, "error opening %s (%s?)\n", arg, strerror(errno));
    return NULL;
  }
  unsigned char* buf = map_troff_data(fd, len);
  close(fd);
  return buf;
}

// invoke ncplane_puttext() on the text starting at s and ending
// (non-inclusive) at e.
static int
puttext(struct ncplane* p, const char* s, const char* e){
  if(e <= s){
//fprintf(stderr, "no text to print\n");
    return 0; // no text to print
  }
  char* dup = strndup(s, e - s);
  if(dup == NULL){
    return -1;
  }
  size_t b = 0;
  int r = ncplane_puttext(p, -1, NCALIGN_LEFT, dup, &b);
  free(dup);
  return r;
}

// try to identify a unicode macro in |macro|, and if so, update |end|. returns
// a static buffer, so this is not threadsafe/reentrant.
static const char*
unicode_macro(const char* macro, const char** end){
  unsigned long u;
  u = strtoul(macro + 1, (char**)end, 16);
  if(**end != ']'){
    return NULL;
  }
  ++*end;
  static char fill[5];
  wint_t wu = u;
  snprintf(fill, sizeof(fill), "%lc", wu);
  return fill;
}

// paragraphs can have formatting information inline within their text. we
// proceed until we find such an inline marker, print the text we've skipped,
// set up the style, and continue.
static int
putpara(struct ncplane* p, const char* text){
  // cur indicates where the current text to be displayed starts.
  const char* cur = text;
  uint16_t style = 0;
  const char* posttext = NULL;
  while(*cur){
    // find the next style marker
    bool inescape = false;
    const char* textend = NULL; // one past where the text to print ends
    // curend is where the text + style markings end
    const char* curend;
    for(curend = cur ; *curend ; ++curend){
      if(*curend == '\\'){
        if(inescape){ // escaped backslash
          inescape = false;
        }else{
          inescape = true;
        }
      }else if(inescape){
        if(*curend == 'f'){ // set font
          textend = curend - 1; // account for backslash
          bool bracketed = false;
          if(*++curend == '['){
            bracketed = true;
            ++curend;
          }
          while(isalpha(*curend)){
            switch(toupper(*curend)){
              case 'R': style = 0; break; // roman, default
              case 'I': style |= NCSTYLE_ITALIC; break;
              case 'B': style |= NCSTYLE_BOLD; break;
              case 'C': break; // unsure! seems to be used with .nf/.fi
              default:
                fprintf(stderr, "unknown font macro %s\n", curend);
                return -1;
            }
            ++curend;
          }
          if(bracketed){
            if(*curend != ']'){
              fprintf(stderr, "missing ']': %s\n", curend);
              return -1;
            }
            ++curend;
          }
          break;
        }else if(*curend == '['){ // escaped sequence
          textend = curend - 1; // account for backslash
          static const struct {
            const char* macro;
            const char* tr;
          } macros[] = {
            { "aq]", "'", },
            { "dq]", "\"", },
            { "lq]", u8"\u201c", }, // left double quote
            { "rq]", u8"\u201d", }, // right double quote
            { "em]", u8"\u2014", }, // em dash
            { "en]", u8"\u2013", }, // en dash
            { "rg]", "®", },
            { "rs]", "\\", },
            { "ti]", "~", },
            { NULL, NULL, }
          };
          ++curend;
          const char* macend = NULL;
          for(typeof(&*macros) m = macros ; m->tr ; ++m){
            if(strncmp(curend, m->macro, strlen(m->macro)) == 0){
              posttext = m->tr;
              macend = curend + strlen(m->macro);
              break;
            }
          }
          if(macend == NULL){
            posttext = unicode_macro(curend, &macend);
            if(posttext == NULL){
              fprintf(stderr, "unknown macro %s\n", curend);
              return -1;
            }
          }
          curend = macend;
          break;
        }else{
          inescape = false;
          cur = curend++;
          break;
        }
        inescape = false;
      }
    }
    if(textend == NULL){
      textend = curend;
    }
    if(puttext(p, cur, textend) < 0){
      return -1;
    }
    if(posttext){
      if(puttext(p, posttext, posttext + strlen(posttext)) < 0){
        return -1;
      }
      posttext = NULL;
    }
    cur = curend;
    ncplane_set_styles(p, style);
  }
  ncplane_cursor_move_yx(p, -1, 0);
  return 0;
}

static int
draw_domnode(struct ncplane* p, const pagedom* dom, const pagenode* n,
             unsigned* wrotetext, unsigned* insubsec){
  ncplane_set_fg_rgb(p, ncchannel_rgb(n->ttype->channel));
  size_t b = 0;
  unsigned y;
  ncplane_cursor_yx(p, &y, NULL);
  switch(n->ttype->ltype){
    case LINE_TH:
      if(docstructure_add(dom->ds, dom->title, y)){
        return -1;
      }
      /*
      ncplane_set_styles(p, NCSTYLE_UNDERLINE);
      ncplane_printf_aligned(p, 0, NCALIGN_LEFT, "%s(%s)", dom->title, dom->section);
      ncplane_printf_aligned(p, 0, NCALIGN_RIGHT, "%s(%s)", dom->title, dom->section);
      ncplane_set_styles(p, NCSTYLE_NONE);
      */break;
    case LINE_SH: // section heading
      if(docstructure_add(dom->ds, n->text, y)){
        return -1;
      }
      if(strcmp(n->text, "NAME")){
        ncplane_puttext(p, -1, NCALIGN_LEFT, "\n\n", &b);
        ncplane_set_styles(p, NCSTYLE_BOLD | NCSTYLE_UNDERLINE);
        ncplane_putstr_aligned(p, -1, NCALIGN_CENTER, n->text);
        ncplane_set_styles(p, NCSTYLE_NONE);
        ncplane_cursor_move_yx(p, -1, 0);
        *wrotetext = true;
      }
      break;
    case LINE_SS: // subsection heading
      if(docstructure_add(dom->ds, n->text, y)){
        return -1;
      }
      ncplane_puttext(p, -1, NCALIGN_LEFT, "\n\n", &b);
      ncplane_set_styles(p, NCSTYLE_ITALIC | NCSTYLE_UNDERLINE);
      ncplane_putstr_aligned(p, -1, NCALIGN_CENTER, n->text);
      ncplane_set_styles(p, NCSTYLE_NONE);
      ncplane_cursor_move_yx(p, -1, 0);
      *wrotetext = true;
      *insubsec = true;
      break;
    case LINE_PP: // paragraph
    case LINE_TP: // tagged paragraph
    case LINE_IP: // indented paragraph
      if(*wrotetext){
        if(n->text){
          ncplane_set_fg_rgb(p, 0xe0f0ff);
          if(*insubsec){
            ncplane_puttext(p, -1, NCALIGN_LEFT, "\n", &b);
            *insubsec = false;
          }else{
            ncplane_puttext(p, -1, NCALIGN_LEFT, "\n\n", &b);
          }
          ncplane_set_fg_alpha(p, NCALPHA_HIGHCONTRAST);
          putpara(p, n->text);
        }
      }else{
        if(n->text){
          ncplane_set_styles(p, NCSTYLE_BOLD | NCSTYLE_ITALIC);
          ncplane_set_fg_rgb(p, 0xff6a00);
          ncplane_putstr_aligned(p, -1, NCALIGN_CENTER, n->text);
          ncplane_set_fg_default(p);
          ncplane_set_styles(p, NCSTYLE_NONE);
        }
      }
      *wrotetext = true;
      break;
    default:
      fprintf(stderr, "unhandled ltype %d\n", n->ttype->ltype);
      return 0; // FIXME
  }
  for(unsigned z = 0 ; z < n->subcount ; ++z){
    if(draw_domnode(p, dom, &n->subs[z], wrotetext, insubsec)){
      return -1;
    }
  }
  return 0;
}

// for now, we draw the entire thing, resizing as necessary, and we'll
// scroll the entire plane. higher memory cost, longer initial latency,
// very fast moves.
static int
draw_content(struct ncplane* stdn, struct ncplane* p){
  pagedom* dom = ncplane_userptr(p);
  unsigned wrotetext = 0; // unused by us
  unsigned insubsec = 0;
  docstructure_free(dom->ds);
  dom->ds = docstructure_create(stdn);
  if(dom->ds == NULL){
    return -1;
  }
  return draw_domnode(p, dom, dom->root, &wrotetext, &insubsec);
}

static int
resize_pman(struct ncplane* pman){
  unsigned dimy, dimx;
  ncplane_dim_yx(ncplane_parent_const(pman), &dimy, &dimx);
  ncplane_resize_simple(pman, dimy - 1, dimx);
  ncplane_erase(pman);
  struct ncplane* stdn = notcurses_stdplane(ncplane_notcurses(pman));
  int r = draw_content(stdn, pman);
  ncplane_move_yx(pman, 0, 0);
  return r;
}

// we create a plane sized appropriately for the troff data. all we do
// after that is move the plane up and down.
static struct ncplane*
render_troff(struct notcurses* nc, const unsigned char* map, size_t mlen,
             pagedom* dom){
  unsigned dimy, dimx;
  struct ncplane* stdn = notcurses_stddim_yx(nc, &dimy, &dimx);
  // this is just an estimate
  if(troff_parse(map, mlen, dom)){
    return NULL;
  }
  // this is just an estimate
  struct ncplane_options popts = {
    .rows = dimy - 2,
    .cols = dimx,
    .y = 1,
    .userptr = dom,
    .resizecb = resize_pman,
    .flags = NCPLANE_OPTION_AUTOGROW | NCPLANE_OPTION_VSCROLL,
  };
  struct ncplane* pman = ncplane_create(stdn, &popts);
  if(pman == NULL){
    return NULL;
  }
  ncplane_set_base(pman, " ", 0, 0);
  if(draw_content(stdn, pman)){
    ncplane_destroy(pman);
    return NULL;
  }
  return pman;
}

static const char USAGE_TEXT[] = "⎥h←s→l⎢⎥b⇞k↑↓j⇟f⎢ (q)uit";
static const char USAGE_TEXT_ASCII[] = "(hsl) (bkjf) (q)uit";

static int
draw_bar(struct ncplane* bar, pagedom* dom){
  ncplane_cursor_move_yx(bar, 0, 0);
  ncplane_set_styles(bar, NCSTYLE_BOLD);
  ncplane_putstr(bar, dom_get_title(dom));
  ncplane_set_styles(bar, NCSTYLE_NONE);
  ncplane_putchar(bar, '(');
  ncplane_set_styles(bar, NCSTYLE_BOLD);
  ncplane_putstr(bar, dom->section);
  ncplane_set_styles(bar, NCSTYLE_NONE);
  ncplane_printf(bar, ") %s", dom->version);
  ncplane_set_styles(bar, NCSTYLE_ITALIC);
  if(notcurses_canutf8(ncplane_notcurses(bar))){
    ncplane_putstr_aligned(bar, 0, NCALIGN_RIGHT, USAGE_TEXT);
  }else{
    ncplane_putstr_aligned(bar, 0, NCALIGN_RIGHT, USAGE_TEXT_ASCII);
  }
  return 0;
}

static int
resize_bar(struct ncplane* bar){
  unsigned dimy, dimx;
  ncplane_dim_yx(ncplane_parent_const(bar), &dimy, &dimx);
  ncplane_resize_simple(bar, 1, dimx);
  ncplane_erase(bar);
  int r = draw_bar(bar, ncplane_userptr(bar));
  ncplane_move_yx(bar, dimy - 1, 0);
  return r;
}

static void
domnode_destroy(pagenode* node){
  if(node){
    free(node->text);
    for(unsigned z = 0 ; z < node->subcount ; ++z){
      domnode_destroy(&node->subs[z]);
    }
    free(node->subs);
  }
}

static void
pagedom_destroy(pagedom* dom){
  destroy_trofftrie(dom->trie);
  domnode_destroy(dom->root);
  free(dom->root);
  free(dom->title);
  free(dom->version);
  free(dom->section);
  docstructure_free(dom->ds);
}

static struct ncplane*
create_bar(struct notcurses* nc, pagedom* dom){
  unsigned dimy, dimx;
  struct ncplane* stdn = notcurses_stddim_yx(nc, &dimy, &dimx);
  struct ncplane_options nopts = {
    .y = dimy - 1,
    .x = 0,
    .rows = 1,
    .cols = dimx,
    .resizecb = resize_bar,
    .userptr = dom,
  };
  struct ncplane* bar = ncplane_create(stdn, &nopts);
  if(bar == NULL){
    return NULL;
  }
  uint64_t barchan = NCCHANNELS_INITIALIZER(0, 0, 0, 0x26, 0x62, 0x41);
  ncplane_set_fg_rgb(bar, 0xffffff);
  if(ncplane_set_base(bar, " ", 0, barchan) != 1){
    ncplane_destroy(bar);
    return NULL;
  }
  if(draw_bar(bar, dom)){
    ncplane_destroy(bar);
    return NULL;
  }
  if(notcurses_render(nc)){
    ncplane_destroy(bar);
    return NULL;
  }
  return bar;
}

static int
manloop(struct notcurses* nc, const char* arg, unsigned noui){
  struct ncplane* stdn = notcurses_stdplane(nc);
  int ret = -1;
  struct ncplane* page = NULL;
  struct ncplane* bar = NULL;
  pagedom dom = {0};
  size_t len;
  unsigned char* buf = get_troff_data(arg, &len);
  if(buf == NULL){
    goto done;
  }
  dom.trie = trofftrie();
  if(dom.trie == NULL){
    goto done;
  }
  page = render_troff(nc, buf, len, &dom);
  if(page == NULL){
    goto done;
  }
  bar = create_bar(nc, &dom);
  if(bar == NULL){
    goto done;
  }
  if(notcurses_render(nc)){
    goto done;
  }
  if(noui){
    ret = 0;
    goto done;
  }
  uint32_t key;
  do{
    bool movedown = false;
    int newy = ncplane_y(page);
    ncinput ni;
    key = notcurses_get(nc, NULL, &ni);
    if(ni.evtype == NCTYPE_RELEASE){
      continue;
    }
    switch(key){
      case 'L':
        if(ni.ctrl && !ni.alt){
          notcurses_refresh(nc, NULL, NULL);
        }
        break;
      case 's':
        docstructure_toggle(page, bar, dom.ds);
        break;
      case 'h': case NCKEY_LEFT:
        newy = docstructure_prev(dom.ds);
        break;
      case 'l': case NCKEY_RIGHT:
        newy = docstructure_next(dom.ds);
        movedown = true;
        break;
      case 'k': case NCKEY_UP:
        newy = ncplane_y(page) + 1;
        break;
      // we can move down iff our last line is beyond the visible area
      case 'j': case NCKEY_DOWN:
        newy = ncplane_y(page) - 1;
        movedown = true;
        break;
      case 'b': case NCKEY_PGUP:{
        newy = ncplane_y(page) + (int)ncplane_dim_y(stdn);
        break;
      }case 'f': case NCKEY_PGDOWN:{
        newy = ncplane_y(page) - (int)ncplane_dim_y(stdn) + 1;
        movedown = true;
        break;
      case 'g': case NCKEY_HOME:
        newy = 1;
        break;
      }case 'q':
        ret = 0;
        goto done;
    }
    if(newy > 1){
      newy = 1;
    }
    if(newy + (int)ncplane_dim_y(page) < (int)ncplane_dim_y(stdn)){
      newy += (int)ncplane_dim_y(stdn) - (newy + (int)ncplane_dim_y(page)) - 1;
    }
    if(newy != ncplane_y(page)){
      ncplane_move_yx(page, newy, 0);
      docstructure_move(dom.ds, newy, movedown);
      if(notcurses_render(nc)){
        goto done;
      }
    }
  }while(key != (uint32_t)-1);

done:
  if(page){
    ncplane_destroy(page);
  }
  ncplane_destroy(bar);
  if(buf){
    munmap(buf, len);
  }
  pagedom_destroy(&dom);
  return ret;
}

static int
tfman(struct notcurses* nc, const char* arg, unsigned noui){
  int r = manloop(nc, arg, noui);
  return r;
}

int main(int argc, char** argv){
  unsigned noui = false;
  int nonopt = parse_args(argc, argv, &noui);
  if(nonopt <= 0){
    return EXIT_FAILURE;
  }
  struct notcurses_options nopts = {0};
  if(noui){
    nopts.flags |= NCOPTION_NO_ALTERNATE_SCREEN
                   | NCOPTION_NO_CLEAR_BITMAPS
                   | NCOPTION_PRESERVE_CURSOR
                   | NCOPTION_DRAIN_INPUT;
  }
  struct notcurses* nc = notcurses_core_init(&nopts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  bool success;
  for(int i = 0 ; i < argc - nonopt ; ++i){
    success = false;
    if(tfman(nc, argv[nonopt + i], noui)){
      break;
    }
    success = true;
  }
  return notcurses_stop(nc) || !success ? EXIT_FAILURE : EXIT_SUCCESS;
}
