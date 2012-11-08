#include <termios.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include "crypto/passwords.h"

using std::cin;
using std::cout;
using std::endl;
using std::string;

using s3::crypto::passwords;

string passwords::read_from_stdin(const string &prompt)
{
  string line;
  termios term_flags, term_flags_orig;

  tcgetattr(STDIN_FILENO, &term_flags_orig);
  memcpy(&term_flags, &term_flags_orig, sizeof(term_flags));

  cout << prompt;

  term_flags.c_lflag &= ~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &term_flags);

  try {
    getline(cin, line);
  } catch (...) {
    line.clear();
  }

  tcsetattr(STDIN_FILENO, TCSANOW, &term_flags_orig);

  cout << endl;

  return line;
}
