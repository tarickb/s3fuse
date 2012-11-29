#include <string.h>

#include <iostream>
#include <string>
#include <vector>

#include "base/xml.h"

using std::cin;
using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::vector;

using s3::base::xml;

int main(int argc, char **argv)
{
  vector<char> vec;
  string xpath;

  if (argc != 2) {
    cerr << "usage: " << argv[0] << " <xpath>" << endl;
    return 1;
  }

  xpath = argv[1];

  xml::init();

  while (cin.good()) {
    string line;

    getline(cin, line);

    if (line.empty())
      continue;

    vec.resize(line.size());
    memcpy(&vec[0], line.c_str(), line.size());

    cout << "match: " << xml::match(vec, xpath.c_str()) << endl;
  }

  return 0;
}
