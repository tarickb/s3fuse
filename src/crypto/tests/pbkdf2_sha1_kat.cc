#include <iostream>
#include <string>
#include <boost/lexical_cast.hpp>

#include "crypto/buffer.h"
#include "crypto/pbkdf2_sha1.h"

using boost::lexical_cast;
using std::cin;
using std::cout;
using std::endl;
using std::runtime_error;
using std::string;

using s3::crypto::buffer;
using s3::crypto::pbkdf2_sha1;

int main(int argc, char **argv)
{
  string password, salt, output;
  int rounds = 0, key_len = 0;

  while (!cin.eof()) {
    string line, first, last;
    size_t pos;

    getline(cin, line);
    pos = line.find(": ");

    if (line[0] == '#' || pos == string::npos)
      continue;

    first = line.substr(0, pos);
    last = line.substr(pos + 2);

    if (first == "password")
      password = last;
    else if (first == "salt")
      salt = last;
    else if (first == "output")
      output = last;
    else if (first == "rounds")
      rounds = lexical_cast<int>(last);
    else if (first == "key_len")
      key_len = lexical_cast<int>(last);

    if (!password.empty() && !salt.empty() && !output.empty() && rounds && key_len) {
      string key_out;

      try {
        buffer::ptr key;

        key = pbkdf2_sha1::derive(password, salt, rounds, key_len);
        key_out = key->to_string();

        if (key_out != output)
          throw runtime_error("key derivation failed");

        cout << "PASSED: rounds: " << rounds << ", key len: " << key_len << endl;

      } catch (const std::exception &e) {
        cout
          << "FAILED: " << e.what() << endl
          << "  password: " << password << endl
          << "  salt: " << salt << endl
          << "  rounds: " << rounds << endl
          << "  key len: " << key_len << endl
          << "  expected: " << output << endl
          << "  derived: " << key_out << endl;

        return 1;
      }

      password.clear();
      salt.clear();
      output.clear();
      rounds = 0;
      key_len = 0;
    }
  }

  return 0;
}
