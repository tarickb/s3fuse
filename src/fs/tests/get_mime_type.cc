#include <iostream>

#include "fs/mime_types.h"

using std::cerr;
using std::cout;
using std::endl;
using std::string;

using s3::fs::mime_types;

int main(int argc, char **argv)
{
  string ext, type;

  if (argc != 2) {
    cerr << "usage: " << argv[0] << " <extension>" << endl;
    return 1;
  }

  mime_types::init();

  ext = argv[1];
  type = mime_types::get_type_by_extension(ext);

  if (type.empty()) {
    cerr << "matching type not found" << endl;
    return 1;
  }

  cout << type << endl;
  return 0;
}
