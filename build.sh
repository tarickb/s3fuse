#!/bin/bash

autoreconf --force --install || exit 1

if [ "$(uname)" == "Darwin" ]; then
  ./configure --enable-tests --enable-darwin $*
else
  ./configure --enable-tests $*
fi
