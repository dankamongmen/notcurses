#include <notcurses/notcurses.h>
#include "parse.h"

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
