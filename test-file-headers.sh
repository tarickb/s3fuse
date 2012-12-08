#!/bin/bash

for F in $(find src -type f ! -path \*.svn\* ! -path \*/tests/\* ! -name \*.am ! -name \*.inc | sort); do 
  MODF=${F##src/}; 
  HT=$(head -n 2 $F | tail -n 1 | sed -e 's/^ \* //'); 
  
  [ "$HT" != "$MODF" ] && echo NO MATCH: $F; 
done
