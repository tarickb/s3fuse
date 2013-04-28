#!/bin/bash

WITH_REV=1

if [ "$1" == "--no-rev" ]; then
  shift
  WITH_REV=0
fi

export WITH_REV

autoreconf --force --install || exit 1

if [ "$(uname)" == "Darwin" ]; then
  ./configure --enable-tests --enable-darwin $*
else
  ./configure --enable-tests $*
fi
