#!/bin/sh

# requires https://pypi.org/project/githubrelease/

set -e

usage() { echo "usage: `basename $0` oldversion newversion quip" ; }

[ $# -eq 3 ] || { usage >&2 ; exit 1 ; }

OLDVERSION="$1"
VERSION="$2"
QUIP="$3"

# update the FFI exports
python tools/generate_ffi.py .

# finalize the date on the most recent version, add any last-minute notes?
vi NEWS.md

git clean -f -d -x

# Doing general context-free regexery has led several times to heartache. We
# thus do tightly-coupled, context-sensitive seds for each class of files.
# Please don't add version numbers where they're not necessary.

# quick sanity checks before and after version update
grep $OLDVERSION CMakeLists.txt > /dev/null || { echo "Couldn't find OLDVERSION ($OLDVERSION) in CMakeLists.txt" >&2 ; exit 1 ; }
sed -i -e "s/\(project(notcurses VERSION \)$OLDVERSION/\1$VERSION/" CMakeLists.txt
grep $VERSION CMakeLists.txt > /dev/null || { echo "Couldn't find VERSION ($VERSION) in CMakeLists.txt" >&2 ; exit 1 ; }

# FIXME we ought probably verify that there has been an actual change, as these
#       will surely otherwise go out of date.
sed -i -e "s/\(PROJECT_NUMBER *= \)$OLDVERSION/\1$VERSION/" doc/Doxyfile
for i in doc/man/man*/*.md cffi/notcurses-*.md cffi/ncdirect-*.md; do
  sed -i -e "s/% v$OLDVERSION/% v$VERSION/" "$i"
done
sed -i -e "s/v$OLDVERSION/v$VERSION/g" doc/man/index.html
sed -i -e "s/version=\"$OLDVERSION\"/version=\"$VERSION\"/" cffi/setup.py
sed -i -e "s/version=\"$OLDVERSION\"/version=\"$VERSION\"/" python/setup.py
sed -i -e "s/<version>$OLDVERSION</<version>$VERSION</" tools/nuspec
sed -i -e "s/Program version = $OLDVERSION/Program version = $VERSION/" tools/notcurses-installer.ifp
sed -i -e "s/File = \(.*\)notcurses-$OLDVERSION/File = \1notcurses-$VERSION/" tools/notcurses-installer.ifp

BUILDDIR="build-$VERSION"

# do a build with Doxygen enabled, upload docs, clean it up
mkdir "$BUILDDIR"
cd "$BUILDDIR"
cmake -DUSE_DOXYGEN=on ..
make -j
make test
ssh qemfd.net rm -rf /opt/notcurses/html
scp -r html qemfd.net:/opt/notcurses/html
scp *.html ../doc/man/index.html model.png qemfd.net:/opt/notcurses/
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
rm $TARBALL

echo "Cut $VERSION, signed to $TARBALL.asc"
echo "Now uploadling the sig to https://github.com/dankamongmen/notcurses/releases"
echo "The bastards are trying to immanentize the Eschaton"

# requires token in ~/.netrc
github-release dankamongmen/notcurses create v$VERSION --name "v$VERSION—$QUIP" --publish $TARBALL.asc
rm $TARBALL.asc

cd "$BUILDDIR"
sudo make install
# restrict to files beginning with n* to leave out shared objects
tar czvf notcurses-doc-$VERSION.tar.gz n*.1 t*.1 n*.3 *.html
github-asset dankamongmen/notcurses upload v$VERSION notcurses-doc-$VERSION.tar.gz
cd ../cffi
python3 setup.py sdist
python3 setup.py build
twine upload -s -udankamongmen dist/*

cd "../$BUILDDIR"
cat install_manifest.txt | sudo xargs rm
