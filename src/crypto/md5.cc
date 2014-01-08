/*
 * crypto/md5.cc
 * -------------------------------------------------------------------------
 * MD5 hasher (implementation).
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

#include <unistd.h>

#include <stdexcept>

#include <openssl/evp.h>
#include <openssl/md5.h>

#include "crypto/md5.h"

using std::runtime_error;

using s3::crypto::md5;

void md5::compute(const uint8_t *input, size_t size, uint8_t *hash)
{
  MD5(input, size, hash);
}

void md5::compute(int fd, uint8_t *hash)
{
  const ssize_t BUF_LEN = 8 * 1024;
  off_t offset = 0;
  char buf[BUF_LEN];

  EVP_MD_CTX md5_ctx;

  EVP_DigestInit(&md5_ctx, EVP_md5());

  while (true) {
    ssize_t read_count;

    read_count = pread(fd, buf, BUF_LEN, offset);

    if (read_count == -1)
      throw runtime_error("error while computing md5, in pread().");

    EVP_DigestUpdate(&md5_ctx, buf, read_count);

    offset += read_count;

    if (read_count < BUF_LEN)
      break;
  }

  EVP_DigestFinal(&md5_ctx, hash, NULL);
}
