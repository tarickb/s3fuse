#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <iostream>

#include "crypto/hash.h"
#include "crypto/hex.h"
#include "crypto/md5.h"

using std::cout;
using std::endl;

using s3::crypto::hash;
using s3::crypto::hex;
using s3::crypto::md5;

int main(int argc, char **argv)
{
  int fd = -1;

  if (argc != 2)
    return 1;

  fd = open(argv[1], O_RDONLY);

  cout << "md5: " << hash::compute<md5, hex>(fd) << endl;

  return 0;
}
