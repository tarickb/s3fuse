BEGIN {
  print(".\\\" man page for " PACKAGE_NAME);
  print(".\\\" generated by " PACKAGE_NAME ".conf.5.awk");

  print(".TH " toupper(PACKAGE_NAME) ".CONF 5 " TODAY " \"" PACKAGE_NAME " " PACKAGE_VERSION "\" \"" PACKAGE_NAME " Configuration File\"");
  print("");
  print(".SH NAME");
  print("\\fB" PACKAGE_NAME ".conf\\fR - Configuration file for " PACKAGE_NAME "");
  print("");
  print(".SH FILES");
  print("\\fB" PACKAGE_NAME "\\fR(1) loads a configuration file on initialization. It tries");
  print("the following paths, in order:");
  print(".IP \\[bu] 2");
  print(".B \"~/." PACKAGE_NAME "/" PACKAGE_NAME ".conf\"");
  print("");
  print(".IP \\[bu]");
  print(".B \"" SYSCONFDIR "/" PACKAGE_NAME ".conf\"");
  print("");
  print(".SH SYNTAX");
  print("The configuration file contains a list of parameters, one per line, in");
  print("the format \\fIparameter_name\\fR=\\fIparameter_value\\fR. Lines beginning");
  print("with a # are ignored.");
}

{
  if ($1 == "SECTION") {
    print "\n.SH " toupper($2)
  } else if ($1 == "PARAM") {
    gsub(/__PACKAGE_NAME__/, PACKAGE_NAME, $5)
    print ".TP\n.B " $3 "\n" $5
  }
}

END {
  print(".SH SEE ALSO");
  print("\\fB" PACKAGE_NAME "\\fR(1), \\fB" PACKAGE_NAME "_gs_get_token\\fR(1), \\fB" PACKAGE_NAME "_purge_versions\\fR(1), \\fB" PACKAGE_NAME "_vol_key\\fR(1)");
}
