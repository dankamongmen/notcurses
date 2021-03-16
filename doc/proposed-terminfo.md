# Some desirable terminfo capabilities

## Semigraphics

### blocks

The `blocks` capability indicates that block characters are drawn by the
terminal itself, rather than through rendering the current font. This should
cover at least:

* The entirety of the [Block Elements](https://www.unicode.org/charts/PDF/U2580.pdf)
   block (U+2580–U+259F).
* The entirety of the [Geometric Shapes](https://www.unicode.org/charts/PDF/U25A0.pdf)
   block (U+25A0–U+25FF).
* The [Symbols for Legacy Computing](https://www.unicode.org/charts/PDF/U1FB00.pdf)
   block from U+1FB00 through U+1FBAF, except U+1FB70–U+1FB7F.

### boxes

The `boxes` capability indicates that box characters are drawn by the terminal
itself, rather than through rendering the current font. This should cover at
least:

* The entirety of the [Box Drawing](https://www.unicode.org/charts/PDF/U2500.pdf)
   block (U+2500–U+257F).
* Symbols U+1FB70–U+1FB7F from the [Symbols for Legacy Computing](https://www.unicode.org/charts/PDF/U1FB00.pdf) block.
