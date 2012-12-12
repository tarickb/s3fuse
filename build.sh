#!/bin/bash

./make-autoconf.sh

if [ "$(uname)" == "Darwin" ]; then
  ./configure --enable-tests --enable-darwin
else
  ./configure --enable-tests
fi

make
