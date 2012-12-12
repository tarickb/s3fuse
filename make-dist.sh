#!/bin/bash

./make-autoconf.sh || exit 1
./configure || exit 1
make dist
