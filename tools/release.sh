#!/bin/sh

set -e

usage() { echo "usage: `basename $0` oldversion newversion" ; }

[ $# -eq 2 ] || { usage >&2 ; exit 1 ; }

OLDVERSION="$1"
VERSION="$2"

vi CHANGELOG.md

git clean -f -d -x
BUMP="CMakeLists.txt doc/Doxyfile doc/FreeBSD-Makefile doc/man/man*/* doc/man/index.html python/setup.py rust/*/Cargo.toml rust/libnotcurses-sys/build.rs"
for i in $BUMP ; do
  sed -i -e "s/$OLDVERSION/$VERSION/g" $i
done
echo "Checking for instances of $OLDVERSION..."
grep -rF "$OLDVERSION" *
git commit -a -m v$VERSION
git push
git pull
git tag -a v$VERSION -m "v$VERSION -s"
git push origin --tags
git pull
wget https://github.com/dankamongmen/notcurses/archive/v$VERSION.tar.gz
gpg --sign --armor --detach-sign v$VERSION.tar.gz 

echo "Cut $VERSION. Now upload the GPG signature to https://github.com/dankamongmen/notcurses/releases"
