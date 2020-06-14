# Release procedure

## Precheck

* Review the testing checklist (doc/testing-checklist.md)
* clang-tidy check with something like:
  * `cmake "-DCMAKE_CXX_CLANG_TIDY=/usr/bin/clang-tidy-11;-checks=-*,clang-analyzer-*,modernize-*,performance-*" ..`
  * `scan-build cmake .. && scan-build make`

## Release

* Run tools/release.sh $OLDVERSION $VERSION
  * Finalize CHANGELOG.md
  * Bumps version numbers everywhere they need bumping
  * Commits changes, tags result with v$VERSION, pushes tag
  * Downloads new tarball and signs it
  * Upload signature to github
* Draft new release at https://github.com/dankamongmen/notcurses/releases
  * Title is "v$VERSION—some quip"
  * That's an em dash (U+2014, UTF-8 e2 80 94), get it right
* Upload new Rust crate with `cargo publish`
* Upload new Python pip with
  * `python3 setup.py sdist`
  * `twine upload dist/*`
* Generate and upload new HTML documentation via `make html`
  * `scp *.html ../doc/man/index.html qemfd.net:/var/www/notcurses/`
* Generate and upload new Doxygen documentation via `doxygen ../doc/Doxyfile`
  * `scp -r html qemfd.net:/var/www/notcurses/`

## Packaging

### Debian

* In gbp repository:
  * Update Debian changelog, if necessary: `dch -v $VERSION+dfsg.1-1`
  * Finalize Debian changelog with `dch -r`
  * Repack DFSG-safe tarball with uscan, upload to github
    * `uscan --repack --compression xz --force`
    * `gpg --sign --armor --detach-sign ../notcurses_$VERSION+dfsg.1.orig.tar.xz`
    * sign, upload dfsg+sig to github
    * import new version: `gbp import-orig ../notcurses_$VERSION+dfsg.1.orig.tar.xz`
    * `git push --tags`
    * build source package: `dpkg-buildpackage --build=source`
    * build binaries: `cd .. && export TERM=xterm-256color && sudo pbuilder build *dsc`
        * perform this in xterm with TERM=xterm-256color
        * beware: freak TERMs won't be present in pbuilder
* Copy `../*notcurses*$VERSION*` to apt repo, import with `reprepro`
* Update Debian changelog with `dch -v $NEXTVERSION-1`
* Update CMakeLists.txt with next version

### Fedora

* In pagure/notcurses master:
  * Update notcurses.spec, bump version, add changelog entry
  * clear out any old ersatz detritus
  * spectool -g notcurses.spec
  * fedpkg upload *.xz *.asc
  * fedpkg new-sources *.xz *.asc
  * fedpkg commit
  * git push
  * fedpkg build
  * fedpkg switch-branch f32
  * git merge master

### Arch

* Upload new AUR information
  * Update `pkgver` and `sha256sums` entries
  * `makepkg --printsrcinfo > .SRCINFO`
  * Test that package builds with `makepkg`
  * `git commit -a`

### FreeBSD

* Update svn checkout of Ports tree: `cd /usr/ports && svn up`
* Upgrade ports: `portupgrade -uap`
* `cd /usr/ports/devel/notcurses`
* Update `DISTVERSION` in `Makefile
* `sudo make makesum`
* `make stage`
* `make stage-qa`
* `portlint`
* `svn diff > ../`make -VPKGNAME`.diff
* File bug on devel/notcurses, attach diff
