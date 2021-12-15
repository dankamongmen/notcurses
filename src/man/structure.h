#ifndef NOTCURSES_MAN_STRUCTURE
#define NOTCURSES_MAN_STRUCTURE

#ifdef __cplusplus
extern "C" {
#endif

struct ncplane;
struct docstructure;

typedef enum {
  DOCSTRUCTURE_TITLE,
  DOCSTRUCTURE_SECTION,
  DOCSTRUCTURE_SUBSECTION,
} docstruct_e;

struct docstructure* docstructure_create(struct ncplane* n);
void docstructure_free(struct docstructure* ds);

void docstructure_toggle(struct ncplane* p, struct ncplane* b, struct docstructure *ds);

// add the specified [sub]section to the document strucure. |line| refers to
// the row on the display plane, *not* the line in the original content.
int docstructure_add(struct docstructure* ds, const char* title, int line,
                     docstruct_e level, unsigned y);

// update the docstructure browser based off a move of the page plane from to |newy|.
int docstructure_move(struct docstructure* ds, unsigned newy);

#ifdef __cplusplus
}
#endif

#endif
