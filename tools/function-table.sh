#!/bin/sh

set -e

# Extract a list of the public API, both shared object functions and those
# static inline functions in the public headers.

usage () { echo "usage: `basename $0` notcurses-dir" ; }

[ $# -eq 1 ] || { usage >&2 ; exit 1 ; }
NCDIR="$1"
[ -d "$NCDIR" ] || { usage >&2 ; exit 1 ; }

generate_lists () {
  grep -h ^API "$1"/include/notcurses/*.h | grep -v inline | sort
  grep -h -A1 ^API\ inline "$1"/include/notcurses/*.h | \
    sed -e '/^--$/d' -e 'N;s/\n/ /' -e 's/\(.*\){$/\1;/' | sort
}

generate_lists "$NCDIR" | sed -e 's/RESTRICT/restrict/g'
