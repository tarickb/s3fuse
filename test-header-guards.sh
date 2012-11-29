#!/bin/bash

for F in $(find . -type f ! -path \*.svn\* -name \*.h | sort); do 
  MODF=`echo $F | sed -e 's/^\.\//S3_/' -e 's/\//_/g' -e 's/\./_/g' -e 's/\(.*\)/\U\1/'`; 
  
  if ! grep "#ifndef $MODF" $F >/dev/null || ! grep "#define $MODF" $F >/dev/null; then 
    echo FAILED: $F; 
  fi; 
done
