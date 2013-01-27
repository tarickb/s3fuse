/*
 * crypto/passwords.cc
 * -------------------------------------------------------------------------
 * Password management (implementation).
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2012, Tarick Bedeir.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <signal.h>
#include <stdlib.h>
#include <string.h>
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

namespace
{
  termios s_term_flags_orig;

  void reset_term_attrs(int, siginfo_t *, void *)
  {
    tcsetattr(STDIN_FILENO, TCSANOW, &s_term_flags_orig);
    exit(1);
  }
}

string passwords::read_from_stdin(const string &prompt)
{
  string line;
  termios term_flags;
  struct sigaction new_action, old_int_action, old_term_action;

  memset(&new_action, 0, sizeof(new_action));
  memset(&old_int_action, 0, sizeof(old_int_action));
  memset(&old_term_action, 0, sizeof(old_term_action));

  new_action.sa_sigaction = reset_term_attrs;
  new_action.sa_flags = SA_SIGINFO;

  tcgetattr(STDIN_FILENO, &s_term_flags_orig);
  memcpy(&term_flags, &s_term_flags_orig, sizeof(term_flags));

  cout << prompt;

  term_flags.c_lflag &= ~ECHO;

  sigaction(SIGINT, &new_action, &old_int_action);
  sigaction(SIGTERM, &new_action, &old_term_action);

  tcsetattr(STDIN_FILENO, TCSANOW, &term_flags);

  try {
    getline(cin, line);
  } catch (...) {
    line.clear();
  }

  tcsetattr(STDIN_FILENO, TCSANOW, &s_term_flags_orig);

  sigaction(SIGINT, &old_int_action, NULL);
  sigaction(SIGTERM, &old_term_action, NULL);

  cout << endl;

  return line;
}
