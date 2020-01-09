* Verify version in CMakeLists.txt
* Update versions in man page headers
* Finalize Debian changelog with `dch -r`
* Update version in rust/Cargo.toml
* git commit -a -m v$VERSION
* Tag with `git tag -a v$VERSION -m "v$VERSION"`
* `git push && git push origin --tags`
* Draft new release at https://github.com/dankamongmen/notcurses/releases
  * Title is "v$VERSION—some quip"
  * That's an em dash (U+2014, UTF-8 e2 80 94), get it right
* Build new Debian package
  * git clean -d -f -x
  * `tar -cJf ../notcurses_$VERSION.orig.tar.xz --exclude=.git --exclude=debian -C.. notcurses-$VERSION`
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
  * `scp *.html qemfd.net:/var/www/notcurses/`
* Update Debian changelog with `dch -v $NEXTVERSION-1`
* Update `doc/FreeBSD-Makefile` version
* Update CMakeLists.txt with next version
