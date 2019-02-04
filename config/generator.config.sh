#!/bin/bash

readonly _CONFIG="$1"
readonly _TEMPLATE="$2"
readonly _OUTPUT="$3"

sed \
  -e "/__CONFIG_INC_GOES_HERE__/ r $_CONFIG" \
  -e "/__CONFIG_INC_GOES_HERE__/ d" \
  < "$_TEMPLATE" \
  > "$_OUTPUT"
