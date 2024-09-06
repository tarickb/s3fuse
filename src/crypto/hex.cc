/*
 * crypto/hex.cc
 * -------------------------------------------------------------------------
 * Hex encoder (implementation).
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

#include "crypto/hex.h"

#include <cstdint>
#include <stdexcept>

namespace s3 {
namespace crypto {

namespace {
inline int HexCharToInt(char c) {
  return (c < 'a') ? (c - '0') : (c - 'a' + 10);
}
}  // namespace

std::string Hex::Encode(const uint8_t *input, size_t size) {
  constexpr char HEX[] = "0123456789abcdef";

  std::string ret;
  ret.resize(size * 2);
  for (size_t i = 0; i < size; i++) {
    ret[2 * i + 0] = HEX[static_cast<uint8_t>(input[i]) / 16];
    ret[2 * i + 1] = HEX[static_cast<uint8_t>(input[i]) % 16];
  }
  return ret;
}

std::vector<uint8_t> Hex::Decode(const std::string &input) {
  if (input.size() % 2)
    throw std::runtime_error(
        "cannot have odd number of hex characters to decode!");

  std::vector<uint8_t> output(input.size() / 2);
  for (size_t i = 0; i < output.size(); i++)
    output[i] =
        HexCharToInt(input[2 * i + 0]) * 16 + HexCharToInt(input[2 * i + 1]);
  return output;
}

}  // namespace crypto
}  // namespace s3
