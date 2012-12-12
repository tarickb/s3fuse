#!/bin/bash

if [ ! -e "$1" ] || [ -z "$2" ] || [ ! -d "$2" ]; then
  echo "Usage: $0 <dist-tarball> <rpm-repo-dir>"
  exit 1
fi

_DIST_TAR=$(pwd)/$1
_REPO_DIR=$2

if [ ! -e ./make-version.sh ]; then
  echo Please run me from the top-level source directory.
  exit 1
fi

source ./make-version.sh

cp $_DIST_TAR $_REPO_DIR/SOURCES || exit 1

pushd $_REPO_DIR >& /dev/null || exit 1

rpmbuild \
  --define "_topdir $(pwd)" \
  --define "version $VERSION" \
  --define "release 1" \
  -ta SOURCES/s3fuse-$VERSION.tar.gz || exit 1

popd >& /dev/null || exit 1
