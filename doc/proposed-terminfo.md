# Some desirable terminfo capabilities

## Semigraphics

### blocks

The `blocks` capability indicates that block characters are drawn by the
terminal itself, rather than through rendering the current font. This should
cover at least:

* The entirety of the [Block Elements](https://www.unicode.org/charts/PDF/U2580.pdf) Unicode block (U+2580--U+259F).
* The entirety of the [Geometric Shapes](https://www.unicode.org/charts/PDF/U25A0.pdf) Unicode block (U+25A0--U+25FF).
* The [Symbols for Legacy Computing](https://www.unicode.org/charts/PDF/U1FB00.pdf) Unicode block from U+1FB00 through U+1FBAF,
   except U+1FB70--U+1FB7F.

### boxes
