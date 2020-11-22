#!/bin/sh

# requires https://pypi.org/project/githubrelease/

set -e

usage() { echo "usage: `basename $0` oldversion newversion quip" ; }

[ $# -eq 3 ] || { usage >&2 ; exit 1 ; }

OLDVERSION="$1"
VERSION="$2"
QUIP="$3"

vi NEWS.md

git clean -f -d -x

# bump version numbers wherever they occur (wherever we enumerate them, anyway)
BUMP="CMakeLists.txt doc/Doxyfile doc/man/man*/* doc/man/index.html python/setup.py python/notcurses-pydemo.1.md rust/Cargo.toml rust/build/build.rs"
for i in $BUMP ; do
  sed -i -e "s/$OLDVERSION/$VERSION/g" $i
done

BUILDDIR="build-$VERSION"

# do a build with Doxygen enabled, upload docs, clean it up
mkdir "$BUILDDIR"
cd "$BUILDDIR"
cmake -DUSE_DOXYGEN=on ..
make -j
make test
ssh qemfd.net rm -rf /opt/notcurses/html
scp -r html qemfd.net:/opt/notcurses/html
scp *.html ../doc/man/index.html qemfd.net:/opt/notcurses/
cd ..

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
echo "Now uploadling the sig to https://github.com/dankamongmen/notcurses/releases"
echo "The bastards are trying to immanentize the Eschaton"

# requires token in ~/.netrc
github-release dankamongmen/notcurses create v$VERSION --name "v$VERSION—$QUIP" --publish $TARBALL.asc
rm $TARBALL.asc

cd "$BUILDDIR"
sudo make install
tar czvf notcurses-doc-$VERSION.tar.gz *.1 *.3 *.html
github-asset dankamongmen/notcurses upload v$VERSION notcurses-doc-$VERSION.tar.gz
cd ../python
python3 setup.py sdist
python3 setup.py build
twine upload -s -udankamongmen dist/*
cd ../rust
cargo clean
cargo publish
cd "../$BUILDDIR"
cat install_manifest.txt | sudo xargs rm
