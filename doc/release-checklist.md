* Run tools/release.sh $OLDVERSION $VERSION
  * Finalize CHANGELOG.md
  * Bumps version numbers everywhere they need bumping
  * Commits changes, tags result with v$VERSION, pushes tag
  * Downloads new tarball and signs it
  * Upload signature to github
* Draft new release at https://github.com/dankamongmen/notcurses/releases
  * Title is "v$VERSION—some quip"
  * That's an em dash (U+2014, UTF-8 e2 80 94), get it right
* Finalize Debian changelog with `dch -r`
* Repack DFSG-safe tarball with uscan, upload to github
  * `uscan --repack --compression xz -v`
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
