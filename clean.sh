#!/bin/bash

if [ ! -d src ] || [ ! -d dist ]; then
  echo Run me from the top-level directory.
  exit 1
fi

for D in src dist; do
  pushd $D >& /dev/null
    rm -rf .deps >& /dev/null
    make clean >& /dev/null
    rm -f Makefile Makefile.in
  popd >& /dev/null
done

rm -f *.m4
rm -rf autom4te.cache
rm -f config.*
rm -f configure configure.ac
rm -f depcomp
rm -f install-sh
rm -f Makefile Makefile.in Makefile.am
rm -f missing
rm -f *.tar.gz
