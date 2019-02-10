#include <iostream>

#include "fs/mime_types.h"

int main(int argc, char **argv) {
  std::string ext, type;

  if (argc != 2) {
    std::cerr << "usage: " << argv[0] << " <extension>" << std::endl;
    return 1;
  }

  s3::fs::MimeTypes::Init();

  ext = argv[1];
  type = s3::fs::MimeTypes::GetTypeByExtension(ext);

  if (type.empty()) {
    std::cerr << "matching type not found" << std::endl;
    return 1;
  }

  std::cout << type << std::endl;
  return 0;
}
