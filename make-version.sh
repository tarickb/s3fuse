#!/bin/bash

if [ ! -e ChangeLog ]; then
  echo please run me from the directory containing ChangeLog
  exit 1
fi

VERSION=`head -n 1 ChangeLog | sed -e 's/.*(//' -e 's/).*//'`
