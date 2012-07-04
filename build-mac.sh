#!/bin/bash

VERSION=`head -n 1 ChangeLog | sed -e 's/.*(//' -e 's/).*//'`

cat configure.ac.in | sed -e "s/__VERSION__/$VERSION/g" > configure.ac
autoreconf --force --install || exit 1
./configure --enable-darwin
