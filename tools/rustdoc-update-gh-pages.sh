#!/usr/bin/env bash
#
# SCRIPT TO BUILD THE RUST DOCS FROM THE GITHUB REPO
#
# Make sure you have the notcurses dependencies installed,
# and your github credentials in order, so that you can push.
#
# This script:
# - clones the full notcurses repo inside $WORKING_DIR
# - compiles & installs notcurses inside $WORKING_DIR
# - compiles the rust documentation inside $WORKING_DIR
# - commits and pushes the new docs to the $DOCS_BRANCH branch
# - deletes $WORKING_DIR
#
# WARNING: This script:
# - Does NOT fetch the latest tag, but the master branch.
# - Does NOT check first the version in $DOCS_BRANCH, but commits always.
# 
# This script is intended to be used just once per release,
# and immediately after the release is published.
#
# The decision to clone and compile everything from scratch has been made
# in order to not risk interfering with the actual working repository, since
# switching between branches can be very messy in certain situations.
# 
set -e


WORKING_DIR=rustdoc-build
DOCS_BRANCH="gh-pages"


# 1. CLONE REPO & COMPILE NOTCURSES
#
# - contain everything in a subfolder

git clone git@github.com:dankamongmen/notcurses.git $WORKING_DIR
if [ $? -ne 0 ]; then
	echo "ERROR cloning notcurses repo via SSH protocol."
	exit
fi

cd $WORKING_DIR
rm -rf build
mkdir -p build
cd build

cmake -DCMAKE_INSTALL_PREFIX=./usr/ ..
if [ $? -ne 0 ]; then
	echo "ERROR configuring notcurses with cmake."
	exit
fi

make -j`nproc`
if [ $? -ne 0 ]; then
	echo "ERROR compiling notcurses."
	exit
fi

make install
if [ $? -ne 0 ]; then
	echo "ERROR installing notcurses."
	exit
fi

export PKG_CONFIG_PATH=`pwd`/usr/lib/pkgconfig


# 2. BUILD THE RUST DOCS
#
# 

cd ../rust
unset CARGO_TARGET_DIR

cargo doc --no-deps
if [ $? -ne 0 ]; then
	echo "ERROR building rust docs."
	exit
fi

VERSION=`grep ^version Cargo.toml | cut -d'"' -f2`


# 3. UPDATE REPO
#
#

git checkout $DOCS_BRANCH
if [ $? -ne 0 ]; then
	echo "ERROR switching to $DOCS_BRANCH branch."
	exit
fi

cd ..

rm -rf rustdoc
mv rust/target/doc rustdoc

git add rustdoc
git commit -m "update rust docs to $VERSION"
if [ $? -ne 0 ]; then
	echo "ERROR commiting version $VERSION."
	exit
fi

git push
if [ $? -ne 0 ]; then
	echo "ERROR pushing version $VERSION to $DOCS_BRANCH branch."
	exit
fi

cd ..
rm -rf $WORKING_DIR

echo "Done!"
