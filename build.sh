#!/bin/bash

ln -s configure.ac_deb configure.ac
ln -s Makefile.am_deb Makefile.am

autoreconf --force --install || exit 1
./configure
