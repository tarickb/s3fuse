/*
 * sha256_sum.cc
 * -------------------------------------------------------------------------
 * Calculates a SHA256 hash of the specified file using the same hash-list
 * method as the file uploader.
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
#include <sys/stat.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <iostream>

#include "crypto/hash_list.h"
#include "crypto/hex.h"
#include "crypto/sha256.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    const char *arg0 = std::strrchr(argv[0], '/');
    std::cerr << "Usage: " << (arg0 ? arg0 + 1 : argv[0]) << " <file-name>"
              << std::endl;
    return 1;
  }

  const char *file_name = argv[1];
  int fd = open(file_name, O_RDONLY);
  if (fd == -1) {
    std::cerr << "Error [" << strerror(errno) << "] (" << errno
              << ") while opening [" << file_name << "]." << std::endl;
    return 1;
  }

  struct stat s;
  if (fstat(fd, &s)) {
    std::cerr << "Error [" << strerror(errno) << "] (" << errno
              << ") while stat-ing [" << file_name << "]." << std::endl;
    return 1;
  }

  try {
    using Sha256Hash = s3::crypto::HashList<s3::crypto::Sha256>;

    Sha256Hash hash(s.st_size);
    off_t offset = 0;
    size_t remaining = s.st_size;

    while (remaining) {
      uint8_t buffer[Sha256Hash::CHUNK_SIZE];
      size_t this_chunk = (remaining > Sha256Hash::CHUNK_SIZE)
                              ? Sha256Hash::CHUNK_SIZE
                              : remaining;

      if (pread(fd, buffer, this_chunk, offset) !=
          static_cast<ssize_t>(this_chunk))
        throw std::runtime_error("pread() failed");

      hash.ComputeHash(offset, buffer, this_chunk);

      remaining -= this_chunk;
      offset += this_chunk;
    }

    std::cout << hash.GetRootHash<s3::crypto::Hex>() << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Caught exception " << e.what() << " while hashing ["
              << file_name << "]" << std::endl;
    return 1;
  }

  return 0;
}
