#include <iostream>

#include "crypto/passwords.h"

int main(int argc, char **argv) {
  std::string pwd;

  pwd = s3::crypto::Passwords::ReadFromStdin("enter volume password: ");

  std::cout << "password is [" << pwd << "]" << std::endl;

  return 0;
}
