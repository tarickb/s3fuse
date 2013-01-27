#!/bin/bash

_CONF=$1

if [ ! -f "$_CONF" ]; then
  echo "Cannot find configuration file [$_CONF]."
  exit 1
fi

echo '####################################'
echo '# s3fuse.conf                      #'
echo '# configuration file for s3fuse(1) #'
echo '####################################'
echo ''

grep '^\(CONFIG\|CONFIG_REQUIRED\|CONFIG_SECTION\)(.*);$' $_CONF | \
  sed -e 's/(/%%%%/' -e 's/);$//' -e 's/, /%%%%/g' -e 's/"//g' | \
  awk -F '%%%%' '
    function underline(n)
    {
      printf "# ";

      for (i = 0; i < n; i++)
        printf "#";
      
      printf "\n\n";
    }
    
    {
      if ($1 == "CONFIG_SECTION") {
        print("# " toupper($2));
        underline(length($2));
      } else if ($1 == "CONFIG") {
        print("# name: " $3 ", type: " $2 "\n# description: " $5 "\n#" $3 "=\n");
      } else if ($1 == "CONFIG_REQUIRED") {
        print("# name: " $3 ", type: " $2 " [REQUIRED]\n# description: " $5 "\n" $3 "=\n");
      }
    }'
