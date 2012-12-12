#!/bin/bash

if [ ! -e configure.ac.in ]; then
  echo please run me from the directory containing configure.ac.in
  exit 1
fi

source ./make-version.sh

CONF_UPDATE=`grep '^# version: ' configure.ac 2>/dev/null | sed -e 's/.*: //'`

if [ "$VERSION" != "$CONF_UPDATE" ]; then
  echo "# version: $VERSION" > configure.ac
  cat configure.ac.in | sed -e "s/__VERSION__/$VERSION/g" >> configure.ac
  autoreconf --force --install || exit 1
else
  echo version unchanged, skipping reconf
fi
