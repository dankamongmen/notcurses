#ifndef NOTCURSES_INPUT
#define NOTCURSES_INPUT

#ifdef __cplusplus
extern "C" {
#endif

// internal header, not installed

#include <stdio.h>

struct tinfo;
struct termios;
struct ncinputlayer;

typedef enum {
    TERMINAL_UNKNOWN,       // no useful information from queries; use termname
    TERMINAL_LINUX,         // ioctl()s
    TERMINAL_XTERM,         // XTVERSION == 'XTerm(ver)'
    TERMINAL_VTE,           // TDA: "~VTE"
    TERMINAL_KITTY,         // XTGETTCAP['TN'] == 'xterm-kitty'
    TERMINAL_FOOT,          // TDA: "\EP!|464f4f54\E\\"
    TERMINAL_MLTERM,        // XTGETTCAP['TN'] == 'mlterm'
    TERMINAL_WEZTERM,       // XTVERSION == 'WezTerm *'
    TERMINAL_ALACRITTY,     // can't be detected; match TERM
    TERMINAL_CONTOUR,       // XTVERSION == 'contour ver'
    TERMINAL_ITERM,         // XTVERSION == 'iTerm2 [ver]'
} queried_terminals_e;

// sets up the input layer, building a trie of escape sequences and their
// nckey equivalents. if we are connected to a tty, this also completes the
// terminal detection sequence (we ought have already written our initial
// queries, ideally as early as possible). if we are able to determine the
// terminal conclusively, it will be written to |detected|. if the terminal
// advertised support for application-sychronized updates, |appsync| will be
// non-zero.
int ncinputlayer_init(struct tinfo* tcache, FILE* infp,
                      queried_terminals_e* detected, unsigned* appsync,
                      int* cursor_y, int* cursor_x);

void ncinputlayer_stop(struct ncinputlayer* nilayer);

// FIXME absorb into ncinputlayer_init()
int cbreak_mode(int ttyfd, const struct termios* tpreserved);

#ifdef __cplusplus
}
#endif

#endif
