# Release procedure

## Precheck

* Review the testing checklist (doc/testing-checklist.md)
* clang-tidy check with something like:
  * `cmake "-DCMAKE_CXX_CLANG_TIDY=/usr/bin/clang-tidy-11;-checks=-*,clang-analyzer-*,modernize-*,performance-*" ..`
  * `scan-build cmake .. && scan-build make`
* Verify that rust + python compile

## Release

* In toplevel, run `tools/release.sh $OLDVERSION $VERSION "quip"`:
  * Cleans repo with `git clean -f -d -x`
  * Opens an editor to finalize NEWS.md
  * Bumps version numbers everywhere they need bumping
  * Commits changes, tags result with v$VERSION, pushes tag
  * Downloads new tarball and signs it
  * Uploads signature to github
  * Uploads new Rust crates with `cargo publish`
  * Uploads new Python pip with
    * `python3 setup.py sdist`
    * `twine upload dist/*`
  * Generates and uploads new HTML documentation via `make html`
    * `scp *.html ../doc/man/index.html qemfd.net:/var/www/notcurses/`
    * `scp -r html qemfd.net:/var/www/notcurses/`
  * Publishes new release at https://github.com/dankamongmen/notcurses/releases
    * Title is "v$VERSION—some quip"

## Packaging

### Debian

* In gbp repository, run `tools/debrelease.sh $VERSION`:
  * Updates Debian changelog with `dch -v $VERSION+dfsg.1-1`
  * Finalizes Debian changelog with `dch -r`
  * Repacks DFSG-safe tarball with `uscan`:
    * `uscan --repack --compression xz --force`
    * `gpg --sign --armor --detach-sign ../notcurses_$VERSION+dfsg.1.orig.tar.xz`
  * Uploads repack + signature to github
  * imports new version: `gbp import-orig ../notcurses_$VERSION+dfsg.1.orig.tar.xz`
  * `git push --tags`
  * builds source package: `dpkg-buildpackage --build=source`
  * builds binaries: `cd .. && export TERM=xterm-256color && sudo pbuilder build *dsc`
    * performs this in xterm with TERM=xterm-256color
    * beware: freak TERMs won't be present in pbuilder

### Fedora

* In pagure/notcurses rawhide:
  * Update notcurses.spec, bump version, add changelog entry
  * clear out any old ersatz detritus
  * spectool -g notcurses.spec
  * fedpkg upload *.xz *.asc
  * fedpkg new-sources *.xz *.asc
  * fedpkg commit
  * git push
  * fedpkg build
  * for each active branch:
    * fedpkg switch-branch f32
    * git merge rawhide

### Arch

* Upload new AUR information
  * Update `pkgver` and `sha256sums` entries
  * `makepkg --printsrcinfo > .SRCINFO`
  * Test that package builds with `makepkg`
  * `git commit -a`

### Alpine

* Update version in `APKBUILD`
  * Run `abuild checksum` to get new checksums
  * `abuild -r` to test `APKBUILD`
  * create merge request

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
