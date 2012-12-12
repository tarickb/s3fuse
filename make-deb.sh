#!/bin/bash

if [ ! -e "$1" ] || [ -z "$2" ] || [ ! -d "$2" ]; then
  echo "Usage: $0 <dist-tarball> <package-output-location>"
  exit 1
fi

_DIST_TAR=$(pwd)/$1
_DEB_LOCATION=$2

if [ ! -e ./make-version.sh ]; then
  echo Please run me from the top-level source directory.
  exit 1
fi

source ./make-version.sh

TEMP_DIR=$(mktemp -d)

pushd $TEMP_DIR >& /dev/null || exit 1
tar xfz $_DIST_TAR || exit 1
cd s3fuse-$VERSION/debian || exit 1
debuild -us -uc || echo "Might have failed to build package, but continuing anyway."
cd ../.. || exit 1
rm -rf s3fuse-$VERSION
popd >& /dev/null || exit 1

cp -f $TEMP_DIR/* $_DEB_LOCATION || echo "Failed to copy package." && exit 1

rm -r $TEMP_DIR
