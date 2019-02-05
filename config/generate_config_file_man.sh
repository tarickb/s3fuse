#!/bin/bash

readonly _CPP="$1"
readonly _CPPFLAGS="$2"
readonly _CONFIG="$3"
readonly _PACKAGE_NAME="$4"
readonly _PACKAGE_VERSION="$5"
readonly _SYSCONFDIR="$6"
readonly _AWK="$7"
readonly _OUTPUT="$8"

readonly _today="$(date "+%Y-%m-%d")"

"${_CPP}" ${_CPPFLAGS} -E -x c "$_CONFIG" | \
  sed \
    -e '/^# / d' \
    -e '/^  *$/ d' \
    -e 's/^  *//' \
    -e 's/;$//' \
    -e '/CONFIG_CONSTRAINT/ d' \
    -e '/CONFIG_KEY/ d' \
    -e 's/CONFIG_SECTION(\(.*\))/SECTION%%%%\1/' \
    -e 's/CONFIG(\(.*\), \(.*\), \(.*\),\(.*\))/PARAM%%%%\1%%%%\2%%%%\3%%%%\4/' \
    -e 's/"//g' | \
    awk \
      -v "PACKAGE_NAME=${_PACKAGE_NAME}" \
      -v "PACKAGE_VERSION=${_PACKAGE_VERSION}" \
      -v "SYSCONFDIR=${_SYSCONFDIR}" \
      -v "TODAY=${_today}" \
      -F '%%%%' \
      -f "${_AWK}" \
      > ${_OUTPUT}
