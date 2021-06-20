# Contributing to Notcurses

Your quality contributions are cheerfully appreciated, however small. Example
code and documentation are especially welcome.

Changes ought reflect the style of the surrounding matter, whether code or
documentation. The styles used in the C core, the C++ wrappers, and the Rust
wrappers are quite distinct; use the appropriate style for the language.

New features should have unit tests. It is appreciated if bugfixes have
unit tests, as well. Wrappers in a new language absolutely must have at
least some superficial tests (it is not necessary to deeply test the
underlying functionality in each language). Adding a wrapper implies that
you're prepared to maintain that wrapper; if you can't maintain it, the wrapper
will likely be removed.

Escape sequences available from terminfo must not be hard-coded. Routines must
check to ensure the relevant escape sequence is valid for the current TERM
definition, and not emit it if invalid. Routines emitting characters beyond
the 128 elements of ASCII should check for UTF8 availability, and fall back to
an ASCII equivalent if not present (or return an error).

Run `make test` with your changes, and ensure all tests pass. Run
`notcurses-demo` as well, if your changes affect the core library (or the
demo code).

## C standard
Notcurses targets the ISO C11 standard. This means you should avoid using
GNU C extensions as they might not work outside GCC/Clang. To verify your
standard compliance on GCC and Clang you can compile with `-std=c11 -pedantic`
command line arguments.
