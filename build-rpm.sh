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

VERSION=`head -n 1 ChangeLog | sed -e 's/.*(//' -e 's/).*//'`
RELEASE=1
TEMP_DIR=$(mktemp -d)
SRC_DIR=$(pwd)

export VERSION RELEASE

pushd $TEMP_DIR >& /dev/null || exit 1

mkdir s3fuse-$VERSION || exit 1
cd s3fuse-$VERSION || exit 1

cp -r $SRC_DIR/* . || exit 1
./clean.sh || exit 1
rm -f *.sh || exit 1
find . -type d -name .svn | xargs rm -rf || exit 1
cat configure.ac.in | sed -e "s/__VERSION__/$VERSION/g" > configure.ac
rm -f configure.ac.in
cp $SRC_DIR/build-config.sh . || exit 1

cd .. || exit 1

tar cfz s3fuse-$VERSION-$RELEASE.tar.gz s3fuse-* || exit 1

popd >& /dev/null || exit 1

mv -t $_REPO_DIR/SOURCES $TEMP_DIR/s3fuse-*.tar.gz || exit 1

rm -rf $TEMP_DIR || exit 1

pushd $_REPO_DIR >& /dev/null || exit 1

rpmbuild \
  --define "_topdir $(pwd)" \
  --define "version $VERSION" \
  --define "release $RELEASE" \
  -ta SOURCES/s3fuse-$VERSION-$RELEASE.tar.gz || exit 1

popd >& /dev/null || exit 1
