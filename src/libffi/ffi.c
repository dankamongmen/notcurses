// Contains all inline functions in include/notcurses/*.h
// This file is auto generated from tools/generate_ffi.py
#include <notcurses/notcurses.h>
#include <notcurses/direct.h>

#include <notcurses/nckeys.h>

bool nccapability_canchangecolor(const nccapabilities* caps);
bool nccell_bg_default_p(const nccell* cl);
bool nccell_bg_palindex_p(const nccell* cl);
bool nccell_double_wide_p(const nccell* c);
bool nccell_fg_default_p(const nccell* cl);
bool nccell_fg_palindex_p(const nccell* cl);
bool nccell_wide_left_p(const nccell* c);
bool nccell_wide_right_p(const nccell* c);
bool nccellcmp(const struct ncplane* n1, const nccell* restrict c1,
          const struct ncplane* n2, const nccell* restrict c2);
bool ncchannel_default_p(uint32_t channel);
bool ncchannel_palindex_p(uint32_t channel);
bool ncchannel_rgb_p(uint32_t channel);
bool ncchannels_bg_default_p(uint64_t channels);
bool ncchannels_bg_palindex_p(uint64_t channels);
bool ncchannels_bg_rgb_p(uint64_t channels);
bool ncchannels_fg_default_p(uint64_t channels);
bool ncchannels_fg_palindex_p(uint64_t channels);
bool ncchannels_fg_rgb_p(uint64_t channels);
bool ncdirect_canbraille(const struct ncdirect* nc);
bool ncdirect_canchangecolor(const struct ncdirect* n);
bool ncdirect_canfade(const struct ncdirect* n);
bool ncdirect_canhalfblock(const struct ncdirect* nc);
bool ncdirect_canopen_images(const struct ncdirect* n __attribute__ ((unused)));
bool ncdirect_canopen_videos(const struct ncdirect* n __attribute__ ((unused)));
bool ncdirect_canquadrant(const struct ncdirect* nc);
bool ncdirect_cansextant(const struct ncdirect* nc);
bool ncdirect_cantruecolor(const struct ncdirect* n);
bool ncinput_alt_p(const ncinput* n);
bool ncinput_capslock_p(const ncinput* n);
bool ncinput_ctrl_p(const ncinput* n);
bool ncinput_equal_p(const ncinput* n1, const ncinput* n2);
bool ncinput_hyper_p(const ncinput* n);
bool ncinput_meta_p(const ncinput* n);
bool ncinput_nomod_p(const ncinput* ni);
bool ncinput_numlock_p(const ncinput* n);
bool ncinput_shift_p(const ncinput* n);
bool ncinput_super_p(const ncinput* n);
bool nckey_mouse_p(uint32_t r);
bool nckey_pua_p(uint32_t w);
bool nckey_supppuaa_p(uint32_t w);
bool nckey_supppuab_p(uint32_t w);
bool nckey_synthesized_p(uint32_t w);
bool ncplane_bg_default_p(const struct ncplane* n);
bool ncplane_fg_default_p(const struct ncplane* n);
bool notcurses_canbraille(const struct notcurses* nc);
bool notcurses_canchangecolor(const struct notcurses* nc);
bool notcurses_canfade(const struct notcurses* n);
bool notcurses_canhalfblock(const struct notcurses* nc);
bool notcurses_canpixel(const struct notcurses* nc);
bool notcurses_canquadrant(const struct notcurses* nc);
bool notcurses_cansextant(const struct notcurses* nc);
bool notcurses_cantruecolor(const struct notcurses* nc);
bool notcurses_canutf8(const struct notcurses* nc);
char* nccell_extract(const struct ncplane* n, const nccell* c,
               uint16_t* stylemask, uint64_t* channels);
char* nccell_strdup(const struct ncplane* n, const nccell* c);
char* ncwcsrtombs(const wchar_t* src);
const char* ncbprefix(uintmax_t val, uintmax_t decimal, char* buf, int omitdec);
const char* nciprefix(uintmax_t val, uintmax_t decimal, char* buf, int omitdec);
const char* ncqprefix(uintmax_t val, uintmax_t decimal, char* buf, int omitdec);
const struct ncplane* notcurses_stddim_yx_const(const struct notcurses* nc, unsigned* restrict y, unsigned* restrict x);
int nccell_load_char(struct ncplane* n, nccell* c, char ch);
int nccell_load_egc32(struct ncplane* n, nccell* c, uint32_t egc);
int nccell_load_ucs32(struct ncplane* n, nccell* c, uint32_t u);
int nccell_prime(struct ncplane* n, nccell* c, const char* gcluster,
             uint16_t stylemask, uint64_t channels);
int nccell_set_bg_alpha(nccell* c, unsigned alpha);
int nccell_set_bg_palindex(nccell* cl, unsigned idx);
int nccell_set_bg_rgb(nccell* c, uint32_t channel);
int nccell_set_bg_rgb8(nccell* cl, unsigned r, unsigned g, unsigned b);
int nccell_set_fg_alpha(nccell* c, unsigned alpha);
int nccell_set_fg_palindex(nccell* cl, unsigned idx);
int nccell_set_fg_rgb(nccell* c, uint32_t channel);
int nccell_set_fg_rgb8(nccell* cl, unsigned r, unsigned g, unsigned b);
int nccells_ascii_box(struct ncplane* n, uint16_t attr, uint64_t channels,
                  nccell* ul, nccell* ur, nccell* ll, nccell* lr, nccell* hl, nccell* vl);
int nccells_double_box(struct ncplane* n, uint16_t attr, uint64_t channels,
                   nccell* ul, nccell* ur, nccell* ll, nccell* lr, nccell* hl, nccell* vl);
int nccells_heavy_box(struct ncplane* n, uint16_t attr, uint64_t channels,
                  nccell* ul, nccell* ur, nccell* ll, nccell* lr, nccell* hl, nccell* vl);
int nccells_light_box(struct ncplane* n, uint16_t attr, uint64_t channels,
                  nccell* ul, nccell* ur, nccell* ll, nccell* lr, nccell* hl, nccell* vl);
int nccells_load_box(struct ncplane* n, uint16_t styles, uint64_t channels,
                 nccell* ul, nccell* ur, nccell* ll, nccell* lr,
                 nccell* hl, nccell* vl, const char* gclusters);
int nccells_rounded_box(struct ncplane* n, uint16_t attr, uint64_t channels,
                    nccell* ul, nccell* ur, nccell* ll, nccell* lr, nccell* hl, nccell* vl);
int ncchannel_set(uint32_t* channel, uint32_t rgb);
int ncchannel_set_alpha(uint32_t* channel, unsigned alpha);
int ncchannel_set_palindex(uint32_t* channel, unsigned idx);
int ncchannel_set_rgb8(uint32_t* channel, unsigned r, unsigned g, unsigned b);
int ncchannels_set_bg_alpha(uint64_t* channels, unsigned alpha);
int ncchannels_set_bg_palindex(uint64_t* channels, unsigned idx);
int ncchannels_set_bg_rgb(uint64_t* channels, unsigned rgb);
int ncchannels_set_bg_rgb8(uint64_t* channels, unsigned r, unsigned g, unsigned b);
int ncchannels_set_fg_alpha(uint64_t* channels, unsigned alpha);
int ncchannels_set_fg_palindex(uint64_t* channels, unsigned idx);
int ncchannels_set_fg_rgb(uint64_t* channels, unsigned rgb);
int ncchannels_set_fg_rgb8(uint64_t* channels, unsigned r, unsigned g, unsigned b);
int ncdirect_ascii_box(struct ncdirect* n, uint64_t ul, uint64_t ur,
                   uint64_t ll, uint64_t lr,
                   unsigned ylen, unsigned xlen, unsigned ctlword);
int ncdirect_heavy_box(struct ncdirect* n, uint64_t ul, uint64_t ur,
                   uint64_t ll, uint64_t lr,
                   unsigned ylen, unsigned xlen, unsigned ctlword);
int ncdirect_light_box(struct ncdirect* n, uint64_t ul, uint64_t ur,
                   uint64_t ll, uint64_t lr,
                   unsigned ylen, unsigned xlen, unsigned ctlword);
int ncdirect_set_bg_rgb8(struct ncdirect* nc, unsigned r, unsigned g, unsigned b);
int ncdirect_set_fg_rgb8(struct ncdirect* nc, unsigned r, unsigned g, unsigned b);
int ncpalette_get(const ncpalette* p, int idx, uint32_t* palent);
int ncpalette_get_rgb8(const ncpalette* p, int idx, unsigned* restrict r, unsigned* restrict g, unsigned* restrict b);
int ncpalette_set(ncpalette* p, int idx, unsigned rgb);
int ncpalette_set_rgb8(ncpalette* p, int idx, unsigned r, unsigned g, unsigned b);
int ncpixel_set_a(uint32_t* pixel, unsigned a);
int ncpixel_set_b(uint32_t* pixel, unsigned b);
int ncpixel_set_g(uint32_t* pixel, unsigned g);
int ncpixel_set_r(uint32_t* pixel, unsigned r);
int ncpixel_set_rgb8(uint32_t* pixel, unsigned r, unsigned g, unsigned b);
int ncplane_ascii_box(struct ncplane* n, uint16_t styles, uint64_t channels,
                  unsigned ylen, unsigned xlen, unsigned ctlword);
int ncplane_box_sized(struct ncplane* n, const nccell* ul, const nccell* ur,
                  const nccell* ll, const nccell* lr, const nccell* hline,
                  const nccell* vline, unsigned ystop, unsigned xstop,
                  unsigned ctlword);
int ncplane_descendant_p(const struct ncplane* n, const struct ncplane* ancestor);
int ncplane_double_box(struct ncplane* n, uint16_t styles, uint64_t channels,
                   unsigned ylen, unsigned xlen, unsigned ctlword);
int ncplane_double_box_sized(struct ncplane* n, uint16_t styles, uint64_t channels,
                         unsigned ylen, unsigned xlen, unsigned ctlword);
int ncplane_halign(const struct ncplane* n, ncalign_e align, int c);
int ncplane_hline(struct ncplane* n, const nccell* c, unsigned len);
int ncplane_move_rel(struct ncplane* n, int y, int x);
int ncplane_perimeter(struct ncplane* n, const nccell* ul, const nccell* ur,
                  const nccell* ll, const nccell* lr, const nccell* hline,
                  const nccell* vline, unsigned ctlword);
int ncplane_perimeter_double(struct ncplane* n, uint16_t stylemask,
                         uint64_t channels, unsigned ctlword);
int ncplane_perimeter_rounded(struct ncplane* n, uint16_t stylemask,
                          uint64_t channels, unsigned ctlword);
int ncplane_printf(struct ncplane* n, const char* format, ...);
int ncplane_printf_aligned(struct ncplane* n, int y, ncalign_e align, const char* format, ...);
int ncplane_printf_stained(struct ncplane* n, const char* format, ...);
int ncplane_printf_yx(struct ncplane* n, int y, int x, const char* format, ...);
int ncplane_putc(struct ncplane* n, const nccell* c);
int ncplane_putchar(struct ncplane* n, char c);
int ncplane_putchar_yx(struct ncplane* n, int y, int x, char c);
int ncplane_putegc(struct ncplane* n, const char* gclust, size_t* sbytes);
int ncplane_putnstr(struct ncplane* n, size_t s, const char* gclustarr);
int ncplane_putnstr_yx(struct ncplane* n, int y, int x, size_t s, const char* gclusters);
int ncplane_putstr(struct ncplane* n, const char* gclustarr);
int ncplane_putstr_aligned(struct ncplane* n, int y, ncalign_e align, const char* s);
int ncplane_putstr_stained(struct ncplane* n, const char* gclusters);
int ncplane_putstr_yx(struct ncplane* n, int y, int x, const char* gclusters);
int ncplane_pututf32_yx(struct ncplane* n, int y, int x, uint32_t u);
int ncplane_putwc(struct ncplane* n, wchar_t w);
int ncplane_putwc_stained(struct ncplane* n, wchar_t w);
int ncplane_putwc_utf32(struct ncplane* n, const wchar_t* w, unsigned* wchars);
int ncplane_putwc_yx(struct ncplane* n, int y, int x, wchar_t w);
int ncplane_putwegc(struct ncplane* n, const wchar_t* gclust, size_t* sbytes);
int ncplane_putwegc_yx(struct ncplane* n, int y, int x, const wchar_t* gclust,
                   size_t* sbytes);
int ncplane_putwstr(struct ncplane* n, const wchar_t* gclustarr);
int ncplane_putwstr_aligned(struct ncplane* n, int y, ncalign_e align,
                        const wchar_t* gclustarr);
int ncplane_putwstr_yx(struct ncplane* n, int y, int x, const wchar_t* gclustarr);
int ncplane_resize_simple(struct ncplane* n, unsigned ylen, unsigned xlen);
int ncplane_rounded_box(struct ncplane* n, uint16_t styles, uint64_t channels,
                    unsigned ystop, unsigned xstop, unsigned ctlword);
int ncplane_rounded_box_sized(struct ncplane* n, uint16_t styles, uint64_t channels,
                          unsigned ylen, unsigned xlen, unsigned ctlword);
int ncplane_valign(const struct ncplane* n, ncalign_e align, int r);
int ncplane_vline(struct ncplane* n, const nccell* c, unsigned len);
int ncplane_vprintf(struct ncplane* n, const char* format, va_list ap);
int notcurses_align(int availu, ncalign_e align, int u);
int notcurses_mice_disable(struct notcurses* n);
int notcurses_render(struct notcurses* nc);
struct ncplane* ncvisualplane_create(struct notcurses* nc, const struct ncplane_options* opts,
                     struct ncvisual* ncv, struct ncvisual_options* vopts);
struct ncplane* notcurses_bottom(struct notcurses* n);
struct ncplane* notcurses_stddim_yx(struct notcurses* nc, unsigned* restrict y, unsigned* restrict x);
struct ncplane* notcurses_top(struct notcurses* n);
uint16_t nccell_styles(const nccell* c);
uint32_t nccell_bchannel(const nccell* cl);
uint32_t nccell_bg_alpha(const nccell* cl);
uint32_t nccell_bg_palindex(const nccell* cl);
uint32_t nccell_bg_rgb(const nccell* cl);
uint32_t nccell_bg_rgb8(const nccell* cl, unsigned* r, unsigned* g, unsigned* b);
uint32_t nccell_fchannel(const nccell* cl);
uint32_t nccell_fg_alpha(const nccell* cl);
uint32_t nccell_fg_palindex(const nccell* cl);
uint32_t nccell_fg_rgb(const nccell* cl);
uint32_t nccell_fg_rgb8(const nccell* cl, unsigned* r, unsigned* g, unsigned* b);
uint32_t ncchannel_rgb(uint32_t channel);
uint32_t ncchannel_rgb8(uint32_t channel, unsigned* restrict r, unsigned* restrict g,
               unsigned* restrict b);
uint32_t ncchannel_set_default(uint32_t* channel);
uint32_t ncchannels_bchannel(uint64_t channels);
uint32_t ncchannels_bg_rgb(uint64_t channels);
uint32_t ncchannels_bg_rgb8(uint64_t channels, unsigned* r, unsigned* g, unsigned* b);
uint32_t ncchannels_fchannel(uint64_t channels);
uint32_t ncchannels_fg_rgb(uint64_t channels);
uint32_t ncchannels_fg_rgb8(uint64_t channels, unsigned* r, unsigned* g, unsigned* b);
uint32_t ncdirect_get_blocking(struct ncdirect* n, ncinput* ni);
uint32_t ncdirect_get_nblock(struct ncdirect* n, ncinput* ni);
uint32_t ncpixel(unsigned r, unsigned g, unsigned b);
uint32_t ncplane_bchannel(const struct ncplane* n);
uint32_t ncplane_bg_alpha(const struct ncplane* n);
uint32_t ncplane_bg_rgb(const struct ncplane* n);
uint32_t ncplane_bg_rgb8(const struct ncplane* n, unsigned* r, unsigned* g, unsigned* b);
uint32_t ncplane_fchannel(const struct ncplane* n);
uint32_t ncplane_fg_alpha(const struct ncplane* n);
uint32_t ncplane_fg_rgb(const struct ncplane* n);
uint32_t ncplane_fg_rgb8(const struct ncplane* n, unsigned* r, unsigned* g, unsigned* b);
uint32_t notcurses_get_blocking(struct notcurses* n, ncinput* ni);
uint32_t notcurses_get_nblock(struct notcurses* n, ncinput* ni);
uint64_t nccell_channels(const nccell* c);
uint64_t nccell_set_bchannel(nccell* c, uint32_t channel);
uint64_t nccell_set_channels(nccell* c, uint64_t channels);
uint64_t nccell_set_fchannel(nccell* c, uint32_t channel);
uint64_t ncchannels_channels(uint64_t channels);
uint64_t ncchannels_combine(uint32_t fchan, uint32_t bchan);
uint64_t ncchannels_reverse(uint64_t channels);
uint64_t ncchannels_set_bchannel(uint64_t* channels, uint32_t channel);
uint64_t ncchannels_set_bg_default(uint64_t* channels);
uint64_t ncchannels_set_channels(uint64_t* dst, uint64_t channels);
uint64_t ncchannels_set_fchannel(uint64_t* channels, uint32_t channel);
uint64_t ncchannels_set_fg_default(uint64_t* channels);
uint64_t nctabbed_hdrchan(struct nctabbed* nt);
uint64_t nctabbed_selchan(struct nctabbed* nt);
uint64_t nctabbed_sepchan(struct nctabbed* nt);
unsigned nccell_cols(const nccell* c);
unsigned ncchannel_alpha(uint32_t channel);
unsigned ncchannel_b(uint32_t channel);
unsigned ncchannel_g(uint32_t channel);
unsigned ncchannel_palindex(uint32_t channel);
unsigned ncchannel_r(uint32_t channel);
unsigned ncchannels_bg_alpha(uint64_t channels);
unsigned ncchannels_bg_palindex(uint64_t channels);
unsigned ncchannels_fg_alpha(uint64_t channels);
unsigned ncchannels_fg_palindex(uint64_t channels);
unsigned ncpixel_a(uint32_t pixel);
unsigned ncpixel_b(uint32_t pixel);
unsigned ncpixel_g(uint32_t pixel);
unsigned ncpixel_r(uint32_t pixel);
unsigned ncplane_cursor_x(const struct ncplane* n);
unsigned ncplane_cursor_y(const struct ncplane* n);
unsigned ncplane_dim_x(const struct ncplane* n);
unsigned ncplane_dim_y(const struct ncplane* n);
void nccell_init(nccell* c);
void nccell_off_styles(nccell* c, unsigned stylebits);
void nccell_on_styles(nccell* c, unsigned stylebits);
void nccell_set_bg_default(nccell* c);
void nccell_set_bg_rgb8_clipped(nccell* cl, int r, int g, int b);
void nccell_set_fg_default(nccell* c);
void nccell_set_fg_rgb8_clipped(nccell* cl, int r, int g, int b);
void nccell_set_styles(nccell* c, unsigned stylebits);
void ncchannel_set_rgb8_clipped(uint32_t* channel, int r, int g, int b);
void ncchannels_set_bg_rgb8_clipped(uint64_t* channels, int r, int g, int b);
void ncchannels_set_fg_rgb8_clipped(uint64_t* channels, int r, int g, int b);
void ncplane_move_bottom(struct ncplane* n);
void ncplane_move_family_bottom(struct ncplane* n);
void ncplane_move_family_top(struct ncplane* n);
void ncplane_move_top(struct ncplane* n);
void notcurses_term_dim_yx(const struct notcurses* n, unsigned* restrict rows, unsigned* restrict cols);
