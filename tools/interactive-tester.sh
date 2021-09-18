#!/bin/sh

set -e

# interactive tester for Notcurses. runs a variety of programs, which the
# user must inspect for correct output -- i.e. these do not check their
# own output for correctness =[. it ought be run from a notcurses build
# directory, with all binaries built.

DATA=../data # FIXME
OUT=$(basename 0).log # FIXME
rm -f "$OUT"

[ -d "$DATA" ] || { echo "$DATA was not a directory" >&2 ; exit 1 ; }

# this ought process the entire file, then allow the user to exit with ^D
./notcurses-input -v < ../src/lib/in.c 2>>"$OUT"

# goes through a series of self-described bitmap frames
./bitmapstates 2>>"$OUT"

./statepixel "$DATA"/worldmap.jpg 2>>"$OUT"

./ncneofetch -v 2>>"$OUT"

./resize
