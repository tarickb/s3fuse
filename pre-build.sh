#!/bin/bash

./make-autoconf.sh || exit 1

if [ "$(uname)" == "Darwin" ]; then
  ./configure --enable-tests --enable-darwin $*
else
  ./configure --enable-tests $*
fi
