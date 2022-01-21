#!/bin/sh

set -e

# export DISTRIBUTION to use something other than unstable (or whatever was
# last used in debian/changelog). see dch(1). can use a DEBVERSION exported
# in the process's environment.
usage() { echo "usage: `basename $0` version" ; }

[ $# -eq 1 ] || { usage >&2 ; exit 1 ; }

VERSION="$1"

if [ -z "$DEBVERSION" ] ; then
  DEBVERSION=1
fi

rm -fv debian/files
dch -v $VERSION+dfsg.1-$DEBVERSION
if [ -n "$DISTRIBUTION" ] ; then
  dch -r --distribution "$DISTRIBUTION"
else
  dch -r
fi
uscan --repack --compression xz --force
gpg --sign --armor --detach-sign ../notcurses_$VERSION+dfsg.1.orig.tar.xz
# FIXME this seems to upload to $VERSION.dfsg as opposed to $VERSION+dfsg?
github-asset dankamongmen/notcurses upload v$VERSION ../notcurses_$VERSION+dfsg.1.orig.tar.xz ../notcurses_$VERSION+dfsg.1.orig.tar.xz.asc
git commit -m "v$VERSION" -a

echo
echo "Go change the $VERSION.dfsg to $VERSION+dfsg before proceeding, bro"
echo

gbp import-orig ../notcurses_$VERSION+dfsg.1.orig.tar.xz
git push --tags
dpkg-buildpackage --build=source
cd ..
sudo pbuilder build notcurses*dsc
cd -
git push
rm debian/files
