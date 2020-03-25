This document attempts to list user-visible changes and any major internal
rearrangements of Notcurse.

* 1.2.5 (not yet released)
** `ncvisual_render()` now returns the number of cells emitted on success, as
    opposed to 0. Failure still sees -1 returned.
** `ncvisual_render()` now interprets length parameters of -1 to mean "to the
   end along this axis", and no longer interprets 0 to mean this. 0 now means
   "a length of 0", resulting in a zero-area rendering.

* 1.2.4 2020-03-24
** Add ncmultiselector
** Add `ncdirect_cursor_enable()` and `ncdirect_cursor_disable()`.
