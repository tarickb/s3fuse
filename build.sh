#!/bin/bash

. version-info.sh
export VERSION RELEASE

autoreconf --force --install || exit 1
./configure
