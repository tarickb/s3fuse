#include <iostream>

#include "md5.h"
#include "util.h"

using namespace std;

using namespace s3;

int main(int argc, char **argv)
{
  uint8_t buf[1024] = { 0 };
  uint8_t hash[md5::HASH_LEN];

  cout << "hash of null buffer: " << util::encode_hash<md5>(buf, 1024, E_HEX) << std::endl;
}
