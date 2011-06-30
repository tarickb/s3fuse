#!/bin/bash

VERSION=`head -n 1 ChangeLog | sed -e 's/.*(//' -e 's/).*//'`

export VERSION

autoreconf --force --install || exit 1
./configure
