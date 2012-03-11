#!/bin/bash

if [ ! -d src ] || [ ! -d dist ]; then
  echo Run me from the top-level directory.
  exit 1
fi

for D in src dist; do
  pushd $D >& /dev/null
    make clean distclean >& /dev/null
    rm -rf .deps >& /dev/null
    rm -f Makefile Makefile.in
    rm -f s3fuse.conf
  popd >& /dev/null
done

rm -f *.m4
rm -rf autom4te.cache
rm -f config.*
rm -f configure
rm -f configure.ac
rm -f depcomp
rm -f install-sh
rm -f Makefile Makefile.in
rm -f missing
rm -f *.tar.gz
