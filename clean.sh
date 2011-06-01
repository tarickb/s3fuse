#!/bin/bash

if [ ! -d src ] || [ ! -d dist ]; then
  echo Run me from the top-level directory.
  exit 1
fi

for D in src dist; do
  pushd $D
    make clean
    rm Makefile Makefile.in
  popd
done

rm *.m4
rm -r autom4te.cache
rm config.*
rm configure
rm depcomp
rm install-sh
rm Makefile Makefile.in
rm missing
