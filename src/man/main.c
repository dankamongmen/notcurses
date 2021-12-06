#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
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
  size_t outbytes;
  enum libdeflate_result r;
  r = libdeflate_gzip_decompress(inflate, buf, *len, ubuf, ulen, &outbytes);
  munmap(buf, *len);
  libdeflate_free_decompressor(inflate);
  if(r != LIBDEFLATE_SUCCESS){
    return NULL;
  }
  *len = ulen;
  return ubuf;
}
#else // libz implementation
#error libz not yet implemented, need libdeflate
static unsigned char*
map_gzipped_data(unsigned char* buf, size_t* len, unsigned char* ubuf, uint32_t ulen){
  munmap(buf, *len);
  (void)ulen; // FIXME
  return NULL;
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
                            MAP_PRIVATE | MAP_POPULATE, fd, 0);
  if(buf == MAP_FAILED){
    return NULL;
  }
  if(buf[0] == 0x1f && buf[1] == 0x8b && buf[2] == 0x08){
    // the last four bytes have the uncompressed length
    uint32_t ulen;
    memcpy(&ulen, buf + *len - 4, 4);
    size_t pgsize = 4096; // FIXME
    void* ubuf = mmap(NULL, (ulen + pgsize - 1) / pgsize * pgsize,
                      PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if(ubuf == MAP_FAILED){
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
} trofftype;

// all troff types start with a period, followed by one or two ASCII
// characters.
static const trofftype trofftypes[] = {
  { .ltype = LINE_UNKNOWN, .symbol = "", .ttype = TROFF_UNKNOWN, },
  { .ltype = LINE_COMMENT, .symbol = "\\\"", .ttype = TROFF_COMMENT, },
#define TROFF_FONT(x) { .ltype = LINE_##x, .symbol = #x, .ttype = TROFF_FONT, },
  TROFF_FONT(B) TROFF_FONT(BI) TROFF_FONT(BR)
  TROFF_FONT(I) TROFF_FONT(IB) TROFF_FONT(IR)
#undef TROFF_FONT
#define TROFF_STRUCTURE(x) { .ltype = LINE_##x, .symbol = #x, .ttype = TROFF_STRUCTURE, },
  TROFF_STRUCTURE(EE) TROFF_STRUCTURE(EX) TROFF_STRUCTURE(RE) TROFF_STRUCTURE(RS)
  TROFF_STRUCTURE(SH) TROFF_STRUCTURE(SS) TROFF_STRUCTURE(TH)
#undef TROFF_STRUCTURE
#define TROFF_PARA(x) { .ltype = LINE_##x, .symbol = #x, .ttype = TROFF_PARAGRAPH, },
  TROFF_PARA(IP) TROFF_PARA(LP) TROFF_PARA(P)
  TROFF_PARA(PP) TROFF_PARA(TP) TROFF_PARA(TQ)
#undef TROFF_PARA
#define TROFF_HLINK(x) { .ltype = LINE_##x, .symbol = #x, .ttype = TROFF_HYPERLINK, },
  TROFF_HLINK(ME) TROFF_HLINK(MT) TROFF_HLINK(UE) TROFF_HLINK(UR)
#undef TROFF_HLINK
#define TROFF_SYNOPSIS(x) { .ltype = LINE_##x, .symbol = #x, .ttype = TROFF_SYNOPSIS, },
  TROFF_SYNOPSIS(OP) TROFF_SYNOPSIS(SY) TROFF_SYNOPSIS(YS)
#undef TROFF_SYNOPSIS
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
          memset(n->next[us], 0, sizeof(*root));
          goto err;
        }
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

// calculate the number of rows necessary to display the troff data,
// assuming the specified width |dimx|.
static int
troff_height(const struct troffnode* trie, unsigned dimx,
             const unsigned char* map, size_t mlen){
  // FIXME for now we assume infinitely wide lines for display
  int lines = 0;
  const unsigned char* line = map;
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
    if(node){
      if(node->ttype != TROFF_FONT && node->ttype != TROFF_COMMENT){
        ++lines;
      }
    }
    off += eol - line;
    line = eol + 1;
  }
  return lines;
}

// we create a plane sized appropriately for the troff data. all we do
// after that is move the plane up and down.
static struct ncplane*
render_troff(struct notcurses* nc, const unsigned char* map, size_t mlen){
  unsigned dimy, dimx;
  struct ncplane* stdn = notcurses_stddim_yx(nc, &dimy, &dimx);
  struct troffnode* trie = trofftrie();
  if(trie == NULL){
    return NULL;
  }
  int rows = troff_height(trie, dimx, map, mlen);
  struct ncplane_options popts = {
    .rows = rows,
    .cols = dimx,
  };
  struct ncplane* pman = ncplane_create(stdn, &popts);
  // FIXME draw it
  destroy_trofftrie(trie);
  return pman;
}

static int
manloop(struct notcurses* nc, const char* arg){
  size_t len;
  unsigned char* buf = get_troff_data(arg, &len);
  if(buf == NULL){
    return -1;
  }
  struct ncplane* page = render_troff(nc, buf, len);
  if(page == NULL){
    munmap(buf, len);
    return -1;
  }
  uint32_t key;
  do{
    if(notcurses_render(nc)){
      munmap(buf, len);
      return -1;
    }
    ncinput ni;
    key = notcurses_get(nc, NULL, &ni);
    switch(key){
      case 'q':
        munmap(buf, len);
        return 0;
    }
  }while(key != (uint32_t)-1);
  munmap(buf, len);
  return -1;
}

static int
ncman(struct notcurses* nc, const char* arg){
  // FIXME usage bar at bottom
  return manloop(nc, arg);
}

int main(int argc, char** argv){
  int nonopt = parse_args(argc, argv);
  if(nonopt <= 0){
    return EXIT_FAILURE;
  }
  struct notcurses_options nopts = {
    .flags = NCOPTION_NO_ALTERNATE_SCREEN,
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
