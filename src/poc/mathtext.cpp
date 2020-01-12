#include <cstdlib>
#include <locale.h>
#include <unistd.h>
#include <notcurses.h>

int mathtext(struct notcurses* nc){
  int dimx, dimy;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  const int HEIGHT = 9;
  const int WIDTH = dimx;
  struct ncplane* n = ncplane_new(nc, HEIGHT, WIDTH, dimy - HEIGHT - 1, dimx - WIDTH - 1, NULL);
  cell b = CELL_TRIVIAL_INITIALIZER;
  cell_set_bg_alpha(&b, CELL_ALPHA_TRANSPARENT);
  cell_set_fg_alpha(&b, CELL_ALPHA_TRANSPARENT);
  ncplane_set_base(n, &b);
  cell_release(n, &b);
  if(n){
    struct ncplane* stdn = notcurses_stdplane(nc);
    ncplane_set_bg_alpha(n, CELL_ALPHA_TRANSPARENT);
    // FIXME reenable the left parts of these strings, issue #260*/
    ncplane_printf_aligned(n, 0, NCALIGN_RIGHT, /*∮E⋅da=Q,n→∞,∑f(i)=∏g(i)*/"⎧⎡⎛┌─────┐⎞⎤⎫");
    ncplane_printf_aligned(n, 1, NCALIGN_RIGHT, "⎪⎢⎜│a²+b³ ⎟⎥⎪");
    ncplane_printf_aligned(n, 2, NCALIGN_RIGHT, /*∀x∈ℝ:⌈x⌉=−⌊−x⌋,α∧¬β=¬(¬α∨β)*/"⎪⎢⎜│───── ⎟⎥⎪");
    ncplane_printf_aligned(n, 3, NCALIGN_RIGHT, "⎪⎢⎜⎷ c₈   ⎟⎥⎪");
    ncplane_printf_aligned(n, 4, NCALIGN_RIGHT, /*ℕ⊆ℕ₀⊂ℤ⊂ℚ⊂ℝ⊂ℂ(z̄=ℜ(z)−ℑ(z)⋅𝑖)*/"⎨⎢⎜       ⎟⎥⎬");
    ncplane_printf_aligned(n, 5, NCALIGN_RIGHT, "⎪⎢⎜ ∞     ⎟⎥⎪");
    ncplane_printf_aligned(n, 6, NCALIGN_RIGHT, /*⊥<a≠b≡c≤d≪⊤⇒(⟦A⟧⇔⟪B⟫)*/"⎪⎢⎜ ⎲     ⎟⎥⎪");
    ncplane_printf_aligned(n, 7, NCALIGN_RIGHT, "⎪⎢⎜ ⎳aⁱ-bⁱ⎟⎥⎪");
    ncplane_printf_aligned(n, 8, NCALIGN_RIGHT, /*2H₂+O₂⇌2H₂O,R=4.7kΩ,⌀200µm*/"⎩⎣⎝i=1    ⎠⎦⎭");
  }
  return 0;
}

int main(void){
  if(setlocale(LC_ALL, "") == nullptr){
    return EXIT_FAILURE;
  }
  notcurses_options opts{};
  opts.inhibit_alternate_screen = true;
  struct notcurses* nc = notcurses_init(&opts, stdout);
  if(nc == nullptr){
    return EXIT_FAILURE;
  }
  if(mathtext(nc)){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  notcurses_render(nc);
  notcurses_stop(nc);
  return 0;
}
