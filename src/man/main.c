#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <wctype.h>
#include <getopt.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <notcurses/notcurses.h>
#include "builddef.h"

static void
usage(const char* argv0, FILE* o){
  fprintf(o, "usage: %s [ -hV ] files\n", argv0);
  fprintf(o, " -h: print help and return success\n");
  fprintf(o, " -v: print version and return success\n");
}

static int
parse_args(int argc, char** argv){
  const char* argv0 = *argv;
  int longindex;
  int c;
  struct option longopts[] = {
    { .name = "help", .has_arg = 0, .flag = NULL, .val = 'h', },
    { .name = NULL, .has_arg = 0, .flag = NULL, .val = 0, }
  };
  while((c = getopt_long(argc, argv, "hV", longopts, &longindex)) != -1){
    switch(c){
      case 'h': usage(argv0, stdout);
                exit(EXIT_SUCCESS);
                break;
      case 'V': fprintf(stderr, "ncman version %s\n", notcurses_version());
                exit(EXIT_SUCCESS);
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
  z_stream z = {};
  int r = inflateInit2(&z, 16);
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

typedef enum {
  LINE_UNKNOWN,
  LINE_COMMENT,
  LINE_B, LINE_BI, LINE_BR, LINE_I, LINE_IB, LINE_IR,
  LINE_RB, LINE_RI, LINE_SB, LINE_SM,
  LINE_EE, LINE_EX, LINE_RE, LINE_RS,
  LINE_SH, LINE_SS, LINE_TH,
  LINE_IP, LINE_LP, LINE_P, LINE_PP,
  LINE_TP, LINE_TQ,
  LINE_ME, LINE_MT, LINE_UE, LINE_UR,
  LINE_OP, LINE_SY, LINE_YS,
} ltypes;

typedef enum {
  TROFF_UNKNOWN,
  TROFF_COMMENT,
  TROFF_FONT,
  TROFF_STRUCTURE,
  TROFF_PARAGRAPH,
  TROFF_HYPERLINK,
  TROFF_SYNOPSIS
} ttypes;

typedef struct {
  ltypes ltype;
  const char* symbol;
  ttypes ttype;
  uint32_t channel;
} trofftype;

// all troff types start with a period, followed by one or two ASCII
// characters.
static const trofftype trofftypes[] = {
  { .ltype = LINE_UNKNOWN, .symbol = "", .ttype = TROFF_UNKNOWN, .channel = 0, },
  { .ltype = LINE_COMMENT, .symbol = "\\\"", .ttype = TROFF_COMMENT, .channel = 0, },
#define TROFF_FONT(x) { .ltype = LINE_##x, .symbol = #x, .ttype = TROFF_FONT, .channel = 0, },
  TROFF_FONT(B) TROFF_FONT(BI) TROFF_FONT(BR)
  TROFF_FONT(I) TROFF_FONT(IB) TROFF_FONT(IR)
#undef TROFF_FONT
#define TROFF_STRUCTURE(x, c) { .ltype = LINE_##x, .symbol = #x, .ttype = TROFF_STRUCTURE, .channel = (c), },
  TROFF_STRUCTURE(EE, 0)
  TROFF_STRUCTURE(EX, 0)
  TROFF_STRUCTURE(RE, 0)
  TROFF_STRUCTURE(RS, 0)
  TROFF_STRUCTURE(SH, NCCHANNEL_INITIALIZER(0x9b, 0x9b, 0xfc))
  TROFF_STRUCTURE(SS, NCCHANNEL_INITIALIZER(0x6c, 0x6b, 0xfb))
  TROFF_STRUCTURE(TH, NCCHANNEL_INITIALIZER(0xcb, 0xcb, 0xfd))
#undef TROFF_STRUCTURE
#define TROFF_PARA(x) { .ltype = LINE_##x, .symbol = #x, .ttype = TROFF_PARAGRAPH, .channel = 0, },
  TROFF_PARA(IP) TROFF_PARA(LP) TROFF_PARA(P)
  TROFF_PARA(PP) TROFF_PARA(TP) TROFF_PARA(TQ)
#undef TROFF_PARA
#define TROFF_HLINK(x) { .ltype = LINE_##x, .symbol = #x, .ttype = TROFF_HYPERLINK, .channel = 0, },
  TROFF_HLINK(ME) TROFF_HLINK(MT) TROFF_HLINK(UE) TROFF_HLINK(UR)
#undef TROFF_HLINK
#define TROFF_SYNOPSIS(x) { .ltype = LINE_##x, .symbol = #x, .ttype = TROFF_SYNOPSIS, .channel = 0, },
  TROFF_SYNOPSIS(OP) TROFF_SYNOPSIS(SY) TROFF_SYNOPSIS(YS)
#undef TROFF_SYNOPSIS
  { .ltype = LINE_UNKNOWN, .symbol = "hy", .ttype = TROFF_UNKNOWN, .channel = 0, },
  { .ltype = LINE_UNKNOWN, .symbol = "br", .ttype = TROFF_UNKNOWN, .channel = 0, },
  { .ltype = LINE_COMMENT, .symbol = "IX", .ttype = TROFF_COMMENT, .channel = 0, },
};

// the troff trie is only defined on the 128 ascii values.
struct troffnode {
  struct troffnode* next[0x80];
  const trofftype *ttype;
};

static void
destroy_trofftrie(struct troffnode* root){
  if(root){
    for(unsigned i = 0 ; i < sizeof(root->next) / sizeof(*root->next) ; ++i){
      destroy_trofftrie(root->next[i]);
    }
    free(root);
  }
}

// build a trie rooted at an implicit leading period.
static struct troffnode*
trofftrie(void){
  struct troffnode* root = malloc(sizeof(*root));
  if(root == NULL){
    return NULL;
  }
  memset(root, 0, sizeof(*root));
  for(size_t toff = 0 ; toff < sizeof(trofftypes) / sizeof(*trofftypes) ; ++toff){
    const trofftype* t = &trofftypes[toff];
    if(strlen(t->symbol) == 0){
      continue;
    }
    struct troffnode* n = root;
    for(const char* s = t->symbol ; *s ; ++s){
      if(*s < 0){ // illegal symbol
        fprintf(stderr, "illegal symbol: %s\n", t->symbol);
        goto err;
      }
      unsigned char us = *s;
      if(us > sizeof(root->next) / sizeof(*root->next)){ // illegal symbol
        fprintf(stderr, "illegal symbol: %s\n", t->symbol);
        goto err;
      }
      if(n->next[us] == NULL){
        if((n->next[us] = malloc(sizeof(*root))) == NULL){
          goto err;
        }
        memset(n->next[us], 0, sizeof(*root));
      }
      n = n->next[us];
    }
    if(n->ttype){ // duplicate command
      fprintf(stderr, "duplicate command: %s %s\n", t->symbol, n->ttype->symbol);
      goto err;
    }
    n->ttype = t;
  }
  return root;

err:
  destroy_trofftrie(root);
  return NULL;
}

// lex the troffnode out from |ws|, where the troffnode is all text prior to
// whitespace or a NUL. the byte following the troffnode is written back to
// |ws|. if it is a valid troff command sequence, the node is returned;
// NULL is otherwise returned. |len| ought be non-negative.
static const trofftype*
get_type(const struct troffnode* trie, const unsigned char** ws, size_t len){
  if(**ws != '.'){
    return NULL;
  }
  ++*ws;
  --len;
  while(len && !isspace(**ws) && **ws){
    if(**ws > sizeof(trie->next) / sizeof(*trie->next)){ // illegal command
      return NULL;
    }
    if((trie = trie->next[**ws]) == NULL){
      return NULL;
    }
    ++*ws;
    --len;
  }
  return trie->ttype;
}

typedef struct pagenode {
  char* text;
  const trofftype* ttype;
  struct pagenode* subs;
  unsigned subcount;
} pagenode;

typedef struct pagedom {
  struct pagenode* root;
  struct troffnode* trie;
  char* title;
  char* section;
  char* version;
  char* footer;
  char* header;
} pagedom;

static const char*
dom_get_title(const pagedom* dom){
  return dom->title;
}

// get the next token. first, chew whitespace. then match a string of
// iswgraph(), or a quoted string of iswprint(). return the number of
// characters consumed, or -1 on error (no token, unterminated quote).
// heap-copies the utf8 to *token on success.
static int
lex_next_token(const char* s, char** token){
  mbstate_t ps = {};
  wchar_t w;
  size_t b, cur;
  cur = 0;
  bool inquote = false;
  const char* tokstart = NULL;
  while((b = mbrtowc(&w, s + cur, MB_CUR_MAX, &ps)) != (size_t)-1 && b != (size_t)-2){
    if(tokstart){
      if(b == 0 || (inquote && w == L'"') || (!inquote && iswspace(w))){
        if(!tokstart || !*tokstart || *tokstart == '"'){
          return -1;
        }
        *token = strndup(tokstart, cur - (tokstart - s));
        return cur + b;
      }
    }else{
      if(iswspace(w)){
        cur += b;
        continue;
      }
      if(w == '"'){
        inquote = true;
        tokstart = s + cur + b;
      }else{
        tokstart = s + cur;
      }
    }
    cur += b;
  }
  return -1;
}

// take the newly-added title section, and extract the title, section, and
// version (technically footer-middle, footer-inside, and header-middle).
// they ought be quoted, but might not be.
static int
lex_title(pagedom* dom){
  const char* tok = dom->root->text;
  int b = lex_next_token(tok, &dom->title);
  if(b < 0){
    fprintf(stderr, "couldn't extract title [%s]\n", dom->root->text);
    return -1;
  }
  tok += b;
  b = lex_next_token(tok, &dom->section);
  if(b < 0){
    fprintf(stderr, "couldn't extract section [%s]\n", dom->root->text);
    return -1;
  }
  tok += b;
  b = lex_next_token(tok, &dom->version);
  if(b < 0){
    //fprintf(stderr, "couldn't extract version [%s]\n", dom->root->text);
    return 0;
  }
  tok += b;
  b = lex_next_token(tok, &dom->footer);
  if(b < 0){
    //fprintf(stderr, "couldn't extract footer [%s]\n", dom->root->text);
    return 0;
  }
  tok += b;
  b = lex_next_token(tok, &dom->header);
  if(b < 0){
    //fprintf(stderr, "couldn't extract header [%s]\n", dom->root->text);
    return 0;
  }
  return 0;
}

static pagenode*
add_node(pagenode* pnode, char* text){
  unsigned ncount = pnode->subcount + 1;
  pagenode* tmpsubs = realloc(pnode->subs, sizeof(*pnode->subs) * ncount);
  if(tmpsubs == NULL){
    return NULL;
  }
  pnode->subs = tmpsubs;
  pagenode* r = pnode->subs + pnode->subcount;
  pnode->subcount = ncount;
  memset(r, 0, sizeof(*r));
  r->text = text;
//fprintf(stderr, "ADDED SECTION %s %u\n", text, pnode->subcount);
  return r;
}

static char*
extract_text(const unsigned char* ws, const unsigned char* feol){
  if(ws == feol || ws == feol + 1){
    fprintf(stderr, "bogus empty title\n");
    return NULL;
  }
  return strndup((const char*)ws + 1, feol - ws);
}

static char*
augment_text(pagenode* pnode, const unsigned char* ws, const unsigned char* feol){
  const size_t slen = pnode->text ? strlen(pnode->text) + 1 : 0;
  char* tmp = realloc(pnode->text, slen + (feol - ws) + 2);
  if(tmp == NULL){
    return NULL;
  }
  pnode->text = tmp;
  if(slen){
    pnode->text[slen - 1] = ' ';
  }
  memcpy(pnode->text + slen, ws, feol - ws + 1);
  pnode->text[slen + (feol - ws + 1)] = '\0';
  return pnode->text;
}

// extract the page structure.
static int
troff_parse(const unsigned char* map, size_t mlen, pagedom* dom){
  const struct troffnode* trie = dom->trie;
  const unsigned char* line = map;
  pagenode* current_section = NULL;
  pagenode* current_subsection = NULL;
  pagenode* current_para = NULL;
  for(size_t off = 0 ; off < mlen ; ++off){
    const unsigned char* ws = line;
    size_t left = mlen - off;
    const trofftype* node = get_type(trie, &ws, left);
    // find the end of this line
    const unsigned char* eol = ws;
    left -= (ws - line);
    while(left && *eol != '\n' && *eol){
      ++eol;
      --left;
    }
    // functional end of line--doesn't include possible newline
    const unsigned char* feol = eol;
    if(left && *eol == '\n'){
      --feol;
    }
    if(node){
      if(node->ltype == LINE_TH){
        if(dom_get_title(dom)){
          fprintf(stderr, "found a second title (was %s)\n", dom_get_title(dom));
          return -1;
        }
        char* et = extract_text(ws, feol);
        if(et == NULL){
          return -1;
        }
        if((dom->root = malloc(sizeof(*dom->root))) == NULL){
          free(et);
          return -1;
        }
        memset(dom->root, 0, sizeof(*dom->root));
        dom->root->ttype = node;
        dom->root->text = et;
        if(lex_title(dom)){
          return -1;
        }
        current_para = dom->root;
      }else if(node->ltype == LINE_SH){
        if(dom->root == NULL){
          fprintf(stderr, "section transcends structure\n");
          return -1;
        }
        char* et = extract_text(ws, feol);
        if(et == NULL){
          return -1;
        }
        if((current_section = add_node(dom->root, et)) == NULL){
          free(et);
          return -1;
        }
        current_section->ttype = node;
        current_subsection = NULL;
        current_para = current_section;
      }else if(node->ltype == LINE_SS){
        char* et = extract_text(ws, feol);
        if(et == NULL){
          return -1;
        }
        if(current_section == NULL){
          fprintf(stderr, "subsection %s without section\n", et);
          free(et);
          return -1;
        }
        if((current_subsection = add_node(current_section, et)) == NULL){
          free(et);
          return -1;
        }
        current_subsection->ttype = node;
        current_para = current_subsection;
      }else if(node->ltype == LINE_PP){
        if(dom->root == NULL){
          fprintf(stderr, "paragraph transcends structure\n");
          return -1;
        }
        if((current_para = add_node(current_para, NULL)) == NULL){
          return -1;
        }
        current_para->ttype = node;
      }else if(node->ltype == LINE_TP){
        if(dom->root == NULL){
          fprintf(stderr, "tagged paragraph transcends structure\n");
          return -1;
        }
        if((current_para = add_node(current_para, NULL)) == NULL){
          return -1;
        }
        current_para->ttype = node;
      }
    }else{
      if(current_para == NULL){
        //fprintf(stderr, "free-floating text transcends para\n");
        //fprintf(stderr, "[%s]\n", line);
      }else{
        char* et = augment_text(current_para, line, feol);
        if(et == NULL){
          return -1;
        }
      }
    }
    off += eol - line;
    line = eol + 1;
  }
  if(dom_get_title(dom) == NULL){
    fprintf(stderr, "no title found\n");
    return -1;
  }
  return 0;
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
          if(*++curend != '['){
            fprintf(stderr, "illegal font macro %s\n", curend);
            return -1;
          }
          while(isalpha(*++curend)){
            switch(toupper(*curend)){
              case 'R': style = 0; break; // roman, default
              case 'I': style |= NCSTYLE_ITALIC; break;
              case 'B': style |= NCSTYLE_BOLD; break;
              default:
                fprintf(stderr, "illegal font macro %s\n", curend);
                return -1;
            }
          }
          if(*curend != ']'){
            fprintf(stderr, "illegal font macro %s\n", curend);
            return -1;
          }
          ++curend;
          break;
        }else if(*curend == '['){ // escaped sequence
          textend = curend - 1; // account for backslash
          static const struct {
            const char* macro;
            const char* tr;
          } macros[] = {
            { "aq]", "'", },
            { "dq]", "\"", },
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
            fprintf(stderr, "unknown macro %s\n", curend);
            return -1;
          }
          curend = macend;
          break;
        }
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
             unsigned* wrotetext){
  ncplane_set_fchannel(p, n->ttype->channel);
  size_t b = 0;
  switch(n->ttype->ltype){
    case LINE_TH: /*
      ncplane_set_styles(p, NCSTYLE_UNDERLINE);
      ncplane_printf_aligned(p, 0, NCALIGN_LEFT, "%s(%s)", dom->title, dom->section);
      ncplane_printf_aligned(p, 0, NCALIGN_RIGHT, "%s(%s)", dom->title, dom->section);
      ncplane_set_styles(p, NCSTYLE_NONE);
      */break;
    case LINE_SH: // section heading
      if(strcmp(n->text, "NAME")){
        ncplane_puttext(p, -1, NCALIGN_LEFT, "\n\n", &b);
        ncplane_set_styles(p, NCSTYLE_BOLD);
        ncplane_putstr_aligned(p, -1, NCALIGN_CENTER, n->text);
        ncplane_set_styles(p, NCSTYLE_NONE);
        ncplane_cursor_move_yx(p, -1, 0);
        *wrotetext = true;
      }
      break;
    case LINE_SS: // subsection heading
      ncplane_puttext(p, -1, NCALIGN_LEFT, "\n\n", &b);
      ncplane_set_styles(p, NCSTYLE_ITALIC);
      ncplane_putstr_aligned(p, -1, NCALIGN_CENTER, n->text);
      ncplane_set_styles(p, NCSTYLE_NONE);
      ncplane_cursor_move_yx(p, -1, 0);
      *wrotetext = true;
      break;
    case LINE_PP: // paragraph
    case LINE_TP: // tagged paragraph
      if(*wrotetext){
        if(n->text){
          ncplane_puttext(p, -1, NCALIGN_LEFT, "\n\n", &b);
          putpara(p, n->text);
        }
      }else{
        ncplane_set_styles(p, NCSTYLE_BOLD | NCSTYLE_ITALIC | NCSTYLE_UNDERLINE);
        ncplane_set_fg_rgb(p, 0xff6a00);
        ncplane_putstr_aligned(p, -1, NCALIGN_CENTER, n->text);
        ncplane_set_fg_default(p);
        ncplane_set_styles(p, NCSTYLE_NONE);
      }
      *wrotetext = true;
      break;
    default:
      fprintf(stderr, "unhandled ltype %d\n", n->ttype->ltype);
      return 0; // FIXME
  }
  for(unsigned z = 0 ; z < n->subcount ; ++z){
    if(draw_domnode(p, dom, &n->subs[z], wrotetext)){
      return -1;
    }
  }
  return 0;
}

// for now, we draw the entire thing, resizing as necessary, and we'll
// scroll the entire plane. higher memory cost, longer initial latency,
// very fast moves.
static int
draw_content(struct ncplane* p){
  const pagedom* dom = ncplane_userptr(p);
  unsigned wrotetext = 0; // unused by us
  return draw_domnode(p, dom, dom->root, &wrotetext);
}

static int
resize_pman(struct ncplane* pman){
  unsigned dimy, dimx;
  ncplane_dim_yx(ncplane_parent_const(pman), &dimy, &dimx);
  ncplane_resize_simple(pman, dimy - 1, dimx);
  int r = draw_content(pman);
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
    .rows = dimy - 1,
    .cols = dimx,
    .userptr = dom,
    .resizecb = resize_pman,
    .flags = NCPLANE_OPTION_AUTOGROW | NCPLANE_OPTION_VSCROLL,
  };
  struct ncplane* pman = ncplane_create(stdn, &popts);
  if(pman == NULL){
    return NULL;
  }
  if(draw_content(pman)){
    ncplane_destroy(pman);
    return NULL;
  }
  return pman;
}

static const char USAGE_TEXT[] = "(k↑/j↓) (q)uit";

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
  ncplane_putstr_aligned(bar, 0, NCALIGN_RIGHT, USAGE_TEXT);
  return 0;
}

static int
resize_bar(struct ncplane* bar){
  unsigned dimy, dimx;
  ncplane_dim_yx(ncplane_parent_const(bar), &dimy, &dimx);
  ncplane_resize_simple(bar, 1, dimx);
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
manloop(struct notcurses* nc, const char* arg){
  struct ncplane* stdn = notcurses_stdplane(nc);
  int ret = -1;
  struct ncplane* page = NULL;
  struct ncplane* bar = NULL;
  pagedom dom = {};
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
  uint32_t key;
  do{
    if(notcurses_render(nc)){
      goto done;
    }
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
      case 'k': case NCKEY_UP:
        if(ncplane_y(page)){
          ncplane_move_rel(page, 1, 0);
        }
        break;
      case 'j': case NCKEY_DOWN:
        if(ncplane_y(page) + ncplane_dim_y(page) > ncplane_dim_y(stdn)){
          ncplane_move_rel(page, -1, 0);
        }
        break;
      case 'q':
        ret = 0;
        goto done;
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
ncman(struct notcurses* nc, const char* arg){
  int r = manloop(nc, arg);
  return r;
}

int main(int argc, char** argv){
  int nonopt = parse_args(argc, argv);
  if(nonopt <= 0){
    return EXIT_FAILURE;
  }
  struct notcurses_options nopts = {
  };
  struct notcurses* nc = notcurses_core_init(&nopts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  bool success;
  for(int i = 0 ; i < argc - nonopt ; ++i){
    success = false;
    if(ncman(nc, argv[nonopt + i])){
      break;
    }
    success = true;
  }
  return notcurses_stop(nc) || !success ? EXIT_FAILURE : EXIT_SUCCESS;
}
