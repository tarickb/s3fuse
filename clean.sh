#!/bin/bash

if [ ! -d src ] || [ ! -d dist ]; then
  echo Run me from the top-level directory.
  exit 1
fi


for D in \
  src \
  src/base \
  src/base/tests \
  src/crypto \
  src/crypto/tests \
  src/fs \
  src/fs/tests \
  src/services \
  src/tests \
  src/threads \
  debian \
  debian/source \
  dist; \
do
  pushd $D >& /dev/null
    make clean distclean >& /dev/null
    rm -rf .deps >& /dev/null
    rm -f Makefile Makefile.in
    rm -f s3fuse.conf
  popd >& /dev/null
done

find . -maxdepth 1 -type f -name \*.m4 ! -name boost.m4 | xargs rm -f
rm -rf autom4te.cache
rm -f config.*
rm -f configure
rm -f configure.ac
rm -f depcomp
rm -f install-sh
rm -f Makefile Makefile.in
rm -f missing
rm -f *.tar.gz
rm -f libtool
rm -f ltmain.sh
