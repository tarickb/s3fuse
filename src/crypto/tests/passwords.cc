#include <iostream>

#include "crypto/passwords.h"

using std::cout;
using std::endl;
using std::string;

using s3::crypto::passwords;

int main(int argc, char **argv)
{
  string pwd;

  pwd = passwords::read_from_stdin("enter volume password: ");

  cout << "password is [" << pwd << "]" << endl;

  return 0;
}
