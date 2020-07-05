#!/bin/sh

set -e

# Used to generate C constants from a Unicode Consortium
# emoji-ordering-rules.txt data file. This is done to build
# the mojibake demo C file.

usage() { echo "usage: `basename $0` < emoji-ordering-rules.txt" ; }

[ "$#" -eq 0 ] || { usage >&2 ; exit 1 ; }

while read line ; do unicode --brief --max 100 $line | cut -d\  -f2 | \
   sed -e 's/U+\(.....\)/\\U000\1/' | \
   sed -e 's/U+\(....\)/\\u\1/' | \
   sed -e ':a; /u200D$/N; s/u200D\n/u200D/; ta' | \
   sed -e ':a; $!N;s/\n\\u200D/\\u200D/;ta;P;D' | \
   sed -e 's/\(.*\)/"\1"/'
done
