#!/bin/bash

if [ ! -d src ]; then
  echo Run me from the top-level directory.
  exit 1
fi

for D in \
  man \
  src \
  src/base \
  src/base/tests \
  src/crypto \
  src/crypto/tests \
  src/fs \
  src/fs/tests \
  src/services \
  src/services/aws \
  src/services/iijgio \
  src/services/gs \
  src/tests \
  src/threads \
  src/threads/tests
do
  pushd $D >& /dev/null
    make clean distclean >& /dev/null
    rm -rf .deps >& /dev/null
    rm -f Makefile Makefile.in
    rm -f *.conf
    rm -f *.gcda *.gcno *.gcov
  popd >& /dev/null
done

rm -rf autom4te.cache
rm -rf coverage-report 

find . -maxdepth 1 -type f -name \*.m4 ! -name boost.m4 | xargs rm -f

rm -f compile
rm -f config.*
rm -f configure
rm -f depcomp
rm -f install-sh
rm -f libtool
rm -f ltmain.sh
rm -f Makefile Makefile.in
rm -f missing
rm -f test-driver
rm -f *.tar.gz
rm -f *.info
