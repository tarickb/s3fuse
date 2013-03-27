#!/bin/bash

if [ "$1" == "--with-rev" ]; then
  shift
  export WITH_REV=1
fi

autoreconf --force --install || exit 1

if [ "$(uname)" == "Darwin" ]; then
  ./configure --enable-tests --enable-darwin $*
else
  ./configure --enable-tests $*
fi
