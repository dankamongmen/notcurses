#ifndef NOTCURSES_AUTOMATON
#define NOTCURSES_AUTOMATON

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

struct ncinput;
struct esctrie;
struct inputctx;

// the state necessary for matching input against our automaton of control
// sequences. we *do not* match the bulk UTF-8 input. we match online (i.e.
// we can be passed a byte at a time).
typedef struct automaton {
  struct esctrie* escapes;  // head Esc node of trie
  int used;                 // bytes consumed thus far
  // FIXME need an array to track the path
  struct esctrie* state;
  unsigned stridx;          // bytes of accumulating string (includes NUL)
} automaton;

void input_free_esctrie(automaton *a);

int inputctx_add_input_escape(automaton* a, const char* esc,
                              uint32_t special, unsigned shift,
                              unsigned ctrl, unsigned alt);

int walk_automaton(automaton* a, struct inputctx* ictx, unsigned candidate,
                   struct ncinput* ni)
  __attribute__ ((nonnull (1, 2, 4)));

uint32_t esctrie_id(const struct esctrie* e);
// returns 128-way array of esctrie pointers
struct esctrie** esctrie_trie(struct esctrie* e);

#ifdef __cplusplus
}
#endif

#endif
