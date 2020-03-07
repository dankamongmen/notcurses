* `for i in CMakeLists.txt doc/Doxyfile doc/FreeBSD-Makefile doc/man/man*/* doc/man/index.html python/setup.py rust/*/Cargo.toml rust/libnotcurses-sys/build.rs ; do sed -i -e "s/$OLDVERSION/$VERSION/g" $i ; done`
* Finalize Debian changelog with `dch -r`
* git commit -a -m v$VERSION
* Verify that Debian package builds properly
  * git clean -d -f -x
  * `tar -cJf ../notcurses_$VERSION.orig.tar.xz --exclude=.git --exclude=debian -C.. notcurses-$VERSION`
  * debuild
* Tag with `git tag -a v$VERSION -m "v$VERSION -s"`
* `git push && git push origin --tags`
* Draft new release at https://github.com/dankamongmen/notcurses/releases
  * Title is "v$VERSION—some quip"
  * That's an em dash (U+2014, UTF-8 e2 80 94), get it right
* Repack DFSG-safe tarball, upload to github
  * download github-spun tarball
  * remove nonfree multimedia:
    * rm data/chun* data/[adeflmPw]* src/demo/jungle.c
  * `tar -cJf ../v$VERSION.dfsg.tar.xz -C.. notcurses-$VERSION`
  * upload to github
* Build new Debian package
  * download DFSG tarball, unpack
  * debuild
* Copy `../*notcurses*$VERSION*` to apt repo, import with `reprepro`
* Upload new AUR information
  * Update `pkgver` and `sha256sums` entries
  * `makepkg --printsrcinfo > .SRCINFO`
  * Test that package builds with `makepkg`
  * `git commit -a`
* Upload new Rust crate with `cargo publish`
* Upload new Python pip with
  * `python3 setup.py sdist`
  * `twine upload dist/*`
* Generate and upload new HTML documentation via `make html`
  * `scp *.html ../doc/man/index.html qemfd.net:/var/www/notcurses/`
* Generate and upload new Doxygen documentation via `doxygen ../doc/Doxyfile`
  * `scp -r html qemfd.net:/var/www/notcurses/`
* Update Debian changelog with `dch -v $NEXTVERSION-1`
* Update `doc/FreeBSD-Makefile` version
* Update CMakeLists.txt with next version
