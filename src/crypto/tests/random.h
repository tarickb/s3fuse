#ifndef S3_CRYPTO_TESTS_RANDOM_H
#define S3_CRYPTO_TESTS_RANDOM_H

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace s3 {
namespace crypto {
namespace tests {
class Random {
 public:
  inline static std::vector<uint8_t> Read(size_t size) {
    std::ifstream f("/dev/urandom", std::ifstream::in);
    if (!f.good()) throw std::runtime_error("unable to open /dev/urandom");
    std::vector<uint8_t> out(size);
    f.read(reinterpret_cast<char *>(&out[0]), size);
    if (static_cast<size_t>(f.gcount()) != size)
      throw std::runtime_error("failed to read requested number of bytes");
    return out;
  }
};
}  // namespace tests
}  // namespace crypto
}  // namespace s3

#endif
