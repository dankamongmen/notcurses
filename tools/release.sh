#!/bin/sh

set -e

usage() { echo "usage: `basename $0` oldversion newversion quip" ; }

[ $# -eq 3 ] || { usage >&2 ; exit 1 ; }

OLDVERSION="$1"
VERSION="$2"
QUIP="$3"

vi NEWS.md

git clean -f -d -x

# bump version numbers wherever they occur (wherever we enumerate them, anyway)
BUMP="CMakeLists.txt doc/Doxyfile doc/man/man*/* doc/man/index.html python/setup.py rust/*/Cargo.toml rust/libnotcurses-sys/build.rs"
for i in $BUMP ; do
  sed -i -e "s/$OLDVERSION/$VERSION/g" $i
done

# do a build with Doxygen enabled, upload docs, clean it up
mkdir build
cd build
cmake -DUSE_DOXYGEN=on ..
make -j
make test
scp html/* qemfd.net:/var/www/notcurses/html/
scp *.html ../doc/man/index.html qemfd.net:/var/www/notcurses/
cd ..
rm -rf build

# if that all worked, commit, push, and tag
git commit -a -m v$VERSION
git push
git pull
git tag -a v$VERSION -m v$VERSION -s
git push origin --tags
git pull
TARBALL=v$VERSION.tar.gz
wget https://github.com/dankamongmen/notcurses/archive/$TARBALL
gpg --sign --armor --detach-sign $TARBALL
rm v$VERSION.tar.gz

echo "Cut $VERSION, signed to $TARBALL.asc"
echo "Now upload the sig to https://github.com/dankamongmen/notcurses/releases"
echo "The bastards are trying to immanentize the Eschaton"

# requires token in ~/.netrc
github-release dankamongmen/notcurses create --name "v$VERSION—$QUIP" --publish
github-asset dankamongmen/notcurses upload v$VERSION $TARBALL.asc

cd ../rust/libnotcurses-sys
cargo clean
cargo publish
cd ../notcurses
cargo clean
cargo publish

cd ../../python
python3 setup.py sdist
twine upload dist/*
