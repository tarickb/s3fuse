#!/bin/bash

for F in $(find src -type f ! -path \*.svn\* -name \*.h | sort); do 
  MODF=`echo $F | sed -e 's/^src\//S3_/' -e 's/\//_/g' -e 's/\./_/g' | tr 'a-z' 'A-Z'`;

  if ! grep "#ifndef $MODF" $F >/dev/null || ! grep "#define $MODF" $F >/dev/null; then 
    echo FAILED: $F; 
  fi; 
done
