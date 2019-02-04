/*
 * sha256_sum.cc
 * -------------------------------------------------------------------------
 * Calculates a SHA256 hash of the specified file using the same hash-list
 * method as the s3fuse file uploader.
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

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include <cstring>
#include <iostream>

#include "crypto/hash_list.h"
#include "crypto/hex.h"
#include "crypto/sha256.h"

using std::cerr;
using std::cout;
using std::endl;
using std::runtime_error;
using std::strrchr;

using s3::crypto::hash_list;
using s3::crypto::hex;
using s3::crypto::sha256;

namespace
{
  typedef hash_list<sha256> sha256_hash;
}

int main(int argc, char **argv)
{
  struct stat s;
  const char *file_name = NULL;
  int fd = -1;
  sha256_hash::ptr hash;

  if (argc != 2) {
    const char *arg0 = strrchr(argv[0], '/');

    cerr << "Usage: " << (arg0 ? arg0 + 1 : argv[0]) << " <file-name>" << endl;
    return 1;
  }

  file_name = argv[1];
  fd = open(file_name, O_RDONLY);

  if (fd == -1) {
    cerr << "Error [" << strerror(errno) << "] (" << errno << ") while opening [" << file_name << "]." << endl;
    return 1;
  }

  if (fstat(fd, &s)) {
    cerr << "Error [" << strerror(errno) << "] (" << errno << ") while stat-ing [" << file_name << "]." << endl;
    return 1;
  }

  try {
    off_t offset = 0;
    size_t remaining = s.st_size;
    uint8_t buffer[sha256_hash::CHUNK_SIZE];

    hash.reset(new sha256_hash(remaining));

    while (remaining) {
      size_t this_chunk = (remaining > sha256_hash::CHUNK_SIZE) ? sha256_hash::CHUNK_SIZE : remaining;

      if (pread(fd, buffer, this_chunk, offset) != static_cast<ssize_t>(this_chunk))
        throw runtime_error("pread() failed");

      hash->compute_hash(offset, buffer, this_chunk);

      remaining -= this_chunk;
      offset += this_chunk;
    }

    cout << hash->get_root_hash<hex>() << endl;

  } catch (const std::exception &e) {
    cerr << "Caught exception " << e.what() << " while hashing [" << file_name << "]" << endl;
    return 1;
  }

  return 0;
}
