#!/bin/bash

_IN="$1"
_OUT="$2"

if [ ! -f "$_IN" ]; then
  echo cannot find input file $_IN
  exit 1
fi

if [ -f "$_OUT" ]; then
  echo output file $_OUT already exists.
  exit 2
fi

cat $_IN \
  | grep "^\<CONFIG_REQUIRED\>" \
  | sed -e 's/CONFIG_REQUIRED(//' -e 's/)//' -e 's/, /%%%%/g' -e 's/; \/\/ /%%%%/' -e 's/;//' \
  | awk -F %%%% '{ print "# name: " $2 ", type: " $1 " [REQUIRED]\n# description: " $4" \n" $2 "=\n" }' \
  > $_OUT

cat $_IN \
  | grep "^\<CONFIG\>" \
  | sed -e 's/CONFIG(//' -e 's/)//' -e 's/, /%%%%/g' -e 's/; \/\/ /%%%%/' -e 's/;//' \
  | awk -F %%%% '{ print "# name: " $2 ", type: " $1 "\n# description: " $4" \n# default: " $3 "\n#" $2 "=\n" }' \
  >> $_OUT

