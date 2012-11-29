#!/bin/bash

for F in $(find . -type f ! -path \*.svn\* ! -name \*.am ! -name \*.inc | sort); do 
  MODF=${F##./}; 
  HT=$(head -n 2 $F | tail -n 1 | sed -e 's/^ \* //'); 
  
  [ "$HT" != "$MODF" ] && echo NO MATCH: $F; 
done
