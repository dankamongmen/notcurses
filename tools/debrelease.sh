#!/bin/sh

set -e

# export DISTRIBUTION to use something other than unstable (or whatever was
# last used in debian/changelog). see dch(1). can use a DEBVERSION exported
# in the process's environment.
usage() { echo "usage: `basename $0` version notcursessrcdir" ; }

[ $# -eq 2 ] || { usage >&2 ; exit 1 ; }

VERSION="$1"
SRCDIR="$2"

if [ -z "$DEBVERSION" ] ; then
  DEBVERSION=1
fi

rm -fv debian/files
dch -v $VERSION+dfsg-$DEBVERSION
if [ -n "$DISTRIBUTION" ] ; then
  dch -r --distribution "$DISTRIBUTION"
else
  dch -r
fi
uscan --repack --compression xz --force
XBALL=../notcurses_$VERSION+dfsg.orig.tar.xz
gpg --sign --armor --detach-sign $XBALL
ASC=$(readlink -f $XBALL.asc)
XBALL=$(readlink -f $XBALL) # eliminates ../
cd "$SRCDIR"
gh release upload v$VERSION $ASC $XBALL
cd -
git commit -m "v$VERSION" -a

gbp import-orig --upstream-version=$VERSION ../notcurses_$VERSION+dfsg.orig.tar.xz
git push --tags
dpkg-buildpackage --build=source
cd ..
# this isn't perfect -- we might not be running xterm -- but it keeps us
# from dying when such a terminal doesn't have its terminfo inside pbuilder
sudo TERM=xterm pbuilder build notcurses*dsc
cd -
git push
rm debian/files
