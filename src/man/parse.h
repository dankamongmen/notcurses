#ifndef NOTCURSES_MAN_PARSE
#define NOTCURSES_MAN_PARSE

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>

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
  LINE_NF, LINE_FI,
} ltypes;

typedef enum {
  TROFF_UNKNOWN,
  TROFF_COMMENT,
  TROFF_FONT,
  TROFF_STRUCTURE,
  TROFF_PARAGRAPH,
  TROFF_HYPERLINK,
  TROFF_SYNOPSIS,
  TROFF_PREFORMATTED,
} ttypes;

typedef struct {
  ltypes ltype;
  const char* symbol;
  ttypes ttype;
  uint32_t channel;
} trofftype;

// the troff trie is only defined on the 128 ascii values.
struct troffnode {
  struct troffnode* next[0x80];
  const trofftype *ttype;
};

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
  struct docstructure* ds;
} pagedom;

static inline const char*
dom_get_title(const pagedom* dom){
  return dom->title;
}

const trofftype*
get_type(const struct troffnode* trie, const unsigned char** ws, size_t len);

struct troffnode* trofftrie(void);

int troff_parse(const unsigned char* map, size_t mlen, pagedom* dom);

void destroy_trofftrie(struct troffnode* root);

#ifdef __cplusplus
}
#endif

#endif
