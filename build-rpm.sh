#!/bin/bash

if [ -z "$1" ] || [ ! -d "$1" ]; then
  echo Usage: $0 RPM_REPO_DIR
  exit 1
fi

_REPO_DIR=$1

if [ ! -d dist ] || [ ! -d src ]; then
  echo Please run me from the top-level source directory.
  exit 2
fi

. version-info.sh

TEMP_DIR=$(mktemp -d)
SRC_DIR=$(pwd)

pushd $TEMP_DIR >& /dev/null || exit 1

mkdir s3fuse-$VERSION || exit 1
cd s3fuse-$VERSION || exit 1

cp -r $SRC_DIR/* . || exit 1
rm -rf build-deb.sh build-rpm.sh clean.sh configure.* debian dist/release.sh dist/*.spec* Makefile.* || exit 1
find . -type d -name .svn | xargs rm -rf || exit 1

cat $SRC_DIR/dist/s3fuse.spec.in \
  | sed -e "s/__VERSION__/$VERSION/g" -e "s/__RELEASE__/$RELEASE/g" \
  > dist/s3fuse.spec \
  || exit 1

cp $SRC_DIR/configure.ac_rpm configure.ac || exit 1
cp $SRC_DIR/Makefile.am_rpm Makefile.am || exit 1

cd .. || exit 1

tar cfz s3fuse-$VERSION-$RELEASE.tar.gz s3fuse-* || exit 1

popd >& /dev/null || exit 1

mv -t $_REPO_DIR/SOURCES $TEMP_DIR/s3fuse-*.tar.gz || exit 1

rm -rf $TEMP_DIR || exit 1

pushd $_REPO_DIR >& /dev/null || exit 1

rpmbuild -ta --define "_topdir $(pwd)" SOURCES/s3fuse-$VERSION-$RELEASE.tar.gz || exit 1

popd >& /dev/null || exit 1
