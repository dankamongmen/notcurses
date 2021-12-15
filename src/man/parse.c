#include <wctype.h>
#include <notcurses/notcurses.h>
#include "parse.h"

// this is pretty much all trash

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
  { .ltype = LINE_NF, .symbol = "nf", .ttype = TROFF_FONT, .channel = 0, },
  { .ltype = LINE_FI, .symbol = "fi", .ttype = TROFF_FONT, .channel = 0, },
};

void destroy_trofftrie(struct troffnode* root){
  if(root){
    for(unsigned i = 0 ; i < sizeof(root->next) / sizeof(*root->next) ; ++i){
      destroy_trofftrie(root->next[i]);
    }
    free(root);
  }
}

// build a trie rooted at an implicit leading period.
struct troffnode* trofftrie(void){
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
      if(*s <= 0){ // illegal symbol
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
const trofftype* get_type(const struct troffnode* trie, const unsigned char** ws,
                          size_t len){
  if(**ws != '.'){
    return NULL;
  }
  ++*ws;
  --len;
  while(len && !isspace(**ws) && **ws){
    if(**ws >= sizeof(trie->next) / sizeof(*trie->next)){ // illegal command
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

// get the next token. first, chew whitespace. then match a string of
// iswgraph(), or a quoted string of iswprint(). return the number of
// characters consumed, or -1 on error (no token, unterminated quote).
// heap-copies the utf8 to *token on success.
static int
lex_next_token(const char* s, char** token){
  mbstate_t ps = {0};
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
// FIXME we need to fuzz this, hard
int troff_parse(const unsigned char* map, size_t mlen, pagedom* dom){
  const struct troffnode* trie = dom->trie;
  const unsigned char* line = map;
  pagenode* current_section = NULL;
  pagenode* current_subsection = NULL;
  pagenode* current_para = NULL;
  bool preformatted = false;
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
    const unsigned char* feol = eol;
    // functional end of line--doesn't include possible newline
    if(!preformatted){
      if(left && *eol == '\n'){
        --feol;
      }
    }
    if(node == NULL){
      if(current_para == NULL){
        //fprintf(stderr, "free-floating text transcends para\n");
        //fprintf(stderr, "[%s]\n", line);
      }else{
        char* et = augment_text(current_para, line, feol);
        if(et == NULL){
          return -1;
        }
      }
    }else if(node->ltype == LINE_NF){
      preformatted = true;
    }else if(node->ltype == LINE_FI){
      preformatted = false;
    }else if(node->ltype == LINE_TH){
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
    off += eol - line;
    line = eol + 1;
  }
  if(dom_get_title(dom) == NULL){
    fprintf(stderr, "no title found\n");
    return -1;
  }
  return 0;
}
