#!/bin/bash

_CONF=$1

if [ ! -f "$_CONF" ]; then
  echo "Cannot find configuration file [$_CONF]."
  exit 1
fi

echo '# s3fuse.conf'
echo '# configuration file for s3fuse(1)'
echo ''

grep "^\<CONFIG_REQUIRED\>(.*);$" $_CONF | \
  sed -e 's/^CONFIG_REQUIRED(//' -e 's/);$//' -e 's/, /%%%%/g' -e 's/"//g' | \
  awk -F '%%%%' '{ print "# name: " $2 ", type: " $1 " [REQUIRED]\n# description: " $4" \n" $2 "=\n" }'

grep "^\<CONFIG\>(.*);$" $_CONF | \
  sed -e 's/^CONFIG(//' -e 's/);$//' -e 's/, /%%%%/g' -e 's/"//g' | \
  awk -F '%%%%' '{ print "# name: " $2 ", type: " $1 "\n# description: " $4" \n# default: " $3 "\n#" $2 "=\n" }'
