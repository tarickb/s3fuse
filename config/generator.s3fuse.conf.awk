BEGIN {
  print("# " PACKAGE_NAME ".conf");
  print("# configuration file for " PACKAGE_NAME "(1) " PACKAGE_VERSION);
  print("");
}

function underline(n)
{
  printf "# ";

  for (i = 0; i < n; i++)
    printf "#";
  
  printf "\n\n";
}

{
  if ($1 == "SECTION") {
    print("# " toupper($2));
    underline(length($2));
  } else if ($1 == "PARAM") {
    gsub(/__PACKAGE_NAME__/, PACKAGE_NAME, $5)
    print("# name: " $3 ", type: " $2 "\n# description: " $5 "\n# default: " $4 " \n#" $3 "=\n");
  }
}
